#include <posix_shm.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <malloc.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>

bool POSIX_SHM_Init(POSIX_SHM *posix_shm)
{
    int mode;
    posix_shm->fd = shm_open(posix_shm->name, O_RDWR | O_CREAT, S_IRWXU | S_IRWXG | S_IRWXO);
    if(posix_shm->fd < 0)
        return false;

    if(ftruncate(posix_shm->fd , posix_shm->buffer_size) < 0)
        return false;
    

    if(posix_shm->mode == write_mode)
        mode = PROT_WRITE;
    else if(posix_shm->mode == read_mode)
        mode = PROT_READ;

    posix_shm->buffer = mmap(NULL, posix_shm->buffer_size, mode, MAP_SHARED, posix_shm->fd, 0);
    if(posix_shm->buffer < 0)
        return false;

    return true;
}

bool POSIX_SHM_Cleanup(POSIX_SHM *posix_shm)
{
    if(posix_shm->buffer > 0)
        munmap(posix_shm->buffer, posix_shm->buffer_size);
    if(posix_shm->fd > 0)
        shm_unlink(posix_shm->name);

    return true;
}