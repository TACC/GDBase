#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

ssize_t getline(char ** lineptr, size_t *n, FILE* fp)
{
  size_t result;
  size_t len;
  size_t cur_len = 0;
  size_t size_left;
  size_t oldsize;
  size_t plen;
  char * ptr;

  if (lineptr == NULL || n == NULL || fp == NULL)
    return -1;

  if (*lineptr == NULL || *n == 0)
    {
      *n = 120;
      *lineptr = (char *) malloc(*n);
      if (*lineptr == NULL)
        return -1;
    }

  size_left = *n;
  ptr       = *lineptr;
  result    = -1;

  while(1)
    {
      char* eof = fgets(ptr, size_left, fp);
      if (eof == NULL)
        return result;

      plen   = strlen(ptr);
      result = strlen(*lineptr);

      if (ptr[plen] == '\n')
        return result;

      oldsize  = *n;
      *n      *= 2;
      *lineptr = (char *) realloc(*lineptr, *n);
      if (*lineptr == NULL)
        return -1;

      size_left = *n - oldsize + 1;
      ptr       = *lineptr + oldsize - 1;

    }
}

#if 0
int main()
{
  FILE *fp;
  char *buffer;
  size_t  sz = 4;

  buffer = (char *) malloc(sz);

  fp = fopen("foo.txt","r");
  getline(&buffer, &sz, fp);

  printf("buffer: %s\n",buffer);
}
#endif
    
