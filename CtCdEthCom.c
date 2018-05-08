#include "MemMap.h" /* PRQA S 5087 */ /* MD_MSR_19.1 */
#include "Rte_CtCdEthCom.h"
#include "Sl_WaitUS.h"
#include "SWCVersionInfo.h"

#include <vxWorks.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h >
#include <sysctlLib.h>
#include <taskLib.h>
#include <stdlib.h>
#include <string.h>
#include <net/if.h>
#include <net/route.h>
#include <ipnet/ipioctl.h>
#include <ipnet/ipioctl_var.h>
#include <stdio.h>
#include <time.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h>
#include <errno.h>
#include <cpuset.h>
#include <sys/mman.h>
#include <sys/fcntlcom.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <msgQLib.h>
#include <fioLib.h>
#include <sockLib.h>
#include <types.h>
#include <socket.h>
#include <netLib.h>
#include <inetLib.h>
#include <stdarg.h>
#include <stddef.h>
#include <net/if_dl.h>
#include <net/if_types.h>
#include <net/ifaddrs.h>
#include <netinet6/in6_var.h>
#include <netinet/ip.h>
#include <sys/ioctl.h>
#include <timers.h>
#include <timerLib.h>
//#include <inflateLib.h>
#include <zlib.h>

//*********************************************************

#define RTE_STOP_SEC_CTCDETHCOM_APPL_CODE
#define RTE_START_SEC_CTCDETHCOM_APPL_CODE

#define SWU_BR_SERVERPORT  29170
#define SWU_BR_CLIENTPORT  29171
#define SWUP_ZFAS_IP_ADDRESS "fd53:7cb8:383:3::4f"	//zFAS
#define SWUP_MIB_ZR_IP_ADDRESS "fd53:7cb8:383:3::73"	//PC
#define SOCKET_ERROR -1
#define BUFLEN 512
#define BACKLOG 10

#define FILENAME "0:/mmc0:1/guitar.jpg"


MSG_Q_ID messages;
TASK_ID task;

struct sockaddr_in server, client;
int s, newSocket, c, recvSize;
char buffer[BUFLEN];

int changeState;	

int changeFSMState;
int changeBackgroundTaskState;

char message[] = "Start";
char respondOK[] = "Let's communicate!";
char respondNotOK[] = "Communication breakdown...";

//FSM functions
void init();
void idle();
void dataCollection();
void dataEncryption();
void dataUpload();

//Task and Message functions
static void backgroundTask(void);


//Other functions


FUNC(void, RTE_CTCDETHCOM_APPL_CODE) REthComInit(void) /* PRQA S 0850 */ /* MD_MSR_19.8 */
{
	messages = msgQCreate(BUFLEN, BUFLEN, MSG_Q_FIFO);
	changeState = 1;
	
	
	task = taskCreate("task1", 115, VX_FP_TASK, 0x8000U, (FUNCPTR)backgroundTask, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
	taskActivate(task);
}


FUNC(void, RTE_CTCDETHCOM_APPL_CODE) REthComCyclic(void) /* PRQA S 0850 */ /* MD_MSR_19.8 */
{
	//fsm switch case
	/*switch(changeState)
	{
		case 1:
			idle();
			break;
		case 2:
			dataCollection();
			break;
		case 3:
			dataEncryption();
			break;
		case 4:
			dataUpload();
			break;
		default:
			idle();
			break;
	}*/
	
	if(changeState == 1)
	{
		puts("ulazim u idleeeeeeeeeeee");
		idle();
	}
	
}

//FSM functions
void idle()
{
	msgQSend(messages, (char*)&changeState, sizeof(changeState), NO_WAIT, MSG_PRI_NORMAL); //msg = 1
}

//Task and Message functions
static void backgroundTask(void)
{
	int msg;
  
	while(1)
	{
		(void) msgQReceive(messages, (char*)&msg, sizeof(msg), WAIT_FOREVER);
		//ovde treba da implementiras sve sto moze da trosi vise od 150us vremena, znaci otvaranje fajlova, socketa, enkripcija, sve...	

		
		if (msg == 1)
		{
			printf("changeSTAT: %d\n\n", changeState);
			changeState = 2;
			
			
			//Create a socket
			if((s = socket(AF_INET, SOCK_STREAM, 0 )) == SOCKET_ERROR)
			{
				printf("Could not create socket!\n");
			}
			printf("Socket created.\n");
			
			memset((char*)&server, 0, sizeof(server));
			server.sin_family = AF_INET;
			server.sin_addr.s_addr = INADDR_ANY;
			server.sin_port = htons(SWU_BR_SERVERPORT);
			
			//Bind
			if( bind(s,(struct sockaddr*)&server , sizeof(server)) == SOCKET_ERROR)
			{
				printf("Bind failed: %s\n", strerror(errno)); 
				//return 1;
			}
			 
			puts("Bind done!");
			
			printf("Listetning...\n");
			
			//Listen to incoming connections
			listen(s, BACKLOG);
			
			//Accept and incoming connection
			puts("Waiting for incoming connections...\n\n");
			
			
			c = sizeof(struct sockaddr_in);
			
			newSocket = accept(s ,(struct sockaddr*)&client, &c);
			if (newSocket == SOCKET_ERROR)
			{
				printf("Accept failed with error!\n");
			}
			puts("Connection accepted!");
			
			
			//try to receive initial message from client
			if ((recvSize = recv(newSocket, buffer, BUFLEN, 0)) == SOCKET_ERROR)
			{
				puts("Recv from client failed!");
			}
			puts("Reply received --------->");

			//Add a NULL terminating character to make it a proper string before printing
			buffer[recvSize] = '\0';
			puts(buffer);
			puts("\n");
			
			//Compare strings and respond to the client
			if(strcmp(buffer, message) == 0)
			{
				if(send(newSocket, respondOK, strlen(respondOK), 0) == SOCKET_ERROR)
				{
					printf("First send() failed\n");
				}
				puts("Everything is fine, let's chat!\n\n");
			}
			else
			{
				if(send(newSocket, respondNotOK, strlen(respondOK), 0) == SOCKET_ERROR)
				{
					printf("Second send() failed\n");
				}
			}
			
			//send image-------------------------------------------------------------------------------------------
			//-----------------------------------------------------------------------------------------------------
			
			
			char* fs_name = "/mmc0:4/guitar.bmp";
			char sdbuf[BUFLEN];
			int blockSize; 
			printf("Sending %s to the client... \n\n", fs_name);
			FILE *fs = fopen(fs_name, "rb");
			if(fs == NULL)
			{
				printf("ERROR: File %s not found.\n", fs_name);
			}
			
			
			struct timespec nsTime;
			nsTime.tv_sec = 0;
			nsTime.tv_nsec = 5000000;
			
			
			fseek(fs, 0, SEEK_SET);
			
			memset(sdbuf, 0, BUFLEN); 
			
			puts("ispred while\n\n");
			while((blockSize = fread(sdbuf, sizeof(char), BUFLEN, fs)) != '\0')
			{
				//sleep(1);
				
				nanosleep(&nsTime, NULL);
				
				printf("%d\t", blockSize);
				
				
				if(send(newSocket, sdbuf, blockSize, 0) < 0)
				{
					fprintf(stderr, "ERROR: Failed to send file %s. (errno = %d)\n", fs_name, errno);
					break;
				}
				memset(sdbuf, 0, BUFLEN); 
			}
			printf("\n\nOk! File %s from server was sent!\n\n", fs_name);
			
			
			
			
			
			fclose(fs);
			
			
			
			
			
			
			
			
			
			
			
			
			
			
			
			
			
			
			
			
			
			
			
			
			
			
			
			//puts("****************************************************");
			
			close(s);
			close(newSocket);
			//puts("zatvaram sokete");
			
			
			//printf("changeSTAT: %d\n\n", changeState);
			msgQSend(messages, (char*)&changeState, sizeof(changeState), NO_WAIT, MSG_PRI_NORMAL);
			//puts("msgQSend(messages, (char*)&changeState, sizeof(changeState), NO_WAIT, MSG_PRI_NORMAL);");
			puts("****************************************************");
		}
		else if(msg == 2)
		{
			printf("USAO SAM U dataCollection()!!!\n\n");
			sleep(1);
		}
		else if(msg == 3)
		{
			//dataEncryption();
		}
		else if(msg == 4)
		{
			//dataUpload();
		}	
	}
}

//Other functions