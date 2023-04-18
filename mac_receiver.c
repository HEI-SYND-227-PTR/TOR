#include "stm32f7xx_hal.h"

#include <stdio.h>
#include <string.h>

#include "main.h"

void MacReceiver(void *argument)
{
	struct queueMsg_t message;
	
	while(1)
	{
		osMessageQueueGet(queue_macR_id,&message,NULL,osWaitForever);
		if(message.type == FROM_PHY)
		{
			printf("WAHAAAAAA\r\n");
		}
		
	}

}
