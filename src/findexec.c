#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "findexec.h"
/*
 *  Find absolute path of a command by searching the path
 *  Return -1 for error
 *          0 for not found
 *          1 for found
 */



int find_exec_in_path(const char* cmd, int* plen, char ** abspath)
{
  int   abslen = 0;
  int   found  = 0;
  int   sz     = 10;
  int   myStat;
  int   cmdlen;
  char* path;
  char* buffer;
  struct stat sb;

  if (abspath == NULL || plen == NULL || cmd == NULL || *cmd == '\0')
    return -1;

  cmdlen = strlen(cmd);
  path   = getenv("PATH");
  
  /* Use buffer to hold path and cmd */

  buffer = (char *) malloc(sz);

  while(*path)
    {

      /* Find next path element */

      int  len;
      char *p = strchr(path,':');
      if (p == NULL)
        p = path + strlen(path);
          
      len     = p - path;
      
      if (len < 1) continue;

      /* Copy path element and cmd into buffer */

      if (len + 2 + cmdlen > sz)
        {
          free(buffer);
          sz = len+ 2 + cmdlen;
          buffer = (char *) malloc(sz);
        }

      memcpy(buffer, path, len);
      buffer[len] = '/';
      memcpy(&buffer[len+1], cmd, cmdlen);
      abslen = len + 1 + cmdlen;
      buffer[abslen] = '\0';

      /* Check to see if filename in buffer exists and is executable
       * Break if found
       */
      
      myStat = stat(buffer, &sb);
      if (myStat == 0 && (sb.st_mode & S_IXUSR))
        {
          found = 1;
          break;
        }
      
      /* Move path variable to next path element */
      path    += len;
      if (*path == ':')
        path++;
    }

  /* Copy result into users's abspath variable */

  if (found == 1)
    {
      if (*plen < abslen+1)
        {
          *plen    = abslen+1;
          *abspath = (char*) realloc(*abspath, *plen);
        }
      memcpy(*abspath, buffer, abslen+1);
    }

  free(buffer);

  return found;
}

int find_gdbase_prefix(const char * cmd, int* sz, char ** prefix)
{
  char        buf1[4096];
  char        buf2[4096];
  char        *q, *p;
  int         myStat, len, i;
  int         szbuf       = 4096;
  int         cmdlen      = strlen(cmd);
  struct stat sb;
  
  p = &buf1[0];
  q = &buf2[0];  

  find_exec_in_path(cmd, sz, prefix);

  strcpy(p, *prefix);


  while (1)
    {
      myStat = lstat(p, &sb);
      if (myStat == 0 && ! S_ISLNK(sb.st_mode))
        {
          len = strlen(p);
          if (*sz < len + 1)
            {
              *sz     = len + 1;
              *prefix = realloc(*prefix, *sz);
            }
          memcpy(*prefix, p, len+1);
          break;
        }
      readlink(p, q, szbuf);
      p = q;
    }

  i   = 0;
  len = strlen(*prefix);
  memcpy(&p[0], "/bin/",5);      i += 5;
  memcpy(&p[i], cmd,    cmdlen); i += cmdlen;
  p[i] = '\0';

  if (strcmp(&(*prefix)[len-i], p) == 0)
    {
      (*prefix)[len-i] = '\0';
      return 1;
    }
  (*prefix)[0] = '\0';
  return -1;

}
  


#if 0
int main(int argc, char* argv[])
{
  
  int   sz        = 10;
  const char* cmd = "opd";
  char* path      = (char *) malloc(sz);

  int result = find_gdbase_prefix(cmd, &sz, &path);

  printf ("abspath: %s\n", path);

  return 0;
}
#endif
