/**
 * @file butterknife.h
 * @author Adam Guthrie
 * @brief butterknife.h
 */
#include "butterknife.h"

#include "deps/buffer/buffer.c"
#include "deps/fs/fs.c"
#include "deps/tiny-regex-c/re.c"
#include "deps/cwalk/cwalk.c"
#include "deps/vec/vec.c"

/**
 * @brief Generates a webpage and returns a buffer_t
 * 
 * Render Order:
 * Layout
 * Head 
 * Page
 * 
 * @return buffer_t* 
 */
buffer_t* bk_generate_webpage(char* webpageFilePath)
{
    #ifdef BK_DEBUG
        printf("Webpage Path: %s\n", webpageFilePath);
    #endif

    //return (buffer_t*)0;
    
    // Create a buffer_t to store the generated html.
    buffer_t* htmlPage;
    
    // Get the base filename without the path. 
    char* basename;
    size_t basenameLength;
    cwk_path_get_basename(webpageFilePath, &basename, &basenameLength);

    #ifdef BK_DEBUG
        printf("basename: %s\n", basename);
    #endif

    //Get the dirname
    size_t dirNameLength;
    buffer_t* dirpath = buffer_new();
    buffer_append(dirpath, webpageFilePath);
    cwk_path_get_dirname(dirpath->data, &dirNameLength);// Get the length of the dirname
    dirpath = buffer_slice(dirpath, 0, dirNameLength);

    #ifdef BK_DEBUG
        printf("dirpath: %s\n", dirpath->data);
    #endif

    // Get the current working directory.
    char* cwd = (char*)malloc(FS_PATH_MAX*sizeof(char));
    getcwd(cwd, FS_PATH_MAX);

    #ifdef BK_DEBUG
        printf("cwd: %s\n", cwd);
    #endif

    //Load the pagefile into a buffer.
    char* pageFile = fs_read(webpageFilePath);
    
    //If the buffer is not null. 
    if(pageFile)
    {
        buffer_t* layoutPath;

        //Check for layout Tag
        int layoutTagMatchLength;
        int layoutTagMatchStartIndex = re_match("@layout [^\\0]+;", pageFile, &layoutTagMatchLength);

        //If a layout tag is found
        if (layoutTagMatchStartIndex != -1)
        {
            //Create a buffer for the layout tag
            buffer_t* layoutTag = buffer_new();
            buffer_append(layoutTag, pageFile);
            layoutTag = buffer_slice(layoutTag, layoutTagMatchStartIndex, layoutTagMatchStartIndex+layoutTagMatchLength);

            //Get the raw path from the tag
            buffer_t* rawLayoutPath = buffer_new();
            buffer_append(rawLayoutPath, layoutTag->data);
            rawLayoutPath = buffer_slice(rawLayoutPath, strlen("@layout "), strlen(layoutTag->data)-strlen(";"));

            //Create buffer_t for the path and prepend our dirname
            layoutPath = buffer_new();
            buffer_append(layoutPath, dirpath->data);
            buffer_append(layoutPath, rawLayoutPath->data);

            #ifdef BK_DEBUG
                printf("Layout Path: %s\n",layoutPath->data);
            #endif

            //Load the layout and free the buffer.
            htmlPage = bk_load_layout(layoutPath->data);
            buffer_free(layoutPath);
            buffer_free(layoutTag);
            buffer_free(rawLayoutPath);
        }
        
        //Load the additional header data into the head tag.
        buffer_t* pageBuffer = buffer_new();
        buffer_append(pageBuffer, pageFile);
        
        //Check if layout has <head> and @head tags
        size_t indexOfHeadTag = buffer_indexof(htmlPage,"<head>");
        size_t indexOfCloseHeadTag = buffer_indexof(htmlPage,"</head>");
        size_t indexOfBKHeadTag = buffer_indexof(pageBuffer,"@head");
        size_t indexOfBKCloseHeadTag = buffer_indexof(pageBuffer,"@headclose");

        //If the page has a @head and @headclose tag and  we need to add the body to the <head> tag
        if(indexOfBKHeadTag != -1 && indexOfBKCloseHeadTag != -1)
        {
            //If the layout already has a <head> tag
            if (indexOfHeadTag != -1 && indexOfCloseHeadTag != -1)
            {
                #ifdef BK_DEBUG
                    printf("Page and Template have <head>/@head tag\n");
                #endif
                //Load the existing page data.
                size_t lengthOfHeadTag = strlen("<head>"); 
                size_t lengthOfCloseHeadTag = strlen("</head>"); 
                buffer_t* existingHeaderValue = buffer_slice(htmlPage, indexOfHeadTag+lengthOfHeadTag, indexOfCloseHeadTag);

                //Load the additional header data.
                size_t lengthOfBKHeadTag = strlen("@head"); 
                size_t lengthOfBKCloseHeadTag = strlen("@headclose");
                buffer_t* additionalHeaderValue = buffer_slice(pageBuffer, indexOfBKHeadTag+lengthOfBKHeadTag, indexOfBKCloseHeadTag);

                //Append the new header data to the existing.
                buffer_append(existingHeaderValue, additionalHeaderValue->data);

                htmlPage = bk_buffer_t_insert(htmlPage, existingHeaderValue, indexOfHeadTag+lengthOfHeadTag, indexOfCloseHeadTag);

                buffer_free(existingHeaderValue);
                buffer_free(additionalHeaderValue);
            }
            else {//If not we need to create one. 
                #ifdef BK_DEBUG
                    printf("Only Page has @head tag\n");
                #endif
                //Locate the <html> and append a new <head> tag to it.
                size_t indexOfHTMLTag = buffer_indexof(htmlPage, "<html>");
                size_t lengthOfHTMLTag = strlen("<html>");

                size_t lengthOfBKHeadTag = strlen("@head"); 
                size_t lengthOfBKCloseHeadTag = strlen("@headclose");
                buffer_t* headerValue = buffer_slice(pageBuffer, indexOfBKHeadTag+lengthOfBKHeadTag, indexOfBKCloseHeadTag);

                buffer_t* headerTag = buffer_new();
                buffer_append(headerTag, "<head>");
                buffer_append(headerTag, headerValue->data);
                buffer_append(headerTag, "</head>");

                //printf("Free ERROR: %s\n", htmlPage->data);
                htmlPage = bk_buffer_t_insert(htmlPage, headerTag, indexOfHTMLTag, indexOfHTMLTag+lengthOfHTMLTag);

                buffer_free(headerTag);
                buffer_free(headerValue);
            }

            #ifdef BK_DEBUG
                printf("Getting Page Sections.\n");
            #endif
            //Handle page sections.
            //Find each section tag in the template, rendering them and updating the html page.
            int loopCount = 0; 
            while(loopCount < MAX_CONTENT_TAGS)
            {                
                //get the original page.
                buffer_t* pageData = buffer_new();
                buffer_append(pageData, pageFile);

                //Get the tag from the page
                vec_void_t getTag = bk_get_next_tag(htmlPage, "@yields [^\\0;]+;");
                buffer_t* tag = (buffer_t*)getTag.data[0];
                
                //If the response tag is null empty, break the loop.
                if(tag->data == NULL || strlen(tag->data) == 1)
                {
                    break;
                }

                #ifdef BK_DEBUG
                    printf("Found Tag: %s\n", tag->data);
                #endif

                //Get the tag
                size_t lenOfYieldTag = strlen("@yields ");
                tag = buffer_slice(tag, lenOfYieldTag, strlen(tag->data));
                tag = buffer_slice(tag, 0, strlen(tag->data)-1);

                #ifdef BK_DEBUG
                    printf("Cleaned Tag to: %s\n", tag->data);
                #endif

                //If we cannot get the section break the loop.
                pageData = bk_get_section_data(pageData,tag);
                if(pageData->data == NULL)
                {
                    break;
                }

                //printf("Inserting Data *%s*\n at %d to %d\n", pageData->data, getTag.data[2], (int)getTag.data[2]+(int)getTag.data[3]);

                //Update the html page.
                htmlPage = bk_buffer_t_insert(htmlPage, pageData, getTag.data[1], (int)getTag.data[1]+(int)getTag.data[2]);

                //Clean-up
                vec_deinit(&getTag);
                buffer_free(pageData);
                loopCount++;
            }
        }
        buffer_free(pageBuffer);
    }
    else//If it is null.
    {
        //Return a blank page.
        htmlPage = buffer_new_with_string("\0");
    }

    return htmlPage;

}

/**
 * @brief loads a layout into a buffer
 * 
 * @param layoutFilePath 
 * @return buffer_t* 
 */
buffer_t* bk_load_layout(char* layoutFilePath)
{
    // Create a buffer_t to store the generate html.
    buffer_t* layout;
    
    char* layoutBuffer = fs_read(layoutFilePath);

    //If the buffer is not null. 
    if(layoutBuffer)
    {
        layout = buffer_new();
        buffer_append(layout, layoutBuffer);
        free(layoutBuffer);
        return layout;
    }
    else//If it is null.
    {
        //Return a blank page.
        layout = buffer_new_with_string("\0");
        return layout;
    }
}

/**
 * @brief splits a string from 0 to leftSlice, and rightSlice and the strlen. Inserting
 * the source buffer_t into the destination buffer_t.
 * 
 * @param destination
 * @param source
 * @param leftSlice
 * @param rightSlice
 * @return buffer_t*
*/
buffer_t* bk_buffer_t_insert(buffer_t* destination, buffer_t* source, int leftSlice, int rightSlice)
{
    //Split the buffer
    buffer_t* leftHalfOfBuffer = buffer_slice(destination, 0, leftSlice);
    buffer_t* rightHalfOfBuffer = buffer_slice(destination, rightSlice, strlen(destination->data));

    //printf("SANITY: %s\n", destination->data);

    //Free the original buffer
    buffer_free(destination);

    //Create new and glue together the 3 pieces.
    destination = buffer_new();
    buffer_append(destination, leftHalfOfBuffer->data);
    buffer_append(destination, source->data);
    buffer_append(destination, rightHalfOfBuffer->data);

    //Cleanup the extra buffers.
    buffer_free(leftHalfOfBuffer);
    buffer_free(rightHalfOfBuffer);

    //Return the newly created buffer.
    return destination;
}

/**
 * Takes in a buffer and a tagPattern, find first tag, trims it from the buffer and returns
 * the trimmed buffer. Returns the tag and trims the buffer. 
*/
vec_void_t bk_get_next_tag(buffer_t* buffer, char* tagPattern)
{
    vec_void_t results;
    buffer_t* tag;

    //Gets the match
    int tagLength;
    int tagStartIndex = re_match(tagPattern, buffer->data, &tagLength);
    
    //If a match is not found, return nullptr to tag
    if(tagStartIndex == -1 || tagLength == -1)
    {
        return results;
    }

    //Extract the tag, trim the buffer and, return
    tag = buffer_slice(buffer, tagStartIndex, tagStartIndex+tagLength);

    vec_init(&results);
    vec_push(&results, tag);
    vec_push(&results, tagStartIndex);
    vec_push(&results, tagLength);

    //printf("TAG SI: %d & Len: %d\n", results.data[2], results.data[3]);

    return results;
}

/**
 * takes a page buffer, tag and returns the section data
*/
buffer_t* bk_get_section_data(buffer_t* page, buffer_t* tag)
{
    //Build the tag specs
    buffer_t* openSectionTag = buffer_new();
    buffer_append(openSectionTag, "@section ");
    buffer_append(openSectionTag, tag->data);
    buffer_t* closeSectionTag = buffer_new();
    buffer_append(closeSectionTag, "@sectionclose");

    //Pull from @section..... to the last @sectionclose
    buffer_t* tagBlock = buffer_new();
    buffer_append(tagBlock, openSectionTag->data);
    buffer_append(tagBlock, "[ ]*\\\n[^]+");
    int lengthOfBlock;
    int indexOfBlockStart = re_match(tagBlock->data, page->data, &lengthOfBlock);
    //printf("%d:%d:%d\n",indexOfBlockStart, lengthOfBlock, indexOfBlockStart+lengthOfBlock);
    if(indexOfBlockStart == -1 || lengthOfBlock == -1)
    {
        return (buffer_t*)0;
    }
    buffer_t* tagBlockValue = buffer_slice(page, indexOfBlockStart, indexOfBlockStart+lengthOfBlock);

    //Locate the first @section close after our @section......
    int lengthOfCloseSectionTag;
    int indexOfCloseSectionTag = re_match(closeSectionTag->data, tagBlockValue->data, &lengthOfCloseSectionTag);
    if(indexOfCloseSectionTag == -1 || lengthOfCloseSectionTag == -1)
    {
        return (buffer_t*)0;
    }

    //Remove everything frome the first @sectionclose to the last EOF
    tagBlockValue = buffer_slice(tagBlockValue, 0, indexOfCloseSectionTag);
    //Remove @section....... tag
    tagBlockValue = buffer_slice(tagBlockValue, strlen(openSectionTag->data), strlen(tagBlockValue->data));
    //printf("TBV: *%s*\n", tagBlockValue->data);

    //Clean up.
    buffer_free(openSectionTag);
    buffer_free(closeSectionTag);
    buffer_free(tagBlock);

    return tagBlockValue;
} 