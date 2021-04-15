#include <stdio.h>
#include <stdlib.h>
#include <string.h> 
#include <fcntl.h> 
#include <sys/stat.h> 
#include <sys/types.h> 
#include <unistd.h>
#include <button_interface.h>

#define _1ms    1000

bool Button_Run(void *object, POSIX_SHM *posix_shm, Button_Interface *button)
{
    int state = 0;
    if(button->Init(object) == false)
        return false;

    if(POSIX_SHM_Init(posix_shm) == false)
        return false;

    while(true)
    {
        while (true)
        {
            if (!button->Read(object))
            {
                usleep(_1ms * 100);
                break;
            }
            else
            {
                usleep(_1ms);
            }
        }

        state ^= 0x01;
        snprintf(posix_shm->buffer, posix_shm->buffer_size, "state = %d", state);
    }

    POSIX_SHM_Cleanup(posix_shm);
   
    return false;
}
