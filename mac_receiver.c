#include "main.h"
#include <stdio.h>
#include <string.h>

void MacReceiver(void *argument)
{
	struct queueMsg_t queueMsg; // queue message
	osStatus_t retCode;
	for (;;) // loop until doomsday
	{
		retCode = osMessageQueueGet(
			queue_macR_id,
			&queueMsg,
			NULL,
			osWaitForever);
		CheckRetCode(retCode, __LINE__, __FILE__, CONTINUE);

		uint8_t *dataPtr = (uint8_t *)queueMsg.anyPtr;

		if(queueMsg.type == FROM_PHY)
		{
			if(dataPtr[0] == TOKEN_TAG)
			{
				queueMsg.type = TOKEN;
				retCode = osMessageQueuePut(
					queue_macS_id,
					&queueMsg,
					osPriorityNormal,
					osWaitForever);
				CheckRetCode(retCode, __LINE__, __FILE__, CONTINUE);
			}
			else // Not a token
			{
				uint8_t srcAddr = (dataPtr[0] >> 3);
				uint8_t destAddr = (dataPtr[1] >> 3);
				uint8_t srcSapi = (dataPtr[0] & 0b111);
				uint8_t destSapi = (dataPtr[1] & 0b111);
				
				if(srcAddr == gTokenInterface.myAddress) // Send Databack
				{
					queueMsg.type = DATABACK;
					queueMsg.addr = srcAddr;
					queueMsg.sapi = srcSapi;
					retCode = osMessageQueuePut(
						queue_macS_id,
						&queueMsg,
						osPriorityNormal,
						osWaitForever);
					CheckRetCode(retCode, __LINE__, __FILE__, CONTINUE);					
				}
				else if(destAddr == gTokenInterface.myAddress) // Send up and to PHY_S
				{
					bool read = true;
					bool ack = false;

					// CRC Compute
					uint8_t crc = dataPtr[0] + dataPtr[1] + dataPtr[2];
					for (uint8_t i = 0; i < dataPtr[2]; i++)
					{
						crc = crc + dataPtr[3 + i];
					}
					if(crc == (dataPtr[3 + dataPtr[2]] >> 2)) //CRC Check
					{
						ack = true;
					}
					dataPtr[3 + dataPtr[2]] = dataPtr[3 + dataPtr[2]] & (ack + (read << 1));

					uint8_t *msg = osMemoryPoolAlloc(memPool, osWaitForever);
					strcpy(msg,&dataPtr[3]);

					queueMsg.type = TO_PHY;
					retCode = osMessageQueuePut(
						queue_phyS_id,
						&queueMsg,
						osPriorityNormal,
						osWaitForever);
					CheckRetCode(retCode, __LINE__, __FILE__, CONTINUE);	

					queueMsg.type = DATA_IND;
					queueMsg.anyPtr = msg;
					queueMsg.addr = srcAddr;
					queueMsg.sapi = srcSapi;

					if(destSapi == CHAT_SAPI)
					{
						retCode = osMessageQueuePut(
							queue_chatR_id,
							&queueMsg,
							osPriorityNormal,
							osWaitForever);
						CheckRetCode(retCode, __LINE__, __FILE__, CONTINUE);	
					}
					else if (destSapi == TIME_SAPI)
					{
						retCode = osMessageQueuePut(
							queue_timeR_id,
							&queueMsg,
							osPriorityNormal,
							osWaitForever);
						CheckRetCode(retCode, __LINE__, __FILE__, CONTINUE);							
					}
				}
			}
		}
	}
}
