/*
 * stream_buffer.h
 */
#ifndef __BUFFINATOR_H__
#define __BUFFINATOR_H__

#include <sys/types.h>

struct Stream_Buffer {
  char *head;
  char *tail;
  size_t curr_size;
  size_t max_size;
};

struct Stream_Buffer* stream_buffer_new();
void stream_buffer_delete(struct Stream_Buffer *buf);
size_t stream_buffer(struct Stream_Buffer *buf, const char *nbuff, const size_t length);
void stream_buffer_clear(struct Stream_Buffer *buf);
size_t stream_buffer_write(struct Stream_Buffer *buf, int fd);

#endif
