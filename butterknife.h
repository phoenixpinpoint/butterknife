/**
 * @file butterknife.h
 * @author Adam Guthrie
 * @brief butterknife.h
 */
#ifndef BUTTERKNIFE_H
#define BUTTERKNIFE_H

#include <unistd.h>
#include <re.h>
#include <buffer.h>
#include <fs.h>
#include <vec.h>

#ifndef FS_PATH_MAX
#define FS_PATH_MAX 1024
#endif

#ifndef MAX_CONTENT_TAGS
#define MAX_CONTENT_TAGS 1000
#endif

// #define BK_DEBUG //Un-comment for debugging.

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
buffer_t* bk_buffer_t_insert(buffer_t* destination, buffer_t* source, int leftSlice, int rightSlice);

/**
 * Takes in a buffer and a tagPattern, find first tag, trims it from the buffer and returns
 * the trimmed buffer. 
*/
vec_void_t bk_get_next_tag(buffer_t* buffer, char* tagPattern);


/**
 * takes a page buffer, tag and returns the section data
*/
buffer_t* bk_get_section_data(buffer_t* page, buffer_t* tag);

#endif