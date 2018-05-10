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
#include <dirent.h>
#include <stat.h>
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
void sendFile(char fs_name[]);
int numOfFiles();

FUNC(void, RTE_CTCDETHCOM_APPL_CODE) REthComInit(void) /* PRQA S 0850 */ /* MD_MSR_19.8 */
{
	messages = msgQCreate(BUFLEN, BUFLEN, MSG_Q_FIFO);
	changeState = 1;
	
	
	task = taskCreate("task1", 115, VX_FP_TASK, 0x8000U, (FUNCPTR)backgroundTask, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
	taskActivate(task);
}


FUNC(void, RTE_CTCDETHCOM_APPL_CODE) REthComCyclic(void) /* PRQA S 0850 */ /* MD_MSR_19.8 */
{
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
			
			//open&read&sendNumOf dir---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
			//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
			//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
			int filesNum = numOfFiles();
			int networkFilesNum;
			
			networkFilesNum = htonl(filesNum);
			if(send(newSocket, &networkFilesNum, sizeof(networkFilesNum), 0) == SOCKET_ERROR)
			{
				printf("Number of files send() FAILED!\n\n");
			}
			puts("Number of files is sent!\n");
			
			
			
			
			DIR* dirp;
			struct dirent* direntp;
			//ime fajla
			char tempStr[BUFLEN];
			
			dirp = opendir("/mmc0:4/a");
			if(dirp == NULL)
			{
				puts("error opening dir!\n\n");
			}
			else
			{
				for(;;)
				{
					direntp = readdir(dirp);
					 
					if(direntp == NULL)
					{
						break;
					}
					
					if((strcmp(direntp->d_name, ".") != 0) && (strcmp(direntp->d_name, "..") != 0))
					{
						//nasetuj na 0
						memset(tempStr, 0, BUFLEN);
						
						//ovde je ime fajla
						strcpy(tempStr, direntp->d_name);
						
						//puts(tempDir);
						puts("U FUNKCIJI GDE CITAM IMENA FAJLOVA:");
						puts(tempStr);
						
						//funkcija u kojoj prvo posaljem ime fajla pa zatim i sam fajl 
						sendFile(tempStr);
						
					}
				}
				puts("");
				
				closedir(dirp);
			}
			
		
			
			close(s);
			close(newSocket);
			
			msgQSend(messages, (char*)&changeState, sizeof(changeState), NO_WAIT, MSG_PRI_NORMAL);
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
void sendFile(char fs_name[])
{
	//sleep(1);
	
	//vreme sleep-a
	struct timespec nsTime;
	nsTime.tv_sec = 0;
	nsTime.tv_nsec = 50000000;	//trebalo bi da je ovo 0.05 sekundi
	
	char tempDir[BUFLEN];
	long fileLentgh;
	char sdbuf[BUFLEN];
	int blockSize;
	
	printf("Pre slanja imena fajla: %s\n", fs_name);
	
	nanosleep(&nsTime, NULL);
	
	//slanje imena fajla BEZ PUTANJE
	if(send(newSocket, fs_name, strlen(fs_name), 0) == SOCKET_ERROR)
	{
		printf("Name of the file send() FAILED!\n\n");
	}
	nanosleep(&nsTime, NULL);

	puts("Name of the file is sent\n\n");
	printf("size of name sent: %d\n", strlen(fs_name));
	
	
	//cistim tempDir
	memset(tempDir, 0, BUFLEN);
	//dodajem putanju
	strcpy(tempDir, "/mmc0:4/a/");
	//spajanje putanje i imena fajla u tempDir-u
	strcat(tempDir, fs_name);
	printf("NAKON SPAJANJA PUTANJE I IMENA FAJLA: %s", tempDir);
	puts("");
	
	printf("Sending %s to the client... \n\n", tempDir);
	FILE *fs = fopen(tempDir, "rb");
	if(fs == NULL)
	{
		printf("ERROR: File %s not found.\n", tempDir);
	}
	
	//idi do kraja
	fseek(fs, 0, SEEK_END);
	//nadji velicinu fajla
	fileLentgh = ftell(fs);
	//vrati na pocetak
	fseek(fs, 0, SEEK_SET);
	
	printf("Velicina fajla koji se salje je: %ld\n\n\n", fileLentgh);
	
	//slanje velicine tekuceg fajla
	long convertedNumber = htonl(fileLentgh);
	if(send(newSocket, &convertedNumber, sizeof(convertedNumber), 0) == SOCKET_ERROR)
	{
		printf("Name of the file send() FAILED!\n\n");
	}
	puts("Size of file is sent!\n");
	
	

	//ocisti sdbuf
	memset(sdbuf, 0, BUFLEN); 
	
	puts("ispred while-a za slanje fajla\n\n");
	while((blockSize = fread(sdbuf, sizeof(char), BUFLEN, fs)) != '\0')
	{
		nanosleep(&nsTime, NULL);
		
		printf("%d\t", blockSize);
		
		
		if(send(newSocket, sdbuf, blockSize, 0) < 0)
		{
			fprintf(stderr, "ERROR: Failed to send file %s. (errno = %d)\n", tempDir, errno);
			break;
		}
		memset(sdbuf, 0, BUFLEN); 
	}
	printf("\n\nOk! File %s from server was sent!\n\n", tempDir);
	
	
	fclose(fs);
	
	nanosleep(&nsTime, NULL);
}

int numOfFiles()
{
	//brojac fajlova
	int i = 0;
	
	DIR* dirp;
	struct dirent* direntp;
	
	dirp = opendir("/mmc0:4/a");
	if(dirp == NULL)
	{
		puts("error opening dir!\n\n");
	}
	else
	{
		for(;;)
		{
			direntp = readdir(dirp);
					 
			if(direntp == NULL)
			{
				break;
			}
			
			if((strcmp(direntp->d_name, ".") != 0) && (strcmp(direntp->d_name, "..") != 0))
			{
				i++;
			}
		}
		
		closedir(dirp);
	}
	
	printf("Nalazim se u funkciji numOfFiles() i izbrojao sam %d fajlova\n\n", i);
	
	return i;
}
