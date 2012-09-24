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

/** Expanding Vector Library
*** Karl Lindekugel, ASU
**/

#include "vector.h"
#include <stdlib.h>

void vector_resize(Vector *v);

Vector* vector_new()
{
  Vector* v;
  v = (Vector*) malloc(sizeof(Vector));
  
  v->cap = 0;
  v->size = 0;

  v->data = NULL;

  return v;
}

int vector_size(Vector *v)
{
  if (v == NULL) return 0;
  return v->size;
}

int vector_capacity(Vector *v)
{
  if (v == NULL) return 0;
  return v->cap;
}

void* vector_getItem(Vector *v, int index)
{
  if (v == NULL)       return NULL;
  if (index > v->size) return NULL;
  if (index < 0)       return NULL;
  
  return v->data[index];
}

int vector_setItem(Vector *v, int index, void *item)
{
  if (v == NULL)       return 1;
  if (index > v->size) return 1;

  v->data[index] = item;

  return 0;
}

void vector_resize(Vector *v)
{
  //Expand v
  
  v->data = realloc(v->data, sizeof(void*) * (v->cap*2 +1) );
  v->cap = v->cap*2+1;
}

int vector_addItem(Vector *v, void *item)
{
  if (v == NULL) return -1;
  if (v->size >= v->cap) vector_resize(v);

  v->data[v->size++] = item;

  return v->size;
}

int vector_findItem(Vector *v, void *item)
{
  int i;
  
  if (v == NULL) return -1;

  for (i = 0; i < v->size; i++)
    if (v->data[i] == item) break;
  
  if (v->size == i) return -1; // Not found
  return i;		    // Found at i
}

int vector_findString(Vector *v, void *item)
{
  int i;

  if (v == NULL) return -1;

  for (i = 0; i < v->size; i++)
    {
      if ( strcasecmp((char*)v->data[i], (char*)item) == 0 ) break;
    }

  if (v->size == i) return -1; // Not found
  return i;		    // Found at i
}

