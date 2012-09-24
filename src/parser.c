/* This file is part of GDBase.
 *
 * GDBase is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * GDBase is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GDBase.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright 2009 Anthony DiGirolamo, Dan Stanzione, Karl Lindekugel
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser.h"
#include "vector.h"

#define DELIMITERS " \t\n"
#define malloc_check(var) if(var==NULL){fprintf(stderr,"malloc failed\n");exit(EXIT_FAILURE);}
#define FALSE 0
#define TRUE 1

#define PARSER_PREFIX "prefix"
#define TIMEOUT "timeout"
#define GDB "gdb"

int parsed = FALSE;
Vector *keys;
Vector *values;

char* get_prefix()
{
  if (!parsed)
    return NULL;
  else
    {
      char *return_string;
      return_string = (char*) malloc(8192 * sizeof(char));
      malloc_check(return_string);
      
      if (vector_findString(keys, PARSER_PREFIX) > -1)
        {
          sprintf(return_string, "%s", (char*) vector_getItem(values, vector_findString(keys, PARSER_PREFIX)));
          return return_string;
        }
      else
        return NULL;
    }
}

char* get_timeout()
{
  if (!parsed)
    return NULL;
  else
    {
      char *return_string;
      return_string = (char*) malloc(8192 * sizeof(char));
      malloc_check(return_string);

    if (vector_findString(keys, TIMEOUT) > -1)
      {
        sprintf(return_string, "%s", (char*) vector_getItem(values, vector_findString(keys, TIMEOUT)));
        return return_string;
      }
    else
      return NULL;
    }
}

char* get_gdb()
{
  if (!parsed)
    return NULL;
  else
    {
      char *return_string;
      return_string = (char*) malloc(8192 * sizeof(char));
      malloc_check(return_string);

      if (vector_findString(keys, GDB) > -1)
        {
          sprintf(return_string, "%s", (char*) vector_getItem(values, vector_findString(keys, GDB)));
          return return_string;
        }
      else
        return NULL;
    }
}

char* get_connectdb()
{
  if (!parsed)
    return NULL;
  else
    {
      char *return_string;
      return_string = (char*) malloc(1024 * sizeof(char));
      malloc_check(return_string);

      if (vector_findString(keys, "database.name") > -1)
        {
          sprintf(return_string,
                  " dbname=%s host=%s port=%s user=%s password=%s ",
                  (char*) vector_getItem(values, vector_findString(keys, "database.name")),
                  (char*) vector_getItem(values, vector_findString(keys, "database.host")),
                  (char*) vector_getItem(values, vector_findString(keys, "database.port")),
                  (char*) vector_getItem(values, vector_findString(keys, "database.user")),
                  (char*) vector_getItem(values, vector_findString(keys, "database.password"))
                  );
          return return_string;
        }
      else
        return NULL;
    }
}

int parse (char* file_name)
{
  FILE *file_pointer;
  int line_size = 4096;
  int line_length = 0;
  int token_length = 0;
  int prepend_length = 0;
  char *token;
  char *prepend = NULL;
  char *temp1;
  char *temp2;

  keys = vector_new();
  values = vector_new();

  // malloc the buffer, necessary for getline
  char *buffer;
  buffer = (char *) malloc(line_size * sizeof(char));
  malloc_check(buffer);

  // Open the file
  if ((file_pointer = fopen(file_name, "r")) == NULL )
    {
      fprintf(stderr, "can't open %s\n", file_name);
      return EXIT_FAILURE;
    }

  // Begin Reading the File
  int is_a_key;
  int saved_key_value;
  while (line_length > -1)
    { // while not EOF
      is_a_key = FALSE;
      saved_key_value = FALSE;
      line_length = getline(&buffer, &line_size, file_pointer);

      if (line_length > 1)
        {
          // if not an empty line
          token = strtok(buffer, DELIMITERS);
          if (token[0] != 35)
            { // if first token doesn't start with #
              while (token != NULL)
                {
                  token_length = strlen(token);
                  if (token[token_length-1] == 58)
                    {
                      // we have a key
                      token[token_length-1] = '\0'; // erase the last ':'
                      is_a_key = TRUE;
                      if (prepend == NULL) 
                        {
                          temp1 = (char *) malloc(token_length * sizeof(char));
                          malloc_check(temp1);
                          strcpy(temp1, token);
                        }
                      else
                        { // prepend the key name
                          temp1 = (char *) malloc((token_length+strlen(prepend)) * sizeof(char));
                          malloc_check(temp1);
                          strcpy(temp1, prepend);
                          strcat(temp1, token);
                        }
                    }
                  token = strtok(NULL, DELIMITERS);
                  if (token != NULL) 
                    { // if there is another token
                      if (is_a_key)
                        { // if the previous token was a key save the pair
                          token_length = strlen(token);
                          temp2 = (char *) malloc(token_length * sizeof(char));
                          malloc_check(temp2);
                          strcpy(temp2, token);
                          vector_addItem(keys, temp1);
                          vector_addItem(values, temp2);
                          saved_key_value = TRUE;
                        }
                      else
                        {
                          free(temp1);
                          break; // don't need the rest of the line
                        }
                    }
                  if (!saved_key_value)
                    {
                      // we have a prepend
                      if (prepend == NULL)
                        prepend = (char *) malloc((token_length+1) * sizeof(char));
                      else
                        {
                          prepend_length = strlen(prepend);
                          prepend = (char *) realloc(prepend, (token_length+prepend_length+1) * sizeof(char));
                        }
                      malloc_check(prepend);
                      strcpy(prepend, temp1);
                      prepend_length = strlen(prepend);
                      prepend[strlen(prepend)] = '.';  // add a .
                      prepend[strlen(prepend)] = '\0';
                    }
                }
            }
        }
      else
        { // if this is an empty line
          if (prepend != NULL)
            {
              free(prepend);
              prepend = NULL;
            }
        }
    } // while !EOF

  // Close and Exit
  fclose(file_pointer);

  //int i;
  //for (i=0; i<vector_size(keys); i++) {
  //	printf("V(%i)=\"%s\",\"%s\"\n", i,
  //		(char*) vector_getItem(keys, i),
  //		(char*) vector_getItem(values, i));
  //}

  parsed = TRUE;
  return EXIT_SUCCESS;
}

// int main (int argc, char const* argv[]) {
// 	parse((char*)argv[1]);
//
// 	int i;
// 	for (i=0; i<vector_size(keys); i++) {
// 		printf("V(%i)=\"%s\",\"%s\"\n", i,
// 			(char*) vector_getItem(keys, i),
// 			(char*) vector_getItem(values, i));
// 	}
//
// 	printf("\"%s\"\n", get_connectdb() ? get_connectdb() : "error" );
// 	printf("\"%s\"\n", get_timeout() ? get_timeout() : "error" );
// 	printf("\"%s\"\n", get_prefix() ? get_prefix() : "error" );
// 	printf("\"%s\"\n", get_gdb() ? get_gdb() : "error" );
//
// 	return EXIT_SUCCESS;
// }
