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
    char* dirpath = (char*)malloc(strlen(webpageFilePath) * sizeof(char));
    strncpy(dirpath, webpageFilePath, strlen(webpageFilePath)); // Copy the path in to a string
    cwk_path_get_dirname(dirpath, &dirNameLength);// Get the length of the dirname
    buffer_t* dirnameBuff = buffer_new_with_string(dirpath);//Create a buffer_t of that path
    dirnameBuff = buffer_slice(dirnameBuff, 0, dirNameLength);//Slice down to dirname
    char* dirname = (char*)malloc(strlen(dirnameBuff->data) * sizeof(char));
    strncpy(dirname, dirnameBuff->data, strlen(dirnameBuff->data));
    buffer_free(dirnameBuff);

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
            char* layoutTag = (char*)malloc(layoutTagMatchLength * sizeof(char));
            strncpy(layoutTag, pageFile+layoutTagMatchStartIndex, layoutTagMatchLength);
            layoutTag[layoutTagMatchLength] = '\0';

            //Get the raw path from the tag
            char* rawLayoutPath = (char*)malloc((strlen(layoutTag) - strlen("@layout ")) * sizeof(char));
            strncpy(rawLayoutPath, layoutTag+strlen("@layout "), strlen(layoutTag) - strlen("@layout "));
            rawLayoutPath[strlen(rawLayoutPath)-1] = '\0';

            //Create buffer_t for the path and prepend our dirname
            layoutPath = buffer_new();
            buffer_append(layoutPath, dirname);
            buffer_append(layoutPath, rawLayoutPath);

            //Load the layout.
            htmlPage = bk_load_layout(layoutPath->data);

        }

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