#include "stm32f7xx_hal.h"

#include <stdio.h>
#include <string.h>

#include "main.h"

void MacSender(void *argument)
{
	
	struct queueMsg_t message;
	struct queueMsg_t myMessage = {0};
	osStatus_t returnPHY;
	//Read The macR queue
	while(1)
	{
		
		// getting the messages from the queue
		osMessageQueueGet(queue_macS_id,&message,NULL,osWaitForever);
		// if the message is of type TOKEN
		if(message.type == TOKEN)
		{		
				uint8_t* dataptr;
				dataptr = message.anyPtr;
		
				// we need to update the list of connected stations 
				for(int i = 0; i <= 15 ; i++)
				{
					gTokenInterface.station_list[i] = dataptr[i];
					printf("station %d data : 0x%02x \r\n", i+1, dataptr[i]);
				}
				
					// creation of a message
					myMessage.type = TOKEN_LIST;
					
					//updating the que of the LCD
					returnPHY = osMessageQueuePut(queue_lcd_id,&myMessage,NULL,osWaitForever);
					CheckRetCode(returnPHY,__LINE__,__FILE__,CONTINUE);
		}
		if(message.type == START)
		{
			gTokenInterface.connected = true;
		}
		// if the message is of type STOP
		if(message.type == STOP)
		{
			gTokenInterface.connected = false;
		}
		// if the message is of type NEW_TOKEN
		if(message.type == NEW_TOKEN)
		{
			printf("WOHOOOW\r\n");
			// creation of the Token
			uint8_t* ptk = osMemoryPoolAlloc(memPool,osWaitForever);
			if (ptk)
			{
					// putting zeros everywhere
					memset(ptk,0,17);
					// token header
					ptk[0] = TOKEN_TAG;
					// putting the correct value inside the token
					//	time sapi is alwais active	
					ptk[MYADDRESS] = (1 << TIME_SAPI);
					// if we are connected
					if(gTokenInterface.connected)
					{
						// chat sapi must be active
						ptk[MYADDRESS] |= (1 << CHAT_SAPI) ;
					}
					
				
					// creation of a message
					myMessage.type = TO_PHY;
					myMessage.anyPtr = ptk;			
		
					
					returnPHY = osMessageQueuePut(queue_phyS_id,&myMessage,NULL,osWaitForever);
					CheckRetCode(returnPHY,__LINE__,__FILE__,CONTINUE);
			}
		
		}
	}
	

}
