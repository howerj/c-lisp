/**
 *  @file           mem.c
 *  @brief          Memory allocation wrappers and handling
 *  @author         Richard James Howe.
 *  @copyright      Copyright 2013 Richard James Howe.
 *  @license        GPL v3.0
 *  @email          howe.r.j.89@gmail.com
 *  @details
 *
 *  Wrappers for allocation. Garabage collection and error handling on
 *  Out-Of-Memory errors should go here as well to keep things out of
 *  the way.
 *
 *  @todo The actual garbage collection stuff.
 *  @todo If an allocation fails, garbage should be collected, then an
 *        allocation reattempted, if it fails again it should abort.
 *  @todo Debug functions; maximum allocations / deallocations for example
 *  @todo Remove instances of fprintf
 */

#include "type.h"
#include "io.h"
#include "mem.h"
#include <stdlib.h> /** malloc(), calloc(), realloc(), free(), exit() */



static unsigned int alloccounter = 0;
static expr alloclist = NULL;
/**** malloc wrappers ********************************************************/

/**
 *  @brief          wrapper around malloc 
 *  @param          size size of desired memory block in bytes
 *  @param          e    error output stream
 *  @return         pointer to newly allocated storage on sucess, exits
 *                  program on failure!
 **/
void *wmalloc(size_t size, io *e){
  void* v;
  if(MAX_ALLOCS < alloccounter++){
    fprintf(stderr,"too many mallocs\n");
    exit(EXIT_FAILURE);
  }
  v = malloc(size);
  if(NULL == v){
    if(NULL == e){
      fprintf(stderr, "malloc failed and *e == NULL\n");
    } else {
    }
    exit(EXIT_FAILURE);
  }
  return v;
}

/**
 *  @brief          wrapper around calloc
 *  @param          num  number of elements to allocate
 *  @param          size size of elements to allocate
 *  @param          e    error output stream
 *  @return         pointer to newly allocated storage on sucess, which
 *                  is zeroed, exits program on failure!
 **/
void *wcalloc(size_t num, size_t size, io *e){
  void* v;
  if(MAX_ALLOCS < alloccounter++){
    fprintf(stderr,"too many mallocs\n");
    exit(EXIT_FAILURE);
  }
  v = calloc(num,size);
  if(NULL == v){
    if(NULL == e){
      fprintf(stderr, "calloc failed and *e == NULL\n");
    } else {
    }
    exit(EXIT_FAILURE);
  }
  return v;
}

/**
 *  @brief          wrapper around realloc, no gc necessary
 *  @param          size size of desired memory block in bytes
 *  @param          ptr  existing memory block to resize
 *  @param          size size of desired memory block in bytes
 *  @param          e    error output stream
 *  @return         pointer to newly resized storage on success, 
 *                  exits program on failure!
 **/
void *wrealloc(void *ptr, size_t size, io *e){
  void* v;
  v = realloc(ptr,size);
  if(NULL == v){
    if(NULL == e){
      fprintf(stderr, "realloc failed and *e == NULL\n");
    } else {
    }
    exit(EXIT_FAILURE);
  }
  return v;
}

/**
 *  @brief          wrapper around malloc for garbage collection
 *  @param          size size of desired memory block in bytes
 *  @param          e    error output stream
 *  @return         pointer to newly allocated storage on sucess, exits
 *                  program on failure!
 **/
void *gcmalloc(size_t size, io *e){
  void* v;
  v = wmalloc(size,e);
  return v;
}

/**
 *  @brief          wrapper around calloc for garbage collection
 *  @param          num  number of elements to allocate
 *  @param          size size of elements to allocate
 *  @param          e    error output stream
 *  @return         pointer to newly allocated storage on sucess, which
 *                  is zeroed, exits program on failure!
 **/
void *gccalloc(size_t num, size_t size, io *e){
  void* v;
  v = wcalloc(num, size, e);
  return v;
}

/**
 *  @brief          wrapper around free
 *  @param          ptr  pointer to free; make sure its not NULL!
 *  @param          e    error output stream
 *  @return         void
 **/
void wfree(void *ptr, io *e){
  alloccounter--;
  if(NULL == e){ /* *I* should not be passing null to free */
    fprintf(stderr, "free failed and *e == NULL\n");
    exit(EXIT_FAILURE);
  }
  free(ptr);
}

/**
 *  @brief          Given a root structure, mark all accessible
 *                  objects in the tree so they do not get garbage
 *                  collected
 *  @param          root root tree to mark
 *  @param          e    error output stream
 *  @return         false == root was not marked, and now is, 
 *                  true == root was already marked
 **/
int gcmark(expr root, io *e){
  if(NULL == root)
    return;

  if(true == root->gcmark)
    return true;

  root->gcmark = true;

  switch(root->type){
    case S_LIST:
      {
        unsigned int i;
        for(i = 0; i < root->len; i++)
          gcmark(root->data.list[i],e);
      }
      break;
    case S_PRIMITIVE:
      break;
    case S_PROC:
      /*@todo Put the S_PROC structure into type.h**/
      gcmark(root->data.list[0],e);
      gcmark(root->data.list[1],e);
      gcmark(root->data.list[2],e);
    case S_NIL:
    case S_TEE:
    case S_STRING:
    case S_SYMBOL:
    case S_INTEGER:
    case S_FILE:
      break;
    default:
      fprintf(stderr,"unmarkable type\n");
      exit(EXIT_FAILURE);
  }
  return false;
}

/**
 *  @brief          Sweep all unmarked objects.
 *  @param          e    error output stream
 *  @return         void
 **/
void gcsweep(io *e){

}


/*****************************************************************************/
