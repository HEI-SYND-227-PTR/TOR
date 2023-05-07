#include "stm32f7xx_hal.h"

#include <stdio.h>
#include <string.h>

#include "main.h"

bool checkCS(DataFrame f);

void MacReceiver(void *argument)
{	
	struct queueMsg_t message;
	osStatus_t returnPHY;
	struct queueMsg_t myMessage = {0};
	DataFrame frame;
	// Read The Mac Receiver queue for ever
	while(1)
	{
		// Getting a message from the Mac Receiver queue
		osMessageQueueGet(queue_macR_id,&message,NULL,osWaitForever);
		// Switch case for all type of message received
		switch(message.type)
		{
			case FROM_PHY: //---------------------------------------- CASE FROM_PHY --------------------------------------------
			{			
				// In case the received message is a TOKEN, send it back to the Mac Sender queue
				if (((uint8_t*)message.anyPtr)[0] == TOKEN_TAG)
				{
					// Create the message with the corresponding type and anyPtr
					myMessage.type = TOKEN;
					myMessage.anyPtr = message.anyPtr;			
				
					returnPHY = osMessageQueuePut(queue_macS_id,&myMessage,NULL,osWaitForever);
					if(returnPHY != osOK)
					{
						osMemoryPoolFree(memPool,message.anyPtr);
					}
					CheckRetCode(returnPHY,__LINE__,__FILE__,CONTINUE);
				}
				else
				{
					// Transform the red message to a DataFrame struct type
					fromByteArrayToStruct(((uint8_t*)message.anyPtr), & frame);
					// In case the message sender was ourself
					if(frame.c.control_field.saddr == MYADDRESS)
					{
						// In case the message recipient was ourself or the message was a broadcast 
						if(frame.c.control_field.daddr == MYADDRESS || frame.c.control_field.daddr == BROADCAST_ADDRESS)
						{
							// The message was intercepted and red								
							frame.s.status_field.read = 1;
					
							// Check the CS and set the acknowledge bit if CS is correct
							if(!checkCS(frame))
							{
								// In case the CS is incorrect 
								frame.s.status_field.ack = 0;
							}
							else
							{
								// In case the CS is correct
								frame.s.status_field.ack = 1;
							
								// Create the message with the corresponding frame
								myMessage.type = DATA_IND;
								frame.dataPtr[frame.length] = 0x00;
								frame.length = frame.length +1;
								myMessage.anyPtr = frame.dataPtr;		
								myMessage.addr = frame.c.control_field.saddr;						
								myMessage.sapi = frame.c.control_field.ssap;
							
								// In case of a chat message (0b001 sapi)
								if(frame.c.control_field.dsap == CHAT_SAPI)
								{
									// In case ou station is connected
									if(gTokenInterface.connected == true)
									{
										// Send the message in the chat receiver queue
										returnPHY = osMessageQueuePut(queue_chatR_id,&myMessage,NULL,0);
										if(returnPHY != osOK)
										{
											osMemoryPoolFree(memPool,message.anyPtr);
										}
										CheckRetCode(returnPHY,__LINE__,__FILE__,CONTINUE);
									}
									// In case ou station is disconnected
									else
									{
										// Need to free the un-used message memory to avoid overloading the memory pool
										osMemoryPoolFree(memPool,message.anyPtr);
									}
								}
								// In case of a time message (0b011 sapi)
								else if(frame.c.control_field.dsap == TIME_SAPI)
								{
									// Send the message in the time receiver queue
									returnPHY = osMessageQueuePut(queue_timeR_id,&myMessage,NULL,0);
									if(returnPHY != osOK)
									{
										osMemoryPoolFree(memPool,message.anyPtr);
									}
									CheckRetCode(returnPHY,__LINE__,__FILE__,CONTINUE);
								}
								else
								{
									// In case no message sapi match ours, we need to send back the message in the ring 
									myMessage.type = TO_PHY;
									myMessage.anyPtr = message.anyPtr;

									// Send the message in the physical Sender queue
									returnPHY = osMessageQueuePut(queue_phyS_id,&myMessage,NULL,0);
									if(returnPHY != osOK)
									{
										osMemoryPoolFree(memPool,message.anyPtr);
									}
									CheckRetCode(returnPHY,__LINE__,__FILE__,CONTINUE);									
								}
							}									
							// Create a byteArray from the DataFrame struct
							fromStructToByteArray(frame, message.anyPtr);
						
							// Create a DATABACK message
							myMessage.type = DATABACK;
							myMessage.anyPtr = message.anyPtr;
							
							// Send the message in the Mac Sender queue
							returnPHY = osMessageQueuePut(queue_macS_id,&myMessage,NULL,0);
							if(returnPHY != osOK)
							{
								osMemoryPoolFree(memPool,message.anyPtr);
							}					
						}
						// In case the message recipient someone else
						else
						{	
							// Check the CS and set the acknowledge bit if CS is correct
							if(!checkCS(frame))
							{
								// In case the CS is incorrect 
								frame.s.status_field.ack = 0;
							}						
								// In case the CS is correct 
								// Create a DATABACK message
								myMessage.type = DATABACK;
								myMessage.anyPtr = message.anyPtr;		
								myMessage.addr = frame.c.control_field.saddr;						
								myMessage.sapi = frame.c.control_field.ssap;

								// Send the message in the Mac Sender queue
								returnPHY = osMessageQueuePut(queue_macS_id,&myMessage,NULL,osWaitForever);
								if(returnPHY != osOK)
								{
									osMemoryPoolFree(memPool,message.anyPtr);
								}
								CheckRetCode(returnPHY,__LINE__,__FILE__,CONTINUE);
						}	
					}
					// In case the message sender was someone else
					// In case the message recipient was ourself or the message was a broadcast
					else if(frame.c.control_field.daddr == MYADDRESS ||frame.c.control_field.daddr == BROADCAST_ADDRESS)
					{
						// The message was intercepted and red
						frame.s.status_field.read = 1;
					
						// Check the CS and set the acknowledge bit if CS is correct
						if(!checkCS(frame))
						{	
							// In case the CS is incorrect 
							frame.s.status_field.ack = 0;
						}
						else
						{	
							// In case the CS is correct
							frame.s.status_field.ack = 1;
						
							// Create the message with the corresponding frame
							myMessage.type = DATA_IND;
							myMessage.anyPtr = frame.dataPtr;	
							frame.dataPtr[frame.length] = 0x00;
							frame.length = frame.length +1;							
							myMessage.addr = frame.c.control_field.saddr;						
							myMessage.sapi = frame.c.control_field.ssap;
						
							// In case of a chat message (0b001 sapi)
							if(frame.c.control_field.dsap == CHAT_SAPI)
							{
								// In case ou station is connected
								if(gTokenInterface.connected == true)
								{
									// Send the message in the chat receiver queue
									returnPHY = osMessageQueuePut(queue_chatR_id,&myMessage,NULL,0);
									if(returnPHY != osOK)
									{
										osMemoryPoolFree(memPool,message.anyPtr);
									}
									CheckRetCode(returnPHY,__LINE__,__FILE__,CONTINUE);
								}
								// In case ou station is disconnected
								else
								{
									// Need to free the un-used message memory to avoid overloading the memory pool
									osMemoryPoolFree(memPool,message.anyPtr);
								}
							}
							// In case of a time message (0b011 sapi)
							else if(frame.c.control_field.dsap == TIME_SAPI)
							{
								// Send the message in the time receiver queue
								returnPHY = osMessageQueuePut(queue_timeR_id,&myMessage,NULL,0);
								if(returnPHY != osOK)
								{
									osMemoryPoolFree(memPool,message.anyPtr);
								}
								CheckRetCode(returnPHY,__LINE__,__FILE__,CONTINUE);
							}
							else
							{
								// In case no message sapi match ours, we need to send back the message in the ring 
								myMessage.type = TO_PHY;
								myMessage.anyPtr = message.anyPtr;

								// Send the message in the physical Sender queue
								returnPHY = osMessageQueuePut(queue_phyS_id,&myMessage,NULL,0);
								if(returnPHY != osOK)
								{
									osMemoryPoolFree(memPool,message.anyPtr);
								}
								CheckRetCode(returnPHY,__LINE__,__FILE__,CONTINUE);									
							}
						}	
						// Create a byteArray from the DataFrame struct
						fromStructToByteArray(frame, message.anyPtr);
					
						myMessage.type = TO_PHY;
						myMessage.anyPtr = message.anyPtr;
						
						// Send the message in the physical Sender queue		
						returnPHY = osMessageQueuePut(queue_phyS_id,&myMessage,NULL,0);
						if(returnPHY != osOK)
						{
							osMemoryPoolFree(memPool,message.anyPtr);
						}
						
					}
					// In case we are neither sender nor receiver of the message nor was it a broadcast message,
					// send back the message on the ring
					else
					{
						myMessage.type = TO_PHY;
						myMessage.anyPtr = message.anyPtr;
	
						// Send the message in the physical Sender queue
						returnPHY = osMessageQueuePut(queue_phyS_id,&myMessage,NULL,0);
						if(returnPHY != osOK)
						{
							osMemoryPoolFree(memPool,message.anyPtr);
						}
						CheckRetCode(returnPHY,__LINE__,__FILE__,CONTINUE);	
					}
				}			
			}
			break;	
		}	
	}
}
//-------------------------------------- Calculate and compare checksum value in the DataFrame -------------------------------
bool checkCS(DataFrame f)
{
	uint32_t sum = 0;
	sum =  (f.c.control>>8)+ (f.c.control&0xff) + (f.length);
	for(int i = f.length; i > 0; i-- )
	{
		sum += f.dataPtr[i-1];
	}
	sum &= 63;

	return (f.s.status_field.cs == sum);
}



