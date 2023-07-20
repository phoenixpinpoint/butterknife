/**
 * @file butterknife.h
 * @author Adam Guthrie
 * @brief butterknife.h
 */
#ifndef BUTTERKNIFE_H
#define BUTTERKNIFE_H

#include "deps/tiny-regex-c/re.h"
#include "deps/buffer/buffer.h"
#include "deps/fs/fs.h"

#ifndef FS_PATH_MAX
#define FS_PATH_MAX 1024
#endif

/**
 * @brief Generates a webpage and returns a buffer_t
 * 
 * @param webpageFilePath - the path to the file.
 * @return buffer_t* 
 */
buffer_t* bk_generate_webpage(char* webpageFilePath);

/**
 * @brief loads a layout into a buffer
 * 
 * @param layoutFilePath 
 * @return buffer_t* 
 */
buffer_t* bk_load_layout(char* layoutFilePath);

#endif