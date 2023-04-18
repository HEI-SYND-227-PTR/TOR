#include "stm32f7xx_hal.h"

#include <stdio.h>
#include <string.h>

#include "main.h"

void MacSender(void *argument)
{
	//TODO
	struct queueMsg_t message;
	struct queueMsg_t myPhyMessage;
	osStatus_t returnPHY;
	//Read The macR queue
	while(1)
	{
		osMessageQueueGet(queue_macS_id,&message,NULL,osWaitForever);
		if(message.type == NEW_TOKEN)
		{
			printf("WOHOOOW\r\n");
			// creation of the Token
			uint8_t* ptk = osMemoryPoolAlloc(memPool,osWaitForever);
			// TODO verifier que ptk != NULL
			memset(ptk,0,17);
			ptk[0] = 0xff;
			ptk[MYADDRESS] = 0x2A;
		
			
			myPhyMessage.type = TO_PHY;
			myPhyMessage.anyPtr = ptk;			
			myPhyMessage.sapi = NULL;
			myPhyMessage.addr = NULL;
			
			
			returnPHY = osMessageQueuePut(queue_phyS_id,&myPhyMessage,NULL,osWaitForever);
			CheckRetCode(returnPHY,__LINE__,__FILE__,CONTINUE);
		}
	}
	

}
