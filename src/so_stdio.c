#include "so_stdio.h"

#define BUFSIZE 4096

struct _so_file {
	char buffer[BUFSIZE];
	int fd;
	int curr;
	int offset;
	int bytes_read;
	int eof;
	int write;
	int read;
	int error;
};

SO_FILE *so_fopen(const char *pathname, const char *mode)
{
	int fd = -1;
	SO_FILE *file;

	if (!strcmp(mode, "r"))
		fd = open(pathname, O_RDONLY);
	else if (!strcmp(mode, "r+"))
		fd = open(pathname, O_RDWR);
	else if (!strcmp(mode, "w"))
		fd = open(pathname, O_WRONLY | O_CREAT | O_TRUNC);
	else if (!strcmp(mode, "w+"))
		fd = open(pathname, O_RDWR | O_CREAT | O_TRUNC);
	else if (!strcmp(mode, "a"))
		fd = open(pathname, O_APPEND | O_WRONLY | O_CREAT);
	else if (!strcmp(mode, "a+"))
		fd = open(pathname, O_APPEND | O_RDWR | O_CREAT);

	if (fd < 0)
		return NULL;

	file = malloc(sizeof(SO_FILE));
	file->fd = fd;
	file->curr = 0;
	file->offset = 0;
	file->bytes_read = 0;
	file->eof = 0;
	file->write = 0;
	file->read = 0;
	file->error = 0;

	return file;
}

int so_fclose(SO_FILE *stream)
{
	int fflush_ret = so_fflush(stream);
    int close_ret = close(stream->fd);

	free(stream);
	if (close_ret == 0 && fflush_ret == 0)
		return 0;

	return SO_EOF;
}

int so_fileno(SO_FILE *stream)
{
	return stream->fd;
}

int so_fflush(SO_FILE *stream)
{
	if (stream->read == 1 || stream->curr == 0) {
		stream->read = 0;
		return 0;
	} else if (stream->curr != 0) {
		int ret = write(stream->fd, stream->buffer, stream->curr);

		while (ret < stream->curr) {
			if (ret <= 0) {
				stream->error = 1;
				return SO_EOF;
			}
			int aux = write(stream->fd, stream->buffer + ret, stream->curr - ret);

			ret += aux;
		}
		stream->curr = 0;
		return 0;
	}
	return 0;
}

int so_fseek(SO_FILE *stream, long offset, int whence)
{
	if (whence == SEEK_CUR && stream->read)
		offset -= stream->bytes_read - stream->curr;

	if (stream->write)
		so_fflush(stream);
	else if (stream->read) {
		stream->curr = 0;
		stream->bytes_read = 0;
		stream->read = 0;
	}

	stream->offset = lseek(stream->fd, offset, whence);
	if (stream->offset < 0) {
		stream->error = 1;
		return -1;
	}

	return 0;
}

long so_ftell(SO_FILE *stream)
{
	return (so_fseek(stream, 0, SEEK_CUR) == -1) ? -1 : stream->offset;
}

size_t so_fread(void *ptr, size_t size, size_t nmemb, SO_FILE *stream)
{
	stream->read = 1;
	for (size_t i = 0; i < nmemb * size; i++) {
		unsigned char c = (unsigned char) so_fgetc(stream);

		if (so_feof(stream) || so_ferror(stream))
			return i / size;

		memcpy(((unsigned char *) ptr + i), &c, 1);
	}
	return nmemb;
}

size_t so_fwrite(const void *ptr, size_t size, size_t nmemb, SO_FILE *stream)
{
	stream->write = 1;

	for (size_t i = 0; i < nmemb * size; i++) {
		int ret = so_fputc(*((unsigned char *) ptr + i), stream);

		if (ret == SO_EOF)
			return i / size;
		if (so_ferror(stream) || so_feof(stream))
			return 0;
	}
	return nmemb;
}

int so_fgetc(SO_FILE *stream)
{
	int ret = 0;

	stream->read = 1;

	if (stream->curr == stream->bytes_read) {
		ret = read(stream->fd, stream->buffer, BUFSIZE);
		stream->bytes_read = ret;
		stream->curr = 0;

		if (ret < 0) {
			stream->error = 1;
			return SO_EOF;
		} else if (ret == 0) {
			stream->eof = SO_EOF;
			return SO_EOF;
		}
	}

	ret = stream->buffer[stream->curr];
	stream->curr++;

	return ret;
}

int so_fputc(int c, SO_FILE *stream)
{
	stream->write = 1;

	if (stream->curr == BUFSIZE && so_fflush(stream) == SO_EOF)
		return SO_EOF;

	stream->buffer[stream->curr] = c;
	stream->curr++;
	return c;
}

int so_feof(SO_FILE *stream)
{
	return stream->eof;
}

int so_ferror(SO_FILE *stream)
{
	return stream->error;
}

SO_FILE *so_popen(const char *command, const char *type)
{
	return NULL;
}

int so_pclose(SO_FILE *stream)
{
	return 0;
}
