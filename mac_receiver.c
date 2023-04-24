#include "stm32f7xx_hal.h"

#include <stdio.h>
#include <string.h>

#include "main.h"

void MacReceiver(void *argument)
{
	struct queueMsg_t message;
	osStatus_t returnPHY;
	struct queueMsg_t myMessage = {0};
	
	while(1)
	{
		osMessageQueueGet(queue_macR_id,&message,NULL,osWaitForever);
		if(message.type == FROM_PHY)
		{
			uint8_t* dataptr;
			dataptr = message.anyPtr;
			
			if (dataptr[0] == TOKEN_TAG)
			{
				
					// creation of a message
					myMessage.type = TOKEN;
					myMessage.anyPtr = message.anyPtr;			
				
					
					returnPHY = osMessageQueuePut(queue_macS_id,&myMessage,NULL,osWaitForever);
					CheckRetCode(returnPHY,__LINE__,__FILE__,CONTINUE);
			}
		}
		
	}

}



