/*makro za izbacivanje printf() funkcije iz koda*/
#if 0
	#define PRINT(a) printf a
#else
	#define PRINT(a) (void)0
#endif

#include "MemMap.h"
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
#include <inttypes.h>

#define RTE_STOP_SEC_CTCDETHCOM_APPL_CODE
#define RTE_START_SEC_CTCDETHCOM_APPL_CODE

#define SWU_BR_SERVERPORT  29170
#define SWU_BR_CLIENTPORT  29171
/*#define SWUP_ZFAS_IP_ADDRESS "fd53:7cb8:383:3::4f"	
#define SWUP_MIB_ZR_IP_ADDRESS "fd53:7cb8:383:3::73"*/	
#define SOCKET_ERROR -1
#define BUFLEN 512
#define BACKLOG 10
#define NULL_POINTER (void*)(0)
#define htonl_num(n) (((((n) & 0x000000ffU)) << 24U) | \
                  ((((n) & 0x0000ff00U)) << 8U) | \
                  ((((n) & 0x00ff0000U)) >> 8U) | \
                  ((((n) & 0xff000000U)) >> 24U))

#define ntohl_num(n) (((((n) & 0xFFU)) << 24U) | \
                  ((((n) & 0xFF00U)) << 8U) | \
                  ((((n) & 0xFF0000U)) >> 8U) | \
                  ((((n) & 0xFF000000U)) >> 24U))

/*baferi za enkripciju*/
static uint8_t sdbuf[BUFLEN];
static uint32_t en[BUFLEN];

/*javni kljucevi za enkripciju*/
static uint64_t publicKey, n;

/**/
static MSG_Q_ID messages;
static TASK_ID task;

/*promenljive koje se koriste za komunikaciju sa racunarom*/
static struct sockaddr_in server, client;
static int32_t s, newSocket, c, recvSize;
static char replyBuffer[BUFLEN];

/*flag na osnovu koga se menja stanje*/
static int32_t changeState;	

/*poruke za komunikaciju*/
static char message[] = "Start";
static char respondOK[] = "Let's communicate!";
static char respondNotOK[] = "Communication breakdown...";

/*deklaracije funkcija*/
static void backgroundTask(void);
static void receivePublicKeys(void);
static void sendFile(const char fs_name[]);
static int32_t numOfFiles(void);
static void encrypt(void);


FUNC(void, RTE_CTCDETHCOM_APPL_CODE) REthComInit(void) 
{
	/**/
	messages = msgQCreate(BUFLEN, BUFLEN, MSG_Q_FIFO);
	changeState = 1;
	
	task = taskCreate("task1", 115, VX_FP_TASK, 0x8000U, (FUNCPTR)backgroundTask, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
	(void)taskActivate(task);
}


FUNC(void, RTE_CTCDETHCOM_APPL_CODE) REthComCyclic(void) /* PRQA S 0850 */ /* MD_MSR_19.8 */
{
	(void)msgQSend(messages, (char*)&changeState, sizeof(changeState), NO_WAIT, MSG_PRI_NORMAL); //msg = 1
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
				PRINT(("Could not create socket!\n"));
			}
			PRINT(("\n\n\nSocket created.\n"));
			
			(void)memset((char*)&server, 0, sizeof(server));
			server.sin_family = AF_INET;
			server.sin_addr.s_addr = INADDR_ANY;
			server.sin_port = htons(SWU_BR_SERVERPORT);
			
			/*bindovanje*/
			if( bind(s,(struct sockaddr*)&server , sizeof(server)) == SOCKET_ERROR)
			{
				PRINT(("Bind failed: %s\n", strerror(errno))); 
			}
			 
			PRINT(("Bind done!\n"));
			PRINT(("Listetning...\n"));
			
			/*slusanje*/
			(void)listen(s, BACKLOG);
			
			PRINT(("Waiting for incoming connections...\n\n\n\n\n"));
			
			c = sizeof(struct sockaddr_in);
			
			/*prihvatanje komunikacije*/
			newSocket = accept(s ,(struct sockaddr*)&client, &c);
			if (newSocket == SOCKET_ERROR)
			{
				PRINT(("Accept failed with error!\n"));
			}
			PRINT(("Connection accepted!\n"));
			
			/*primanje inicijalne poruke od racunara*/
			if ((recvSize = recv(newSocket, replyBuffer, BUFLEN, 0)) == SOCKET_ERROR)
			{
				PRINT(("Recv from client failed!\n"));
			}
			PRINT(("Reply received --------->\n"));

			/*terminacija stringa na kraju*/
			replyBuffer[recvSize] = '\0';
			PRINT(("%s\n", replyBuffer));
			PRINT(("\n\n"));
			
			/*uporedjivanje stringova*/
			if(strcmp(replyBuffer, message) == 0)
			{
				if(send(newSocket, respondOK, strlen(respondOK), 0) == SOCKET_ERROR)
				{
					PRINT(("First send() failed\n"));
				}
				PRINT(("Everything is fine, let's chat!\n\n\n"));
			}
			else
			{
				if(send(newSocket, respondNotOK, strlen(respondOK), 0) == SOCKET_ERROR)
				{
					PRINT(("Second send() failed\n"));
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
				PRINT(("Number of files send() FAILED!\n\n"));
			}
			PRINT(("Number of files is sent!\n\n"));
			
			DIR* dirp;
			struct dirent* direntp;
			/*ime fajla*/
			char tempStr[BUFLEN];
			
			/*putanja na kojoj se nalaze fajlovi koje najpre treba enkriptovati pa zatim i poslati racunaru*/
			dirp = opendir("/mmc0:4/a");
			if(dirp == NULL_POINTER)
			{
				PRINT(("Error opening dir!\n\n\n"));
			}
			else
			{
				for(;;)
				{
					/*otvaranje direktorijuma*/
					direntp = readdir(dirp);
					 
					if(direntp == NULL_POINTER)
					{
						break;
					}
					
					if(strcmp(direntp->d_name, ".") != 0)
					{
						if(strcmp(direntp->d_name, "..") != 0)
						{
							/*ciscenje bafera*/
							(void)memset(tempStr, 0, BUFLEN);
							
							/*kopiram ime fajla*/
							(void)strcpy(tempStr, direntp->d_name);

							PRINT(("U FUNKCIJI GDE CITAM IMENA FAJLOVA:"));
							PRINT(("%s\n", tempStr));
							
							/*funkcija u kojoj prvo posaljem ime fajla pa zatim i sam fajl*/
							sendFile(tempStr);
						}
						
					}
				}
				PRINT(("\n"));
				
				/*zatvaranje direktorijuma*/
				(void)closedir(dirp);
			}
			
			/*zatvaranje soketa*/
			(void)close(s);
			(void)close(newSocket);
			
			/*prelazak u finalno stanje*/
			(void)msgQSend(messages, (char*)&changeState, sizeof(changeState), NO_WAIT, MSG_PRI_NORMAL);
		}
		else
		{
			/*Do nothing...*/
		}
	}
}

/*primi javne kljuceve neophodne za enkripciju*/
static void receivePublicKeys(void)
{
	PRINT(("---------------------------------------------- receivePublicKeys\n"));
	
	/*vreme spavanja 0.05 sekundi*/
	struct timespec nsTime;
	nsTime.tv_sec = 0;
	nsTime.tv_nsec = 50000000;
	
	uint64_t NETWORKmodulus, NETWORKexponent;
	
	(void)nanosleep(&nsTime, NULL_POINTER);
	
	/*primi moduo*/
	recvSize = recv(newSocket, &NETWORKmodulus, sizeof(NETWORKmodulus), 0);
	if (recvSize == SOCKET_ERROR)
	{
		PRINT(("Recv from client failed!\n"));
	}
	PRINT(("MODULUS received!\n"));
	
	(void)nanosleep(&nsTime, NULL_POINTER);
	
	/*primi eksponent*/
	recvSize = recv(newSocket, &NETWORKexponent, sizeof(NETWORKexponent), 0);
	if (recvSize  == SOCKET_ERROR)
	{
		PRINT(("Recv from client failed!\n"));
	}
	PRINT(("EXPONENT received!\n"));
	
	(void)nanosleep(&nsTime, NULL_POINTER);
	
	n = ntohl_num(NETWORKmodulus);
	publicKey = ntohl_num(NETWORKexponent);
	
	PRINT(("\nPublic Key:\n\te[0]: %lld\n\t(p * q): %lld\n\n", publicKey, n));
}

/*slanje enkriptovanog fajla*/
static void sendFile(const char fs_name[])
{
	struct timespec nsTime;
	nsTime.tv_sec = 0;
	nsTime.tv_nsec = 50000000;
	
	char tempDir[BUFLEN];
	uint32_t blockSize, i, fileLentgh;
	
	PRINT(("Pre slanja imena fajla: %s\n", fs_name));
	
	(void)nanosleep(&nsTime, NULL_POINTER);
	
	/*slanje imena fajla (bez putanje)*/
	if(send(newSocket, fs_name, strlen(fs_name), 0) == SOCKET_ERROR)
	{
		PRINT(("Name of the file send() FAILED!\n\n"));
	}
	
	(void)nanosleep(&nsTime, NULL_POINTER);

	PRINT(("Name of the file is sent\n\n\n"));
	PRINT(("size of name sent: %d\n", strlen(fs_name)));
	
	/*ciscenje bafera*/
	(void)memset(tempDir, 0, BUFLEN);
	
	/*dodavanje putanje*/
	(void)strcpy(tempDir, "/mmc0:4/a/");
	
	//spajanje imena fajla i zeljene putanje
	(void)strcat(tempDir, fs_name);
	PRINT(("NAKON SPAJANJA PUTANJE I IMENA FAJLA: %s\n", tempDir));
	
	PRINT(("Sending %s to the client... \n\n", tempDir));
	
	/*otvaranje fajla*/
	FILE *fs = fopen(tempDir, "rb");
	if(fs == NULL_POINTER)
	{
		PRINT(("ERROR: File %s not found.\n", tempDir));
	}
	
	/*trazenje velicine trenutnog fajla*/
	(void)fseek(fs, 0, SEEK_END);
	fileLentgh = (uint32_t)ftell(fs);
	(void)fseek(fs, 0, SEEK_SET);
	
	PRINT(("Velicina fajla koji se salje je: %lld\n\n\n", fileLentgh));
	
	/*slanje velicine tekuceg fajla*/
	fileLentgh = htonl_num(fileLentgh);
	if(send(newSocket, &fileLentgh, sizeof(fileLentgh), 0) == SOCKET_ERROR)
	{
		PRINT(("Name of the file send() FAILED!\n\n"));
	}
	PRINT(("Size of file is sent!\n\n"));
	
	/*ciscenje bafera*/
	(void)memset(sdbuf, NULL, BUFLEN); 
	(void)memset(en, NULL, BUFLEN);
	
	/*citanje fajla*/
	blockSize = fread(sdbuf, sizeof(char), BUFLEN, fs);
	while(blockSize != (uint32_t)0)
	{
		(void)nanosleep(&nsTime, NULL_POINTER);
		
		PRINT(("%d\t", blockSize));
		
		(void)nanosleep(&nsTime, NULL_POINTER);
		
		/*E N K R I P C I J A*/
		encrypt();
		
		/*SLANJE FAJLA*/
		
		if(send(newSocket, en, blockSize, 0) == SOCKET_ERROR)
		{
			PRINT(("ERROR: Failed to send file %s!\n", tempDir));
			break;
		}
		
		(void)nanosleep(&nsTime, NULL_POINTER);
		
		/*validno slanje fajla, jer saljem bafer koji je popunjem int32_t vrednostima, pa moram slati jedan po jedan element, a ne sve zajedno u baferu*/
		for(i = 0; i < blockSize; i++)
		{
			en[i] = htonl_num(en[i]);
			(void)send(newSocket, &en[i], sizeof(en[i]), 0);
		}
		
		/*ciscenje*/
		(void)memset(sdbuf, NULL, BUFLEN); 
		(void)memset(en, NULL, BUFLEN); 
		
		blockSize = fread(sdbuf, sizeof(char), BUFLEN, fs);
		
	}
	
	PRINT(("\n\nOk! File %s from server was sent!\n\n", tempDir));
	
	(void)fclose(fs);
	
	(void)nanosleep(&nsTime, NULL_POINTER);
}

/*funkcija za brojanje fajlova u direktorijumu*/
static int32_t numOfFiles(void)
{
	/*brojac fajlova*/
	int32_t fileCounter = 0;
	
	DIR* dirp;
	struct dirent* direntp;
	
	dirp = opendir("/mmc0:4/a");
	if(dirp == NULL_POINTER)
	{
		PRINT(("Error opening dir!\n\n\n"));
	}
	else
	{
		for(;;)
		{
			direntp = readdir(dirp);
					 
			if(direntp == NULL_POINTER)
			{
				break;
			}
			
			/*broji sve fajlove osim "." i ".."*/
			if(strcmp(direntp->d_name, ".") != 0)
			{
				if(strcmp(direntp->d_name, "..") != 0)
				{
					fileCounter++;
				}
			}
		}
		
		(void)closedir(dirp);
	}
	
	PRINT(("Nalazim se u funkciji numOfFiles() i izbrojao sam %d fajlova\n\n", fileCounter));
	
	return fileCounter;
}

/*enkripcija*/
static void encrypt(void)
{
	uint32_t pt, iter, j, temp;
	uint64_t k;
	temp = BUFLEN;

	for(iter = 0; iter < temp; iter++)
	{
		//uzima vrednost trenutnog bajta
		pt = sdbuf[iter];
		k = 1;
		for (j = 0; j < publicKey; j++)
		{
			k = k * pt;
			k = k % n;
		}
		en[iter] = (uint32_t)k;
	}
}
