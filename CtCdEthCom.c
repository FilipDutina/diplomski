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

#define RTE_STOP_SEC_CTCDETHCOM_APPL_CODE
#define RTE_START_SEC_CTCDETHCOM_APPL_CODE

#define SWU_BR_SERVERPORT  29170
#define SWU_BR_CLIENTPORT  29171
/*#define SWUP_ZFAS_IP_ADDRESS "fd53:7cb8:383:3::4f"	
#define SWUP_MIB_ZR_IP_ADDRESS "fd53:7cb8:383:3::73"*/	
#define SOCKET_ERROR -1
#define BUFLEN 512
#define BACKLOG 10

/*baferi za enkripciju*/
uint8_t sdbuf[BUFLEN];
uint32_t en[BUFLEN];

/*javni kljucevi za enkripciju*/
uint32_t publicKey, n;

/**/
MSG_Q_ID messages;
TASK_ID task;

/*promenljive koje se koriste za komunikaciju sa racunarom*/
struct sockaddr_in server, client;
int32_t s, newSocket, c, recvSize;
char replyBuffer[BUFLEN];

/*flag na osnovu koga se menja stanje*/
int32_t changeState;	

/*poruke za komunikaciju*/
char message[] = "Start";
char respondOK[] = "Let's communicate!";
char respondNotOK[] = "Communication breakdown...";

/*deklaracije funkcija*/
static void backgroundTask(void);
void receivePublicKeys();
void sendFile(char fs_name[]);
int32_t numOfFiles();
void encrypt();


FUNC(void, RTE_CTCDETHCOM_APPL_CODE) REthComInit(void) 
{
	/**/
	messages = msgQCreate(BUFLEN, BUFLEN, MSG_Q_FIFO);
	changeState = 1;
	
	task = taskCreate("task1", 115, VX_FP_TASK, 0x8000U, (FUNCPTR)backgroundTask, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
	taskActivate(task);
}


FUNC(void, RTE_CTCDETHCOM_APPL_CODE) REthComCyclic(void) /* PRQA S 0850 */ /* MD_MSR_19.8 */
{
	msgQSend(messages, (char*)&changeState, sizeof(changeState), NO_WAIT, MSG_PRI_NORMAL); //msg = 1
}

/*DEFINICIJE FUNKCIJA*/

/*funkcija koju poziva promenljiva task*/
static void backgroundTask(void)
{
	int32_t msg;
  
	while(1)
	{
		/*primanje poruke na ovsnovu koje se zna u koje stanje treba ici*/
		(void) msgQReceive(messages, (char*)&msg, sizeof(msg), WAIT_FOREVER);
		
		if (msg == 1)
		{
			changeState = 2;
			
			/*kreiranje soketa*/
			if((s = socket(AF_INET, SOCK_STREAM, 0 )) == SOCKET_ERROR)
			{
				printf("Could not create socket!\n");
			}
			printf("\n\n\nSocket created.\n");
			
			memset((char*)&server, 0, sizeof(server));
			server.sin_family = AF_INET;
			server.sin_addr.s_addr = INADDR_ANY;
			server.sin_port = htons(SWU_BR_SERVERPORT);
			
			/*bindovanje*/
			if( bind(s,(struct sockaddr*)&server , sizeof(server)) == SOCKET_ERROR)
			{
				printf("Bind failed: %s\n", strerror(errno)); 
				//return 1;
			}
			 
			puts("Bind done!");
			printf("Listetning...\n");
			
			/*slusanje*/
			listen(s, BACKLOG);
			
			puts("Waiting for incoming connections...\n\n\n\n");
			
			c = sizeof(struct sockaddr_in);
			
			/*prihvatanje komunikacije*/
			newSocket = accept(s ,(struct sockaddr*)&client, &c);
			if (newSocket == SOCKET_ERROR)
			{
				printf("Accept failed with error!\n");
			}
			puts("Connection accepted!");
			
			/*primanje inicijalne poruke od racunara*/
			if ((recvSize = recv(newSocket, replyBuffer, BUFLEN, 0)) == SOCKET_ERROR)
			{
				puts("Recv from client failed!");
			}
			puts("Reply received --------->");

			/*terminacija stringa na kraju*/
			replyBuffer[recvSize] = '\0';
			puts(replyBuffer);
			puts("\n");
			
			/*uporedjivanje stringova*/
			if(strcmp(replyBuffer, message) == 0)
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
			
			/*primi javne kljuceve neophodne za enkripciju*/
			receivePublicKeys();
			
			/*broj fajlova u direktorijumu*/
			int32_t filesNum = numOfFiles();
			int32_t networkFilesNum;
			
			networkFilesNum = htonl(filesNum);
			/*slanje broja fajlova*/
			if(send(newSocket, &networkFilesNum, sizeof(networkFilesNum), 0) == SOCKET_ERROR)
			{
				printf("Number of files send() FAILED!\n\n");
			}
			puts("Number of files is sent!\n");
			
			DIR* dirp;
			struct dirent* direntp;
			/*ime fajla*/
			char tempStr[BUFLEN];
			
			/*putanja na kojoj se nalaze fajlovi koje najpre treba enkriptovati pa zatim i poslati racunaru*/
			dirp = opendir("/mmc0:4/a");
			if(dirp == NULL)
			{
				puts("Error opening dir!\n\n");
			}
			else
			{
				for(;;)
				{
					/*otvaranje direktorijuma*/
					direntp = readdir(dirp);
					 
					if(direntp == NULL)
					{
						break;
					}
					
					if((strcmp(direntp->d_name, ".") != 0) && (strcmp(direntp->d_name, "..") != 0))
					{
						/*ciscenje bafera*/
						memset(tempStr, 0, BUFLEN);
						
						/*kopiram ime fajla*/
						strcpy(tempStr, direntp->d_name);

						puts("U FUNKCIJI GDE CITAM IMENA FAJLOVA:");
						puts(tempStr);
						
						/*funkcija u kojoj prvo posaljem ime fajla pa zatim i sam fajl*/
						sendFile(tempStr);
						
					}
				}
				puts("");
				
				/*zatvaranje direktorijuma*/
				closedir(dirp);
			}
			
			/*zatvaranje soketa*/
			close(s);
			close(newSocket);
			
			/*prelazak u finalno stanje*/
			msgQSend(messages, (char*)&changeState, sizeof(changeState), NO_WAIT, MSG_PRI_NORMAL);
		}
		else if(msg == 2)
		{
			puts("\n\n\n\nProgram se uspesno izvrsio i svi fajlovi su poslati!\n\n\n\n");
			sleep(1);
		}
	}
}

/*primi javne kljuceve neophodne za enkripciju*/
void receivePublicKeys()
{
	puts("---------------------------------------------- receivePublicKeys");
	
	/*vreme spavanja 0.05 sekundi*/
	struct timespec nsTime;
	nsTime.tv_sec = 0;
	nsTime.tv_nsec = 50000000;
	
	uint64_t NETWORKmodulus;
	uint64_t NETWORKexponent;
	
	nanosleep(&nsTime, NULL);
	
	/*primi moduo*/
	if ((recvSize = recv(newSocket, &NETWORKmodulus, sizeof(NETWORKmodulus), 0)) == SOCKET_ERROR)
	{
		puts("Recv from client failed!");
	}
	puts("MODULUS received!");
	
	nanosleep(&nsTime, NULL);
	
	/*primi eksponent*/
	if ((recvSize = recv(newSocket, &NETWORKexponent, sizeof(NETWORKexponent), 0)) == SOCKET_ERROR)
	{
		puts("Recv from client failed!");
	}
	puts("EXPONENT received!");
	
	nanosleep(&nsTime, NULL);
	
	n = ntohl(NETWORKmodulus);
	publicKey = ntohl(NETWORKexponent);
	
	printf("\nPublic Key:\n\te[0]: %d\n\t(p * q): %d\n\n", publicKey, n);
}

/*slanje enkriptovanog fajla*/
void sendFile(char fs_name[])
{
	struct timespec nsTime;
	nsTime.tv_sec = 0;
	nsTime.tv_nsec = 50000000;
	
	char tempDir[BUFLEN];
	int64_t fileLentgh;
	int32_t blockSize, i;
	
	printf("Pre slanja imena fajla: %s\n", fs_name);
	
	nanosleep(&nsTime, NULL);
	
	/*slanje imena fajla (bez putanje)*/
	if(send(newSocket, fs_name, strlen(fs_name), 0) == SOCKET_ERROR)
	{
		printf("Name of the file send() FAILED!\n\n");
	}
	nanosleep(&nsTime, NULL);

	puts("Name of the file is sent\n\n");
	printf("size of name sent: %d\n", strlen(fs_name));
	
	/*ciscenje bafera*/
	memset(tempDir, 0, BUFLEN);
	
	/*dodavanje putanje*/
	strcpy(tempDir, "/mmc0:4/a/");
	
	//spajanje imena fajla i zeljene putanje
	strcat(tempDir, fs_name);
	printf("NAKON SPAJANJA PUTANJE I IMENA FAJLA: %s", tempDir);
	puts("");
	
	printf("Sending %s to the client... \n\n", tempDir);
	
	/*otvaranje fajla*/
	FILE *fs = fopen(tempDir, "rb");
	if(fs == NULL)
	{
		printf("ERROR: File %s not found.\n", tempDir);
	}
	
	/*trazenje velicine trenutnog fajla*/
	fseek(fs, 0, SEEK_END);
	fileLentgh = ftell(fs);
	fseek(fs, 0, SEEK_SET);
	
	printf("Velicina fajla koji se salje je: %lld\n\n\n", fileLentgh);
	
	/*slanje velicine tekuceg fajla*/
	long convertedNumber = htonl(fileLentgh);
	if(send(newSocket, &convertedNumber, sizeof(convertedNumber), 0) == SOCKET_ERROR)
	{
		printf("Name of the file send() FAILED!\n\n");
	}
	puts("Size of file is sent!\n");
	
	/*ciscenje bafera*/
	memset(sdbuf, NULL, BUFLEN); 
	memset(en, NULL, BUFLEN);
	
	/*citanje fajla*/
	while((blockSize = fread(sdbuf, sizeof(char), BUFLEN, fs)) != '\0')
	{
		nanosleep(&nsTime, NULL);
		
		printf("%d\t", blockSize);
		
		nanosleep(&nsTime, NULL);
		
		/*E N K R I P C I J A*/
		encrypt();
		
		//SLANJE FAJLA
		if(send(newSocket, en, blockSize, 0) < 0)
		{
			fprintf(stderr, "ERROR: Failed to send file %s. (errno = %d)\n", tempDir, errno);
			break;
		}
		
		nanosleep(&nsTime, NULL);
		
		/*validno slanje fajla, jer saljem bafer koji je popunjem int32_t vrednostima, pa moram slati jedan po jedan element, a ne sve zajedno u baferu*/
		for(i = 0; i < blockSize; i++)
		{
			en[i] = htonl(en[i]);
			send(newSocket, &en[i], sizeof(en[i]), 0);
		}
		
		/*ciscenje*/
		memset(sdbuf, NULL, BUFLEN); 
		memset(en, NULL, BUFLEN); 
		
	}
	
	printf("\n\nOk! File %s from server was sent!\n\n", tempDir);
	
	fclose(fs);
	
	nanosleep(&nsTime, NULL);
}

/*funkcija za brojanje fajlova u direktorijumu*/
int32_t numOfFiles()
{
	/*brojac fajlova*/
	int32_t fileCounter = 0;
	
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
				fileCounter++;
			}
		}
		
		closedir(dirp);
	}
	
	printf("Nalazim se u funkciji numOfFiles() i izbrojao sam %d fajlova\n\n", fileCounter);
	
	return fileCounter;
}

/*enkripcija*/
void encrypt()
{
	int32_t pt, k, iter, j;

	for(iter = 0; iter < BUFLEN; iter++)
	{
		//uzima vrednost trenutnog bajta
		pt = sdbuf[iter];
		k = 1;
		for (j = 0; j < publicKey; j++)
		{
			k = k * pt;
			k = k % n;
		}
		en[iter] = k;
	}
}