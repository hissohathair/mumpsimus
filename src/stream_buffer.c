/*
 * stream_buffer.c: Provides a simple stream buffer for buffering output.
 */

#include "config.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "ulog.h"
#include "util.h"

#include "stream_buffer.h"


/*
 * stream_buffer_new(): Creates a new stream buffer object and
 * allocates internal structures.
 */
struct Stream_Buffer* stream_buffer_new()
{
  struct Stream_Buffer *buf = malloc(sizeof(struct Stream_Buffer));
  buf->head = malloc(sizeof(char) * BUFFER_MAX);
  buf->tail = buf->head;
  buf->curr_size = 0;
  buf->max_size  = BUFFER_MAX;
  ulog_debug("Created new buffer at %X that is %ld bytes big", buf, buf->max_size);
  return buf;
}

/*
 * stream_buffer_delete: Deallocates internal structures and then finally the
 * passed in object itself. Caller does not need to call free() on anything.
 */
void stream_buffer_delete(struct Stream_Buffer *buf)
{
  ulog_debug("Freeing buffers");
  free(buf->head);
  free(buf);
  return;
}


/* 
 * stream_buffer: Adds to the stream buffer, and returns the number of bytes added.
 * This should either be equal to 'length' or 0 -- the function does not add partial
 * buffer content.
 */
size_t stream_buffer(struct Stream_Buffer *buf, const char *nbuff, const size_t length)
{
  assert(buf != NULL);
  assert(nbuff != NULL);
  assert(buf->head != NULL );

  /* Check for buffer overflow with current memory allocation */
  if ( buf->curr_size + length > buf->max_size ) {
    size_t new_size = upper_power_of_two((buf->max_size + length) * 2);

    ulog_debug("Reallocating buffer from %ld to %ld bytes", buf->max_size, new_size);
    char *new_buff = realloc(buf->head, new_size);
    if ( NULL == new_buff ) {
      perror("Out of memory in realloc -- buffer overflow");
      return 0;
    } 
    else {
      /* realloc can move the cheese so better keep up */
      buf->head = new_buff;
      buf->tail = buf->head + buf->curr_size;
      buf->max_size = new_size;
    }
  }

  //ulog_debug("Appending %ld bytes to end of buffer at %X (current size=%ld)", length, buf->head, buf->curr_size);
  memcpy(buf->tail, nbuff, length);
  buf->tail += length;
  buf->curr_size += length;  

  assert(buf->tail - buf->head == buf->curr_size);
  assert(buf->curr_size <= buf->max_size);

  return length;
}

/*
 * stream_buffer_clear: "Empties" the buffer, but resetting its
 * contents to empty state.
 */
void stream_buffer_clear(struct Stream_Buffer *buf)
{
  assert(buf != NULL);
  buf->tail = buf->head;
  buf->curr_size = 0;
  return;
}

size_t stream_buffer_size(struct Stream_Buffer *buf) 
{
  return buf->curr_size;
}


/*
 * stream_buffer_write: Dumps the contents of the buffer onto file
 * descriptor 'fd'.  This also "clears" the buffer.
 */
size_t stream_buffer_write(struct Stream_Buffer *buf, int fd)
{
  assert(buf != NULL);
  assert(buf->head != NULL);
  ssize_t nw = write_all(fd, buf->head, buf->curr_size);
  stream_buffer_clear(buf);
  return nw;
}


