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
	
	while(1)
	{
		osMessageQueueGet(queue_macR_id,&message,NULL,osWaitForever);
		switch(message.type)
		{
			case FROM_PHY: 
			{			
				if (((uint8_t*)message.anyPtr)[0] == TOKEN_TAG)
				{
					// creation of a message
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
					
					 fromByteArrayToStruct(((uint8_t*)message.anyPtr), & frame);
					// we check if the message was from us
					if(frame.c.control_field.saddr == MYADDRESS)
					{
						// self message
						if(frame.c.control_field.daddr == MYADDRESS || frame.c.control_field.daddr == BROADCAST_ADDRESS)
						{
							// we do the same thing as a normal message but instead we re-send the 
							// message in DATABACK
								
							frame.s.status_field.read = 1;
					
							if(!checkCS(frame))
							{
								frame.s.status_field.ack = 0;
							}
							else
							{
								frame.s.status_field.ack = 1;
							
								myMessage.type = DATA_IND;
								frame.dataPtr[frame.length] = 0x00;
								frame.length = frame.length +1;
								myMessage.anyPtr = frame.dataPtr;		
								myMessage.addr = frame.c.control_field.saddr;						
								myMessage.sapi = frame.c.control_field.ssap;
							
								// if it is a chat message (0b001 sapi)
								if(frame.c.control_field.dsap == CHAT_SAPI)
								{
									if(gTokenInterface.connected == true)
									{
										returnPHY = osMessageQueuePut(queue_chatR_id,&myMessage,NULL,0);
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
								}
								// if it is a time message (0b011 sapi)
								else if(frame.c.control_field.dsap == TIME_SAPI)
								{
									returnPHY = osMessageQueuePut(queue_timeR_id,&myMessage,NULL,0);
									if(returnPHY != osOK)
									{
										osMemoryPoolFree(memPool,message.anyPtr);
									}
									CheckRetCode(returnPHY,__LINE__,__FILE__,CONTINUE);
								}
								else
								{
									myMessage.type = TO_PHY;
									myMessage.anyPtr = message.anyPtr;
	
									returnPHY = osMessageQueuePut(queue_phyS_id,&myMessage,NULL,0);
									if(returnPHY != osOK)
									{
									osMemoryPoolFree(memPool,message.anyPtr);
									}
									CheckRetCode(returnPHY,__LINE__,__FILE__,CONTINUE);									
								}
							}									
						
							fromStructToByteArray(frame, message.anyPtr);
						
							myMessage.type = DATABACK;
							myMessage.anyPtr = message.anyPtr;
							
							returnPHY = osMessageQueuePut(queue_macS_id,&myMessage,NULL,0);
							if(returnPHY != osOK)
							{
								osMemoryPoolFree(memPool,message.anyPtr);
							}					
						}
						else
						{
							if(!checkCS(frame))
							{
								frame.s.status_field.ack = 0;
							}						
								// if so we need to send a databack to MAC sender 
								// creation of a message
								myMessage.type = DATABACK;
								myMessage.anyPtr = message.anyPtr;		
								myMessage.addr = frame.c.control_field.saddr;						
								myMessage.sapi = frame.c.control_field.ssap;

								returnPHY = osMessageQueuePut(queue_macS_id,&myMessage,NULL,osWaitForever);
								if(returnPHY != osOK)
								{
									osMemoryPoolFree(memPool,message.anyPtr);
								}
								CheckRetCode(returnPHY,__LINE__,__FILE__,CONTINUE);
						}	
					}
					// we check if the message was for us
					else if(frame.c.control_field.daddr == MYADDRESS ||frame.c.control_field.daddr == BROADCAST_ADDRESS)
					{
					
						frame.s.status_field.read = 1;
					
						if(!checkCS(frame))
						{
							frame.s.status_field.ack = 0;
						}
						else
						{
							frame.s.status_field.ack = 1;
						
							myMessage.type = DATA_IND;
							myMessage.anyPtr = frame.dataPtr;	
							frame.dataPtr[frame.length] = 0x00;
							frame.length = frame.length +1;							
							myMessage.addr = frame.c.control_field.saddr;						
							myMessage.sapi = frame.c.control_field.ssap;
						
							// if it is a chat message (0b001 sapi)
							if(frame.c.control_field.dsap == CHAT_SAPI)
							{
								if(gTokenInterface.connected == true)
								{
									returnPHY = osMessageQueuePut(queue_chatR_id,&myMessage,NULL,0);
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
							}
							// if it is a time message (0b011 sapi)
							else if(frame.c.control_field.dsap == TIME_SAPI)
							{
								returnPHY = osMessageQueuePut(queue_timeR_id,&myMessage,NULL,0);
								if(returnPHY != osOK)
								{
									osMemoryPoolFree(memPool,message.anyPtr);
								}
								CheckRetCode(returnPHY,__LINE__,__FILE__,CONTINUE);
							}
							else
							{
								myMessage.type = TO_PHY;
								myMessage.anyPtr = message.anyPtr;

								returnPHY = osMessageQueuePut(queue_phyS_id,&myMessage,NULL,0);
								if(returnPHY != osOK)
								{
								osMemoryPoolFree(memPool,message.anyPtr);
								}
								CheckRetCode(returnPHY,__LINE__,__FILE__,CONTINUE);									
							}
						}	

						fromStructToByteArray(frame, message.anyPtr);
					
						myMessage.type = TO_PHY;
						myMessage.anyPtr = message.anyPtr;
						
						returnPHY = osMessageQueuePut(queue_phyS_id,&myMessage,NULL,0);
						if(returnPHY != osOK)
						{
							osMemoryPoolFree(memPool,message.anyPtr);
						}
						
					}
					else
					{
						myMessage.type = TO_PHY;
						myMessage.anyPtr = message.anyPtr;
	
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



