/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2016, Alexey Kramarenko
    All rights reserved.
*/

#ifndef SO_H
#define SO_H

#include "array.h"

//defined public for less fragmentaion
typedef struct _SO {
    ARRAY* ar;
    unsigned int first_free;
} SO;

SO* so_create(SO* so, unsigned int data_size, unsigned int reserved);
void so_destroy(SO* so);
//Remember: always re-fetch objects after so_allocate
HANDLE so_allocate(SO* so);
bool so_check_handle(SO* so, HANDLE handle);
void so_free(SO* so, HANDLE handle);
void* so_get(SO* so, HANDLE handle);
HANDLE so_first(SO* so);
HANDLE so_next(SO* so, HANDLE prev);
unsigned int so_count(SO* so);

#endif // SO_H
