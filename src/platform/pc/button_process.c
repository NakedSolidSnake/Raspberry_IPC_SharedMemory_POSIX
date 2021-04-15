#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h> 
#include <button_interface.h>
#include <sys/stat.h> 
#include <sys/types.h>

#define BUFFER_SIZE     256

static bool Init(void *object);
static bool Read(void *object);
static const char * myfifo = "/tmp/shared_memory_posix_fifo";

static int fd;

int main(int argc, char *argv[])
{    
    Button_Interface button_interface = 
    {
        .Init = Init,
        .Read = Read
    };

    POSIX_SHM posix_shm = 
    {
        .buffer_size = BUFFER_SIZE,
        .mode = write_mode,
        .name = "/shm"
    };

    Button_Run(NULL, &posix_shm, &button_interface);
        
    return 0;
}

static bool Init(void *object)
{    
    (void)object;
    remove(myfifo);
    int ret = mkfifo(myfifo, 0666);
    return (ret == -1 ? false : true);
}

static bool Read(void *object)
{
    (void)object;
    int state;
    char buffer[2];

    fd = open(myfifo,O_RDONLY);
    read(fd, buffer, 2);	
    state = atoi(buffer);
    return state ? true : false;
}