#include <stdio.h> 
#include <stdlib.h>
#include <string.h> 
#include <fcntl.h> 
#include <sys/stat.h> 
#include <sys/types.h> 
#include <unistd.h>
#include <led_interface.h>

#define _1ms    1000

bool LED_Run(void *object, POSIX_SHM *posix_shm, LED_Interface *led)
{
	int state_current = 0;
	int state_old = -1;

	if(led->Init(object) == false)
		return false;

	if(POSIX_SHM_Init(posix_shm) == false)
		return false;

	while(true)
	{
		sscanf(posix_shm->buffer, "state = %d", &state_current);
		if(state_current != state_old)
		{
			led->Set(object, state_current);
			state_old = state_current;
		}

		usleep(_1ms);
	}

	POSIX_SHM_Cleanup(posix_shm);
	return false;	
}
