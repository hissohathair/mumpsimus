/* stream_buffer.h
 *
 *    Provides a simple stream buffer for buffering output. Callers
 *    can append data and the buffer will attempt to grow
 *    automatically.
 */
#ifndef __STREAM_BUFFER_H__
#define __STREAM_BUFFER_H__

#include <sys/types.h>

/* Stream_Buffer:
 *
 *    A struct to hold data for stream buffers. All members should be treated
 *    as 'private'. 
 */
struct Stream_Buffer
{
  // private
  char *head;			// Start of unwritten data portion
  char *tail;			// End of unwritten data portion (append here)
  size_t curr_size;		// Current data size in buffer
  size_t max_size;		// Max allocated to buffer
};

struct Stream_Buffer *stream_buffer_new (void);
void stream_buffer_delete (struct Stream_Buffer *buf);

size_t stream_buffer_add (struct Stream_Buffer *buf, const char *nbuff,
			  const size_t length);

void stream_buffer_clear (struct Stream_Buffer *buf);
size_t stream_buffer_write (struct Stream_Buffer *buf, int fd);
size_t stream_buffer_write_to (struct Stream_Buffer *buf, int fd,
			       size_t length);

size_t stream_buffer_size (struct Stream_Buffer *buf);

#endif
