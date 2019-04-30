#include "app_pipe.h"

pipe_obj_t *pipe_init(pipe_obj_t *pipe_obj, char *name, int size, void *usr)
{
    int ret;
    int fd[2];

    if (!pipe_obj) {
        return NULL;
    }

    ret = pipe(fd);
    if (ret != 0) {
        printf("get pipe error\n");
        return NULL;
    }

    memset(pipe_obj, 0, sizeof(*pipe_obj));

    strncpy(pipe_obj->name, name, PIPE_NAME_LEN);
    pipe_obj->readfd = fd[0];
    pipe_obj->writefd = fd[1];
    pipe_obj->user = usr;
    pipe_obj->size = size;

    return pipe_obj;
}

int pipe_write(pipe_obj_t *pipe, void *buf, int size)
{
    int ret;

    if (pipe == NULL || buf == NULL) {
        return -1;
    }
    //size = 0
    if (size != pipe->size) {
        printf("input param is error, size = %d.\n", size);
        return -1;
    }

    ret = write(pipe->writefd, buf, size);
    if (ret == -1) {
        printf("pipe write error.\n");
        return -1;
    }

    return 0;
}

int pipe_read(pipe_obj_t *pipe, void *buf, int size)
{
    int ret;

    if (pipe == NULL || buf == NULL) {
        printf("input para error\n");
        return -1;
    }
    if (size != pipe->size) {
        printf("input param is error, size = %d.\n", size);
        return -1;
    }

    ret = read(pipe->readfd, buf, size);
    if (ret == -1) {
        printf("pipe read error.\n");
        return -1;
    }

    return 0;
}

int pipe_get_readfd(pipe_obj_t *pipe)
{
    return pipe->readfd;
}

