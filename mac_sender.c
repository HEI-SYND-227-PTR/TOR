#include "stm32f7xx_hal.h"

#include <stdio.h>
#include <string.h>

#include "main.h"

void MacSender(void *argument)
{
	// Local variable use in many cases
	uint8_t* memory;
	struct queueMsg_t message;
	struct queueMsg_t myMessage = {0};
	osStatus_t returnPHY;
	DataFrame f;
	uint8_t ttl = 0;
	uint8_t * original;
	uint8_t * copy;
	uint8_t * token;
	// Read The Mac Sender queue for ever
	while(1)
	{		
		// Getting a message from the Mac Sender queue
		osMessageQueueGet(queue_macS_id,&message,NULL,osWaitForever);
		// Switch case for all type of message received
		switch(message.type){
			case DATABACK :  //---------------------------------------- CASE DATABACK --------------------------------------------
			{		
				// Transform the red message to a DataFrame struct type
				fromByteArrayToStruct(message.anyPtr, &f);		
				if((f.s.status_field.ack != 1 || f.s.status_field.read != 1) && ttl < 4)
				{
					// TTL => Time to Live	
					ttl++;
					// There were a problem with the acknowledge or the read byte, we need to resend a message			
					osMemoryPoolFree(memPool,message.anyPtr);
					// Make a copy of the sent message
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
						// After trying to send the same message 4 times, send a MAC_ERROR message to the LCD screen
						myMessage.type = MAC_ERROR;
											
						sprintf(myMessage.anyPtr,"MAC error:\nStation %d does not respond !\r\n", f.c.control_field.daddr + 1);
									
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
						// In case no other condition was triggered, free the message pointer to avoid overload the memory pool
						if(f.c.control_field.daddr != MYADDRESS && f.c.control_field.daddr != BROADCAST_ADDRESS  )
						{
								osMemoryPoolFree(memPool,message.anyPtr);
						}						
					}							
					// The original message can be forgotten, since the dispatch was correctly sent and received 
					osMemoryPoolFree(memPool,original);	
					
					// The token can be released, since the dispatch was correctly sent and received 
					myMessage.type = TO_PHY;
					myMessage.anyPtr = token;				
					
					returnPHY = osMessageQueuePut(queue_phyS_id,&myMessage,NULL,osWaitForever);
					CheckRetCode(returnPHY,__LINE__,__FILE__,CONTINUE);

					ttl = 0;					
				}
			}
			case DATA_IND:   //---------------------------------------- CASE DATA_IND --------------------------------------------
			{		
				// Create the DataFrame with the correspondent control field		
				f.c.control_field.ssap = (gTokenInterface.station_list[MYADDRESS]>>1);
				f.c.control_field.saddr = MYADDRESS;
				f.c.control_field.dsap = message.sapi;
				f.c.control_field.daddr = message.addr;
				// Create the DataFrame with the correspondent data pointer and length
				f.dataPtr = message.anyPtr;
				f.length = strlen(message.anyPtr);	
			
				// In case of a broadcast message, the acknowledge and read bit must be set to 1
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
				
				// Allocate a memory space in the memory pool
				memory = osMemoryPoolAlloc(memPool,osWaitForever);

				myMessage.type = TO_PHY;
				fromStructToByteArray(f, memory);
				myMessage.anyPtr = memory;

				// In case our station is connected or in case the time sapi bit is set, send the message in the Mac Buffer queue
				if(gTokenInterface.connected == true || (f.c.control_field.dsap == TIME_SAPI))
				{
					returnPHY = osMessageQueuePut(queue_macB_id,&myMessage,NULL,0);
					if(returnPHY != osOK){
						osMemoryPoolFree(memPool,memory);
					}
				}
				else
				{
					osMemoryPoolFree(memPool,memory);
				}				
					
				osMemoryPoolFree(memPool,message.anyPtr);
			}
			case TOKEN:      //---------------------------------------- CASE TOKEN -----------------------------------------------
			{		
				// Putting zeros everywhere
				((uint8_t *)message.anyPtr)[MYADDRESS+1] = 0;
				// Place the token header
				((uint8_t *)message.anyPtr)[0] = TOKEN_TAG;
				// The time sapi is always active	
				((uint8_t *)message.anyPtr)[MYADDRESS+1] = (1 << TIME_SAPI);									
		
				// In case ou station is connected, set the chat sapi bit
				if(gTokenInterface.connected)					
				{
					((uint8_t *)message.anyPtr)[MYADDRESS+1] |= (1 << CHAT_SAPI) ;
				}
				
				// Copy the message in our local variable <token>
				token = message.anyPtr;	
				
				// Update the list of connected stations 
				for(int i = 0; i <= 14 ; i++)
				{
					gTokenInterface.station_list[i] = ((uint8_t *)message.anyPtr)[i+1];					
				}
				
				// Add the correspondent message type
				myMessage.type = TOKEN_LIST;
				
				// Update the station list on the LCD sreen
				returnPHY = osMessageQueuePut(queue_lcd_id,&myMessage,NULL,osWaitForever);
				CheckRetCode(returnPHY,__LINE__,__FILE__,CONTINUE);
			
				struct queueMsg_t Buffer;
		
				// Trying to read the Mac Buffer queue 
				osStatus_t  bufferStatus =	osMessageQueueGet(queue_macB_id,&Buffer,NULL,0);
				original = Buffer.anyPtr;
				// In cas the message as been successfully retrieve from the Mac Buffer queue
				if (bufferStatus == osOK)
				{
					// Copy the red message on a local variable
					copy = osMemoryPoolAlloc(memPool,osWaitForever);
					memcpy(copy,Buffer.anyPtr,80);
					Buffer.anyPtr = copy;

					// Send the message in the physical sender queue
					returnPHY = osMessageQueuePut(queue_phyS_id,&Buffer,NULL,osWaitForever);
					if(returnPHY != osOK)
					{
						osMemoryPoolFree(memPool,Buffer.anyPtr);
					}
					CheckRetCode(returnPHY,__LINE__,__FILE__,CONTINUE);
				}
				// In case the queue is empty
				else if(bufferStatus == osErrorResource)
				{
					// The token can be released, since there is no message to send
					returnPHY = osMessageQueuePut(queue_phyS_id,&message,NULL,osWaitForever);
					CheckRetCode(returnPHY,__LINE__,__FILE__,CONTINUE);
				}
		}
			case START:		 //---------------------------------------- CASE START -----------------------------------------------
			{
				// Update our stations status to "true"
				gTokenInterface.connected = true;				
			}
			case STOP:		 //---------------------------------------- CASE STOP ------------------------------------------------
			{
				// Update our stations status to "false"
				gTokenInterface.connected = false;				
			}
			case NEW_TOKEN:	 //---------------------------------------- CASE NEW_TOKEN -------------------------------------------
			{
			// Allocate a memory space to save the token in the memory pool
			uint8_t* ptk = osMemoryPoolAlloc(memPool,osWaitForever);
			if (ptk)
			{
				// Putting zeros everywhere
				memset(ptk,0,17);
				// Adding the token header
				ptk[0] = TOKEN_TAG;
				// The time sapi is always active
				ptk[MYADDRESS+1] = (1 << TIME_SAPI);

				// In case ou station is connected, set the chat sapi bit
				if(gTokenInterface.connected)
				{
					// chat sapi must be active
					ptk[MYADDRESS+1] |= (1 << CHAT_SAPI) ;
				}			
				// Create the token message to send in the ring
				myMessage.type = TO_PHY;
				myMessage.anyPtr = ptk;				
				
				// Send the message in the physical sender queue
				returnPHY = osMessageQueuePut(queue_phyS_id,&myMessage,NULL,osWaitForever);
				CheckRetCode(returnPHY,__LINE__,__FILE__,CONTINUE);
			}		
		}
		}
	}
}
