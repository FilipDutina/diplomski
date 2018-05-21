#include "zFAS_rsa.h"

//*********************************************************

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


//ovde ce mi se nalaziti javni kljucevi za enkripciju
struct public_key_class pub[1];

//FSM functions
void init();
void idle();
void dataCollection();
void dataEncryption();
void dataUpload();

//Task and Message functions
static void backgroundTask(void);


//OTHER FUNCTIONS
void receivePublicKeys();
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
			printf("\n\nSocket created.\n");
			
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
			puts("Waiting for incoming connections...\n\n\n\n");
			
			
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
			
			
			//primi javne kljuceve neophodne za enkripciju
			receivePublicKeys();
			
			
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
void receivePublicKeys()
{
	puts("---------------------------------------------- receivePublicKeys");
	
	struct timespec nsTime;
	nsTime.tv_sec = 0;
	nsTime.tv_nsec = 50000000;
	
	//Primi javne kljuceve
	long long NETWORKmodulus;
	long long NETWORKexponent;
	
	nanosleep(&nsTime, NULL);
	
	if ((recvSize = recv(newSocket, &NETWORKmodulus, sizeof(NETWORKmodulus), 0)) == SOCKET_ERROR)
	{
		puts("Recv from client failed!");
	}
	puts("MODULUS received!");
	
	nanosleep(&nsTime, NULL);
	
	if ((recvSize = recv(newSocket, &NETWORKexponent, sizeof(NETWORKexponent), 0)) == SOCKET_ERROR)
	{
		puts("Recv from client failed!");
	}
	puts("EXPONENT received!");
	
	nanosleep(&nsTime, NULL);
	
	
	pub->modulus = ntohl(NETWORKmodulus);
	pub->exponent = ntohl(NETWORKexponent);
	
	printf("\nPublic Key:\n Modulus: %lld\n Exponent: %lld\n\n", (long long)pub->modulus, (long long)pub->exponent);
}

void sendFile(char fs_name[])
{
	//vreme sleep-a
	struct timespec nsTime;
	nsTime.tv_sec = 0;
	nsTime.tv_nsec = 50000000;	//trebalo bi da je ovo 0.05 sekundi
	
	char tempDir[BUFLEN];
	long fileLentgh;
	unsigned char sdbuf[BUFLEN];
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
	
	//promenljiva u koju se smesta povratna vrenost enkripcije
	
	//int i;
	
	
	
	printf("\n\n\n\n\n\n char: %d, unsigned char: %d, int: %d, long long: %d \n\n\n\n\n\n\n", sizeof(char), sizeof(unsigned char), sizeof(int), sizeof(long long));
	
	
	
	puts("ispred while-a za slanje fajla\n\n");
	while((blockSize = fread(sdbuf, sizeof(char), BUFLEN, fs)) != '\0')
	{
		//sleep(1);
		
		nanosleep(&nsTime, NULL);
		//printf("%d\t", blockSize);
		
		//printf("\nPublic Key:\n Modulus: %lld\n Exponent: %lld\n\n", (long long)pub->modulus, (long long)pub->exponent);
		
		printf("%x ", sdbuf[0]);
		
		/*pre enkripcije fajlove cita identicno sto je dobro, medjutim nakon enkripcije poneti bajti postaju
		razliciti...nemam pojma zasto........................*/
		
		//E N K R I P C I A
		nanosleep(&nsTime, NULL);
		uint16_t *encrypted = rsa_encrypt(sdbuf, sizeof(sdbuf), pub);
		if (!encrypted) 
		{
			puts("\nEnkripcija nije uspela!\n");
		}
		
		printf("-%x-,                          ", encrypted[0]);
		
		/*for(i = 0; i < blockSize; i++)
		{
			printf("%x ", encrypted[i]);
		}
		
		puts("");*/
		
		/*for(i = 0; i < blockSize; i++)
		{
			sdbuf[i] = encrypted[i];
		}*/
		nanosleep(&nsTime, NULL);
		//SLANJE FAJLA
		if(send(newSocket, (unsigned char*)encrypted, blockSize, 0) < 0)
		{
			fprintf(stderr, "ERROR: Failed to send file %s. (errno = %d)\n", tempDir, errno);
			break;
		}
		memset(sdbuf, 0, BUFLEN); 
		nanosleep(&nsTime, NULL);
		
		free(encrypted);
	}
	printf("\n\nOk! File %s from server was sent!\n\n", tempDir);
	
	
	fclose(fs);
	
	nanosleep(&nsTime, NULL);
}

int numOfFiles()
{
	//brojac fajlova
	int itera = 0;
	
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
				itera++;
			}
		}
		
		closedir(dirp);
	}
	
	printf("Nalazim se u funkciji numOfFiles() i izbrojao sam %d fajlova\n\n", itera);
	
	return itera;
}