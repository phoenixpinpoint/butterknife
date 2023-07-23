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
    // Create a buffer_t to store the generated html.
    buffer_t* htmlPage;
    
    // Get the base filename without the path. 
    char* basename;
    size_t basenameLength;
    cwk_path_get_basename(webpageFilePath, &basename, &basenameLength);

    //Get the dirname
    size_t dirNameLength;
    buffer_t* dirpath = buffer_new();
    buffer_append(dirpath, webpageFilePath);
    cwk_path_get_dirname(dirpath->data, &dirNameLength);// Get the length of the dirname
    dirpath = buffer_slice(dirpath, 0, dirNameLength);
    

    // Get the current working directory.
    char* cwd = (char*)malloc(FS_PATH_MAX*sizeof(char));
    getcwd(cwd, FS_PATH_MAX);

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

            //Handle page sections.
            //Find each section tag in the template.
            int firstTagLength;
            int firstTagStartIndex = re_match("@yields [^\\0;]+;", htmlPage->data, &firstTagLength);
            if (firstTagStartIndex != -1)//If there is any @yields tags
            {
                //get the original page.
                buffer_t* tagData = buffer_new();
                buffer_append(tagData, pageFile);

                //Get the tag
                buffer_t* tagToFind = buffer_slice(htmlPage, firstTagStartIndex, firstTagStartIndex+firstTagLength);
                size_t lenOfYieldTag = strlen("@yields ");
                tagToFind = buffer_slice(tagToFind, lenOfYieldTag, strlen(tagToFind->data));
                tagToFind = buffer_slice(tagToFind, 0, strlen(tagToFind->data)-1);

                //Build the tag specs
                buffer_t* openSectionTag = buffer_new();
                buffer_append(openSectionTag, "@section ");
                buffer_append(openSectionTag, tagToFind->data);

                buffer_t* closeSectionTag = buffer_new();
                buffer_append(closeSectionTag, "@sectionclose");

                //Get the value of the tag.
                int lengthOfTagStart;
                int indexOfTagStart = re_match(openSectionTag->data, tagData->data, &lengthOfTagStart);
                int lengthOfTagEnd;
                int indexOfTagEnd = re_match(closeSectionTag->data, tagData->data, &lengthOfTagEnd);
                tagData = buffer_slice(tagData, indexOfTagStart+lengthOfTagStart, indexOfTagEnd);
                
                //Insert the tag value.
                htmlPage = bk_buffer_t_insert(htmlPage, tagData, firstTagStartIndex, firstTagStartIndex+firstTagLength);
                
                // Split the rendered page.
                //buffer_t* leftHalfOfPage = buffer_slice(htmlPage, 0, firstTagStartIndex);
                //buffer_t* rightHalfOfPage = buffer_slice(htmlPage, firstTagStartIndex+firstTagLength, strlen(htmlPage->data));






                // //Reset the HTML Page
                // buffer_free(htmlPage);

                // //Rebuild the HTML Page
                // htmlPage = buffer_new();
                // buffer_append(htmlPage, leftHalfOfPage->data);
                // buffer_append(htmlPage, tagData->data);
                // buffer_append(htmlPage, rightHalfOfPage->data);

                // for(int iterator = 0; iterator < MAX_CONTENT_TAGS; iterator++)
                // {
                    
                // }              
            }
        }
        //buffer_free(pageBuffer);
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