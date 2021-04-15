#ifndef POSIX_SHM_H_
#define POSIX_SHM_H_

#include <stdbool.h>

typedef enum 
{
    write_mode,
    read_mode
} Mode;

typedef struct 
{
    int fd;
    const char *name;
    char *buffer;
    int buffer_size;
    Mode mode;
} POSIX_SHM;

bool POSIX_SHM_Init(POSIX_SHM *posix_shm);
bool POSIX_SHM_Cleanup(POSIX_SHM *posix_shm);

#endif /* POSIX_SHM_H_ */
