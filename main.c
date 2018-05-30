#include <stdio.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <inttypes.h>
#include <time.h>
#include <Windows.h>

#pragma comment(lib, "ws2_32.lib") //Winsock Library

#define SLEEP_TIME 15
#define SWU_BR_SERVERPORT  29170
#define SWU_BR_CLIENTPORT  29171
//#define SWUP_ZFAS_IP_ADDRESS "fd53:7cb8:383:3::4f"	//zFAS ploca
//#define SWUP_MIB_ZR_IP_ADDRESS "fd53:7cb8:383:3::73"	//PC
#define BUFLEN 512
#define zFAS_IPv4 "192.168.122.60"
//#define PC_IPv4 "192.168.122.40"
//#define PC2_IPv4 "192.168.122.88"

SOCKET s;

//static const uint8_t zfasAddr[16] = {0xfd, 0x53, 0x7c, 0xb8, 0x03, 0x83, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x4f};

void receiveFile();
void decrypt();


long fileLentgh;
uint32_t en[BUFLEN];
uint8_t sdbuf[BUFLEN];
int blockSize;

static uint32_t primes[] = {41, 43, 47, 53, 59, 61, 67, 71, 73, 79, 83, 89, 97};

uint32_t p, q, n, publicKey, a, b;

int main()
{
	//p = 41;
	//q = 43;

	time_t t;
	srand((unsigned)time(&t));

	a = rand() % 13;
	b = rand() % 13;

	//dva prosta broja nikada ne smeju imati istu vrednost
	if (a == b && a > 0)
	{
		a--;
	}
	else if (a == b && a == 0)
	{
		a++;
	}

	p = primes[a];
	q = primes[b];

	//da prvi prost broj uvek bude manji od drugog
	if (p > q)
	{
		uint32_t tempo = p;
		p = q;
		q = tempo;
	}

	printf("Primes are %d and %d\n\n", p, q);

	
	n = p * q;
	publicKey = 11;

	


	//printf("Private Key:\n Modulus: %lld\n Exponent: %lld\n\n", (long long)priv->modulus, (long long)priv->exponent);
	//printf("Public Key:\n Modulus: %lld\n Exponent: %lld\n\n", (long long)pub->modulus, (long long)pub->exponent);


	//variables
	WSADATA wsa;
	//SOCKET s;
	struct sockaddr_in server;
	char message[] = "Start";
	char respondOK[] = "Let's communicate!";
	char serverReply[BUFLEN];
	int recvSize;
	int numOfFilesToBeReceived;

	//connection functions
	printf("\nInitialising Winsock...");
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		printf("Failed. Error Code : %d", WSAGetLastError());
		return 1;
	}
	printf("Initialised.\n");


	if ((s = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
	{
		printf("Could not create socket : %d", WSAGetLastError());
	}
	printf("Socket created.\n");

	server.sin_family = AF_INET;
	server.sin_port = htons(SWU_BR_SERVERPORT);
	//server.sin6_addr = in6addr_any;
	//server.sin6_addr.s6_addr = SWUP_ZFAS_IP_ADDRESS;
	//memcpy(server.sin6_addr.s6_addr, zfasAddr, sizeof(zfasAddr));
	//inet_pton(AF_INET6, "::1", &(server.sin6_addr));
	server.sin_addr.s_addr = inet_addr(zFAS_IPv4);

	//Connect to remote server
	puts("Connect to remote server");
	if (connect(s, (struct sockaddr*)&server, sizeof(server)) < 0)
	{
		printf("Failed. Error Code : %d\n", WSAGetLastError());
		exit(EXIT_FAILURE);
	}
	puts("Connected");

	//Send some data
	if (send(s, message, strlen(message), 0) < 0)
	{
		puts("Send failed");
		exit(EXIT_FAILURE);
	}
	puts("Data sent\n");

	//Receive a reply from the server
	if ((recvSize = recv(s, serverReply, BUFLEN, 0)) == SOCKET_ERROR)
	{
		puts("recv failed");
	}
	puts("Reply received\n");

	//Add a NULL terminating character to make it a proper string before printing
	printf("server replay: %d\n", recvSize);
	serverReply[recvSize] = '\0';
	puts(serverReply);
	puts("\n");

	if (strcmp(serverReply, respondOK) == 0)
	{
		puts("We are connected now, CHEERS! :)\n\n");
		puts("*****************************************************************************");
	}

	//receive files-----------------------------------------------------------------------------------------------------------------------------------
	//------------------------------------------------------------------------------------------------------------------------------------------------

	//slanje javnih kljuceva
	long long NETWORKmodulus = htonl(n);
	long long NETWORKexponent = htonl(publicKey);

	Sleep(SLEEP_TIME);

	if (send(s, &NETWORKmodulus, sizeof(NETWORKmodulus), 0) == SOCKET_ERROR)
	{
		printf("MODULUS send() FAILED!\n\n");
	}
	puts("MODULUS is sent!\n");

	Sleep(SLEEP_TIME);

	if (send(s, &NETWORKexponent, sizeof(NETWORKexponent), 0) == SOCKET_ERROR)
	{
		printf("EXPONENT send() FAILED!\n\n");
	}
	puts("EXPONENT is sent!\n");

	Sleep(SLEEP_TIME);


	//primi broj fajlova
	if ((recvSize = recv(s, &numOfFilesToBeReceived, sizeof(numOfFilesToBeReceived), 0)) == SOCKET_ERROR)
	{
		puts("Num of files recv() failed");
	}
	numOfFilesToBeReceived = ntohl(numOfFilesToBeReceived);
	printf("Broj fajlova koje primam: %d\n\n\n", numOfFilesToBeReceived);

	for (int i = numOfFilesToBeReceived; i > 0; i--)
	{
		Sleep(SLEEP_TIME);
		printf("Primam fajl broj: %d\n", i);
		receiveFile();

		puts("\n\n");
	}

	puts("\n\n\n\n\n\n\n\t\t\t\t\t**********G  O  T  O  V  O**********\n\n\n\n\n");

	closesocket(s);
	WSACleanup();


	return 0;
}

void receiveFile()
{
	char fr_name[BUFLEN];
	unsigned char revbuf[BUFLEN];
	long long revbufLong[BUFLEN];
	int fr_block_sz = 0;
	int recvNameSize;
	int recvSizeSize;
	long sizeOfFile;
	int iter = 1;
	int i;
	int ceo;
	int ostatak;

	//ocisti revbuf i revbufLong
	memset(revbuf, 0, BUFLEN);
	memset(revbufLong, 0, BUFLEN);

	//primi ime fajla
	if ((recvNameSize = recv(s, fr_name, sizeof(fr_name), 0)) == SOCKET_ERROR)
	{
		puts("Name of the file recv() failed");
	}
	//ispisi primljenu duzinu
	printf("velicina imena fajla: %d\t---------->\t", recvNameSize);
	//skrati string
	fr_name[recvNameSize] = '\0';

	//ispisi ime fajla
	puts(fr_name);

	//otvori fajl za pisanje sa primnjenim imenom
	FILE *fr = fopen(fr_name, "wb");
	if (fr == NULL)
	{
		printf("File %s cannot be opened!\n", fr_name);
	}

	//primi velicinu fajla
	if ((recvSizeSize = recv(s, &sizeOfFile, sizeof(sizeOfFile), 0)) == SOCKET_ERROR)
	{
		puts("Name of the file recv() failed");
	}

	sizeOfFile = ntohl(sizeOfFile);

	printf("PRIMLJENA velicina fajla: %ld\n\n", sizeOfFile);


	//vrati na pocetak
	fseek(fr, 0, SEEK_SET);

	ceo = sizeOfFile / BUFLEN;	//ovde mi ostaje 255
	ostatak = sizeOfFile % BUFLEN;	//ovde mi ostaje ostatak (479)

	printf("ceo deo: %d * BUFLEN \t\t ostatak: %d\n\n", ceo, ostatak);


	//while u kom primam fajl
	while ((fr_block_sz = recv(s, revbuf, sizeof(revbuf), 0)) != 0)
	{
		Sleep(SLEEP_TIME);
		
		
		for (i = 0; i < fr_block_sz; i++)
		{
			recv(s, &en[i], sizeof(en[i]), 0);
			en[i] = ntohl(en[i]);
		}


		Sleep(SLEEP_TIME);


		decrypt();
		

		//*******************************************************************************************
		Sleep(SLEEP_TIME);
		//pisanje u fajl
		fwrite(sdbuf, sizeof(char), fr_block_sz, fr);
		Sleep(SLEEP_TIME);
		//ispisi velicinu primljenog paketa
		printf("%d\t", fr_block_sz);
		//ocisti revbuf
		memset(revbuf, 0, BUFLEN);
		memset(revbufLong, 0, BUFLEN);

		memset(sdbuf, 0, BUFLEN);
		memset(en, 0, BUFLEN);

		//PODESI MEMSETOVE I BROBAJ SA LONG BAFEROM


		if (fr_block_sz < sizeof(revbuf))
		{
			break;
		}
	}



	fclose(fr);
}

//*********************************************************************************
//*********************************************************************************

void decrypt()
{
	int i, j;
	int key = 611;
	int ct, k;

	for (i = 0; i < BUFLEN; i++)
	{
		//ovde ne valja!!!
		ct = en[i];
		k = 1;
		for (j = 0; j < key; j++)
		{
			k = k * ct;
			k = k % n;
		}
		sdbuf[i] = k;
	}
	sdbuf[i] = -1;
}