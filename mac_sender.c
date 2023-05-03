#include "stm32f7xx_hal.h"

#include <stdio.h>
#include <string.h>

#include "main.h"

void MacSender(void *argument)
{
	uint8_t* memory;
	struct queueMsg_t message;
	struct queueMsg_t myMessage = {0};
	osStatus_t returnPHY;
	DataFrame f;
	uint8_t ttl = 0;
	uint8_t * original;
	uint8_t * copy;
	uint8_t * token;
	//Read The macR queue
	while(1)
	{
		
		// getting the messages from the queue
		osMessageQueueGet(queue_macS_id,&message,NULL,osWaitForever);
		// if the message is of type DATA_IND
		if(message.type == DATABACK)
		{		
				fromByteArrayToStruct(message.anyPtr, &f);
				if((f.s.status_field.ack != 1 || f.s.status_field.read != 1) && ttl < 4)
				{
					ttl++;
					// there is a problem we need to re send					
				
					osMemoryPoolFree(memPool,message.anyPtr);
					copy = osMemoryPoolAlloc(memPool,osWaitForever);
   				memcpy(copy,original,80);
					myMessage.anyPtr = copy;			
					returnPHY = osMessageQueuePut(queue_phyS_id,&myMessage,NULL,0);
					if(returnPHY != osOK)
					{
						osMemoryPoolFree(memPool,copy);
					}
					CheckRetCode(returnPHY,__LINE__,__FILE__,CONTINUE);	
				}
				else
				{
					if(ttl >= 4)
					{
						myMessage.type = MAC_ERROR;
						myMessage.anyPtr = message.anyPtr;			
						myMessage.addr = message.addr;
						
						returnPHY = osMessageQueuePut(queue_lcd_id,&myMessage,NULL,0);
						if(returnPHY != osOK)
						{
							osMemoryPoolFree(memPool,message.anyPtr);
						}
						CheckRetCode(returnPHY,__LINE__,__FILE__,CONTINUE);
					}
					else
					{
							osMemoryPoolFree(memPool,message.anyPtr);
					}							
					// we can forget the message 
					osMemoryPoolFree(memPool,original);	
					
					// creation of a message
					myMessage.type = TO_PHY;
					myMessage.anyPtr = token;				
					
					returnPHY = osMessageQueuePut(queue_phyS_id,&myMessage,NULL,osWaitForever);
					CheckRetCode(returnPHY,__LINE__,__FILE__,CONTINUE);

					ttl = 0;					
				}
		}
		if(message.type == DATA_IND)
		{				
				f.c.control_field.ssap = (gTokenInterface.station_list[MYADDRESS]>>1);
				f.c.control_field.saddr = MYADDRESS;
				f.c.control_field.dsap = message.sapi;
				f.c.control_field.daddr = message.addr;
				f.dataPtr = message.anyPtr;
				f.length = strlen(message.anyPtr);	
			
				if(f.c.control_field.daddr == BROADCAST_ADDRESS)
				{
						f.s.status_field.ack = 1;
						f.s.status_field.read = 1;
				}
				else
				{
					f.s.status_field.ack = 0;
					f.s.status_field.read = 0;
				}	
				
			  memory = osMemoryPoolAlloc(memPool,osWaitForever);

				myMessage.type = TO_PHY;
				fromStructToByteArray(f, memory);
				myMessage.anyPtr = memory;
			
				returnPHY = osMessageQueuePut(queue_macB_id,&myMessage,NULL,0);
				if(returnPHY != osOK){
					osMemoryPoolFree(memPool,memory);
				}
				
					
				osMemoryPoolFree(memPool,message.anyPtr);
		}
		if(message.type == TOKEN) 
		{		
				// we need to update the list of connected stations 
				for(int i = 0; i <= 14 ; i++)
				{
					gTokenInterface.station_list[i] = ((uint8_t *)message.anyPtr)[i+1];
					//printf("station %d data : 0x%02x \r\n", i+1, ((uint8_t *)message.anyPtr)[i]);
				}
				token = message.anyPtr;					
			
					if(gTokenInterface.connected)
					{
						// chat sapi must be active
						((uint8_t *)message.anyPtr)[MYADDRESS+1] |= (1 << CHAT_SAPI) ;
					}
					// creation of a message
					myMessage.type = TOKEN_LIST;
					
					//updating the que of the LCD
					returnPHY = osMessageQueuePut(queue_lcd_id,&myMessage,NULL,osWaitForever);
					CheckRetCode(returnPHY,__LINE__,__FILE__,CONTINUE);
				
					struct queueMsg_t Buffer;
			
					// Trying to read the Buffer queue 
					osStatus_t  bufferStatus =	osMessageQueueGet(queue_macB_id,&Buffer,NULL,0);
					original = Buffer.anyPtr;
					// if message queue is empty 
					if (bufferStatus == osOK)
					{
						// sending the messages inside of the buffer 
						copy = osMemoryPoolAlloc(memPool,osWaitForever);
						memcpy(copy,Buffer.anyPtr,80);
						Buffer.anyPtr = copy;
						returnPHY = osMessageQueuePut(queue_phyS_id,&Buffer,NULL,osWaitForever);
						if(returnPHY != osOK)
						{
							osMemoryPoolFree(memPool,Buffer.anyPtr);
						}
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
					ptk[MYADDRESS+1] = (1 << TIME_SAPI);
					// if we are connected
					if(gTokenInterface.connected)
					{
						// chat sapi must be active
						ptk[MYADDRESS+1] |= (1 << CHAT_SAPI) ;
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
