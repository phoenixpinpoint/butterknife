/**
 * @file butterknife.h
 * @author Adam Guthrie
 * @brief butterknife.h
 */
#include "butterknife.h"

#include "deps/buffer/buffer.c"
#include "deps/fs/fs.c"
#include "deps/tiny-regex-c/re.c"

/**
 * @brief Generates a webpage and returns a buffer_t
 * 
 * @return buffer_t* 
 */
buffer_t* bk_generate_webpage(char* webpageFilePath)
{
    buffer_t *pageName;
    int page_match_length;
    int page_match_idx = re_match("[A-z0-9]+\\.[A-z0-9]+\\.[A-z0-9]+", webpageFilePath, &page_match_length);
    if (page_match_idx != -1)
    {
        //Store the location of the match start as the start of the pointer for our new string. 
        //And set the stop to the match_length
        pageName = buffer_new_with_string_length(&page_match_idx[webpageFilePath], page_match_length);
    }

    // Create a buffer_t to store the generate html.
    buffer_t* htmlPage;

    //Load the pagefile into a buffer.
    char* pageFile = fs_read(webpageFilePath);
    
    //If the buffer is not null. 
    if(pageFile)
    {
        //Get the layout name.
        buffer_t* layoutName;
        int match_length;
        int match_idx = re_match("[A-z0-9\-_\.]+;", pageFile, &match_length);
        if (match_idx != -1)
        {
            //Store the location of the match start as the start of the pointer for our new string. 
            //And set the stop to the match_length
            layoutName = buffer_new_with_string_length(&match_idx[pageFile], match_length);
            layoutName->data[layoutName->len-1] = '\0';//Strip the ; and add a null terminator.
        }
        //Load the layout.
        printf("FILE: %s\n", layoutName);
        buffer_t *layoutPath = buffer_new_with_string("./");
        buffer_t *layoutFile = bk_load_layout(layoutPath->data);
        printf("LAYOUT: *%s*\n", layoutFile->data);
    }
    else//If it is null.
    {
        //Return a blank page.
        htmlPage = buffer_new_with_string("\0");
        return htmlPage;
    }
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
        layout = buffer_new_with_string(layoutBuffer);
        return layout;
    }
    else//If it is null.
    {
        //Return a blank page.
        layout = buffer_new_with_string("\0");
        return layout;
    }
}