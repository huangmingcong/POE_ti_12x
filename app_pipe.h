
#ifndef _APP_PIPE_H_
#define _APP_PIPE_H_

#include <string.h>
#include <stdio.h>
#include<unistd.h> 
 
#define PIPE_NAME_LEN (128)

typedef struct {
    char name[PIPE_NAME_LEN];
    void *user;

    int readfd;
    int writefd;
    int size;
    int inited;
} pipe_obj_t;

pipe_obj_t *pipe_init(pipe_obj_t *pipe_obj, char *name, int size, void *usr);

int pipe_write(pipe_obj_t *pipe, void *buf, int size);

int pipe_read(pipe_obj_t *pipe, void *buf, int size);

int pipe_get_readfd(pipe_obj_t *pipe);

#endif