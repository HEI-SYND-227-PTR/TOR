#include "stm32f7xx_hal.h"

#include <stdio.h>
#include <string.h>

#include "main.h"

void MacSender(void *argument)
{
	uint8_t* memory ;
	struct queueMsg_t message;
	struct queueMsg_t myMessage = {0};
	osStatus_t returnPHY;
	
	//Read The macR queue
	while(1)
	{
		
		// getting the messages from the queue
		osMessageQueueGet(queue_macS_id,&message,NULL,osWaitForever);
		// if the message is of type DATA_IND
		if(message.type == DATABACK)
		{		
				DataFrame f = fromByteArrayToStruct(message.anyPtr);
				if(f.s.status_field.ack != 1 || f.s.status_field.read != 1)
				{
					// there is a problem we need to re send
					
					myMessage.type = TO_PHY;
					myMessage.anyPtr = memory;
	
			
					returnPHY = osMessageQueuePut(queue_phyS_id,&myMessage,NULL,osWaitForever);
					CheckRetCode(returnPHY,__LINE__,__FILE__,CONTINUE);	
				}
				else
				{
					// we can forget the message 
					osMemoryPoolFree(memPool,memory);
				}
	
					
			
		}
		if(message.type == DATA_IND)
		{			
				DataFrame f;
				
			
				f.c.control_field.ssap = gTokenInterface.station_list[MYADDRESS];
				f.c.control_field.saddr = MYADDRESS;
				f.c.control_field.dsap = message.sapi;
				f.c.control_field.daddr = message.addr;
				f.dataPtr = message.anyPtr;
				f.length = sizeof(*((uint8_t*) message.anyPtr));			
				
				uint8_t* ptr;
				fromStructToByteArray(f, ptr);
			
				memory = osMemoryPoolAlloc(memPool,osWaitForever);
				memcpy(memory , &ptr ,f.length+4);
			
				myMessage.type = TO_PHY;
				myMessage.anyPtr = memory;
	
			
			
				returnPHY = osMessageQueuePut(queue_phyS_id,&myMessage,NULL,osWaitForever);
				CheckRetCode(returnPHY,__LINE__,__FILE__,CONTINUE);	
				
		}
		if(message.type == TOKEN) 
		{		
				uint8_t* dataptr;
				dataptr = message.anyPtr;
				
				// we need to update the list of connected stations 
				for(int i = 0; i <= 15 ; i++)
				{
					gTokenInterface.station_list[i] = dataptr[i+1];
//					printf("station %d data : 0x%02x \r\n", i+1, dataptr[i]);
				}
				
					
					// putting zeros everywhere
					uint8_t* ptk;
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
					myMessage.type = TOKEN_LIST;
					myMessage.anyPtr = ptk;
					
					//updating the que of the LCD
					returnPHY = osMessageQueuePut(queue_lcd_id,&myMessage,NULL,osWaitForever);
					CheckRetCode(returnPHY,__LINE__,__FILE__,CONTINUE);
				
					struct queueMsg_t Buffer;
			
					// Trying to read the Buffer queue 
					osStatus_t  bufferStatus =	osMessageQueueGet(queue_macB_id,&Buffer,NULL,0);
					// if message queue is empty 
					if (bufferStatus == osOK)
					{
						// sending the messages inside of the buffer 
						returnPHY = osMessageQueuePut(queue_phyS_id,&Buffer,NULL,osWaitForever);
						CheckRetCode(returnPHY,__LINE__,__FILE__,CONTINUE);
					}
					else if(bufferStatus == osErrorResource) // the que is empty
					{
						// sending back the token
						returnPHY = osMessageQueuePut(queue_phyS_id,&message,NULL,osWaitForever);
						CheckRetCode(returnPHY,__LINE__,__FILE__,CONTINUE);
					}
			
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
