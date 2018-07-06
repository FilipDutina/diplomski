/*makro za izbacivanje printf() funkcije iz koda*/
#if 1
#define PRINT(a) printf a
#else
#define PRINT(a) (void)0
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <inttypes.h>
#include <time.h>
#include <math.h>
#include <Windows.h>

/*Winsock Library*/
#pragma comment(lib, "ws2_32.lib") 

#define SLEEP_TIME 15
#define BUFLEN 512
#define NUM_OF_KEYS 22
#define LOOP_STOP 99
#define SWU_BR_SERVERPORT  29170
#define SWU_BR_CLIENTPORT  29171
/*#define SWUP_ZFAS_IP_ADDRESS "fd53:7cb8:383:3::4f"
#define SWUP_MIB_ZR_IP_ADDRESS "fd53:7cb8:383:3::73"*/

#define zFAS_IPv4 "192.168.122.60"

/*deklaracije funkcija*/
void receiveFile();
void decrypt();
uint32_t prime(uint32_t pr);
void ce();
uint32_t cd(uint32_t x);


/*soket*/
SOCKET s;
/*bafer u koji primam enkriptovanu poruku*/
uint32_t en[BUFLEN];
/*izlazni bafer sa dekriptovanom vrednoscu*/
uint8_t sdbuf[BUFLEN];
/*privatni i javni kljucevi*/
uint32_t e[BUFLEN], d[BUFLEN];
/**/
uint32_t p, q, n, a, b, flag, phi, i, j, numOfEncAndDec;
/*niz prostih brojeva koji se koriste za enkripciju*/
static uint32_t primes[] = { 41, 43, 47, 53, 59, 61, 67, 71, 73, 79, 83, 89, 97 };

int main()
{
	time_t randTime;
	srand((unsigned)time(&randTime));

	a = rand() % (sizeof(primes) / sizeof(uint32_t));
	b = rand() % (sizeof(primes) / sizeof(uint32_t));

	/*dva prosta broja nikada ne smeju imati istu vrednost*/
	if (a == b && a > 0)
	{
		a--;
	}
	else if (a == b && a == 0)
	{
		a++;
	}

	p = primes[a];

	flag = prime(p);
	if (flag == 0)
	{
		PRINT(("\nWRONG INPUT 1\n"));
		exit(1);
	}

	q = primes[b];

	flag = prime(q);
	if (flag == 0 || p == q)
	{
		PRINT(("\nWRONG INPUT 2\n"));
		exit(1);
	}

	/*prvi prost broj treba da bude manji od drugog*/
	if (p > q)
	{
		uint32_t tempo = p;
		p = q;
		q = tempo;
	}

	PRINT(("Primes are -%d- and -%d-, and p*q is -%d-\n\n", p, q, p*q));

	n = p * q;
	phi = (p - 1)*(q - 1);

	ce();

	PRINT(("POSSIBLE VALUES OF e AND d ARE:"));
	for (i = 0; i < j - 1; i++)
	{
		PRINT(("\n%d.\t%d\t%ld", i, e[i], d[i]));
	}
	PRINT(("\n\n"));

	/*izmedju 0 i 22*/
	numOfEncAndDec = rand() % NUM_OF_KEYS;

	PRINT(("numOfEncAndDec is: %d\n", numOfEncAndDec));
	PRINT(("e[%d] = %d, d[%d] = %d\n\n", numOfEncAndDec, e[numOfEncAndDec], numOfEncAndDec, d[numOfEncAndDec]));

	/*promenljive koje se koriste za komunikaciju*/
	WSADATA wsa;
	struct sockaddr_in server;
	uint8_t message[] = "Start";
	uint8_t folder[] = "a";
	uint8_t respondOK[] = "Let's communicate!";
	uint8_t serverReply[BUFLEN];
	int32_t recvSize;
	int32_t numOfFilesToBeReceived;

	/*funkcije za konekciju*/
	PRINT(("\nInitialising Winsock..."));
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		PRINT(("Failed. Error Code : %d", WSAGetLastError()));
		return 1;
	}
	PRINT(("Initialised.\n"));

	if ((s = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
	{
		PRINT(("Could not create socket : %d", WSAGetLastError()));
	}
	PRINT(("Socket created.\n"));

	server.sin_family = AF_INET;
	server.sin_port = htons(SWU_BR_SERVERPORT);
	server.sin_addr.s_addr = inet_addr(zFAS_IPv4);

	/*konekcija na zFAS plocu*/
	PRINT(("Connect to remote server\n"));
	if (connect(s, (struct sockaddr*)&server, sizeof(server)) < 0)
	{
		PRINT(("Failed. Error Code : %d\n", WSAGetLastError()));
		exit(EXIT_FAILURE);
	}
	PRINT(("Connected"));

	/*slanje inicijalne poruke*/
	if (send(s, message, strlen(message), 0) < 0)
	{
		PRINT(("Send failed\n"));
		exit(EXIT_FAILURE);
	}
	PRINT(("Data sent\n\n"));

	/*primanje odgovora od zFAS ploce*/
	if ((recvSize = recv(s, serverReply, BUFLEN, 0)) == SOCKET_ERROR)
	{
		PRINT(("recv failed\n"));
	}
	PRINT(("Reply received\n\n"));

	/*dodavanje NULL terminatora na kraj stringa*/
	PRINT(("server replay: %d\n", recvSize));
	serverReply[recvSize] = '\0';
	PRINT(("%s\n", serverReply));
	PRINT(("\n\n"));

	if (strcmp(serverReply, respondOK) == 0)
	{
		PRINT(("We are connected now, CHEERS!\n\n\n"));
		PRINT(("*****************************************************************************\n"));
	}

	/*slanje javnih kljuceva ploci*/
	uint64_t NETWORKmodulus = htonl(n);
	uint64_t NETWORKexponent = htonl(e[numOfEncAndDec]);

	Sleep(SLEEP_TIME);

	if (send(s, &NETWORKmodulus, sizeof(NETWORKmodulus), 0) == SOCKET_ERROR)
	{
		PRINT(("MODULUS send() FAILED!\n\n"));
	}
	PRINT(("MODULUS is sent!\n\n"));

	Sleep(SLEEP_TIME);

	if (send(s, &NETWORKexponent, sizeof(NETWORKexponent), 0) == SOCKET_ERROR)
	{
		PRINT(("EXPONENT send() FAILED!\n\n"));
	}
	PRINT(("EXPONENT is sent!\n\n"));

	Sleep(SLEEP_TIME);

	/*slanje zeljenog foldera*/
	if (send(s, folder, strlen(folder), 0) < 0)
	{
		PRINT(("Send failed\n"));
		exit(EXIT_FAILURE);
	}
	PRINT(("Folder sent: %s\n\n", folder));

	/*primanje broja fajlova koje ploca salje*/
	recvSize = recv(s, &numOfFilesToBeReceived, sizeof(numOfFilesToBeReceived), 0);
	if (recvSize == SOCKET_ERROR)
	{
		PRINT(("Num of files recv() failed"));
	}
	numOfFilesToBeReceived = ntohl(numOfFilesToBeReceived);
	PRINT(("Broj fajlova koje primam: %d\n\n\n", numOfFilesToBeReceived));

	/*for petlja u kojoj pozivam funkciju receiveFile() za svaki fajl posebno*/
	for (int i = numOfFilesToBeReceived; i > 0; i--)
	{
		Sleep(SLEEP_TIME);

		PRINT(("Primam fajl broj: %d\n", i));

		Sleep(SLEEP_TIME);

		receiveFile();

		PRINT(("\n\n\n"));
	}

	PRINT(("\n\n\n\n\n\n\n\n\t\t\t\t\t**********G  O  T  O  V  O**********\n\n\n\n\n\n"));

	/*zatvaranje soketa*/
	closesocket(s);
	WSACleanup();

	return 0;
}

/*DEFINICIJE FUNKCIJA*/

/*funkcija za primanje fajla*/
void receiveFile()
{
	uint8_t fr_name[BUFLEN];
	uint8_t revbuf[BUFLEN];
	uint32_t fr_block_sz = 0;
	int32_t recvNameSize;
	int32_t recvSizeSize;
	int64_t sizeOfFile;
	int32_t iter = 1;
	int32_t ceo;
	int32_t ostatak;

	/*ciscenje bafera*/
	memset(revbuf, 0, BUFLEN);

	/*primanje imena fajla*/
	if ((recvNameSize = recv(s, fr_name, sizeof(fr_name), 0)) == SOCKET_ERROR)
	{
		PRINT(("Name of the file recv() failed\n"));
	}
	PRINT(("velicina imena fajla: %d\t---------->\t", recvNameSize));

	/*skracivanje stringa*/
	fr_name[recvNameSize] = '\0';

	PRINT(("%s\n", fr_name));

	/*otvaranje fajla za pisanje sa primnjenim imenom*/
	FILE *fr = fopen(fr_name, "wb");
	if (fr == NULL)
	{
		PRINT(("File %s cannot be opened!\n", fr_name));
	}

	/*primanje velicine fajla*/
	recvSizeSize = recv(s, &sizeOfFile, sizeof(sizeOfFile), 0);
	if (recvSizeSize == SOCKET_ERROR)
	{
		PRINT(("Name of the file recv() failed\n"));
	}

	sizeOfFile = ntohl(sizeOfFile);

	/*PRINT(("PRIMLJENA velicina fajla: %lld\n\n", sizeOfFile));*/

	/*vracanje na pocetak fajla*/
	fseek(fr, 0, SEEK_SET);

	/*racunanje celog dela i ostatka datog fajla*/
	ceo = sizeOfFile / BUFLEN;
	ostatak = sizeOfFile % BUFLEN;

	PRINT(("ceo deo: %d * BUFLEN \t\t ostatak: %d\n\n", ceo, ostatak));

	/*petlja u kojoj se prima fajl*/
	while ((fr_block_sz = recv(s, revbuf, sizeof(revbuf), 0)) != 0)
	{
		Sleep(SLEEP_TIME);

		/*ovde pristizu podaci koji su enkriptovani*/
		for (i = 0; i < fr_block_sz; i++)
		{
			recv(s, &en[i], sizeof(en[i]), 0);
			en[i] = ntohl(en[i]);
		}

		Sleep(SLEEP_TIME);

		/*dekripcija*/
		decrypt();

		Sleep(SLEEP_TIME);

		/*pisanje u fajl dekriptovanih podataka*/
		fwrite(sdbuf, sizeof(char), fr_block_sz, fr);

		Sleep(SLEEP_TIME);

		/*velicina primljenog paketa*/
		PRINT(("%d\t", fr_block_sz));

		/*ciscenje bafera za naredno koriscenje*/
		memset(revbuf, 0, BUFLEN);
		memset(sdbuf, 0, BUFLEN);
		memset(en, 0, BUFLEN);

		/*provera da li je stigao poslednji paket; ako jeste izadji iz petlje*/
		if (fr_block_sz < sizeof(revbuf))
		{
			break;
		}
	}

	/*zatvaranje primljenog fajla*/
	fclose(fr);
}

/*funkcija za dekripciju*/
void decrypt()
{
	uint32_t key = d[numOfEncAndDec];
	uint32_t ct, k;

	for (i = 0; i < BUFLEN; i++)
	{
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

/*funckija u kojoj se proverava da li je broj prost*/
uint32_t prime(uint32_t pr)
{
	uint32_t iter2;
	j = (uint32_t)sqrt(pr);

	for (iter2 = 2; iter2 <= j; iter2++)
	{
		if (pr % iter2 == 0)
		{
			return 0;
		}
	}
	return 1;
}

/*funkcija u kojoj se racunaju privatni i javni kljucevi*/
void ce()
{
	uint32_t k = 0;

	for (i = 2; i < phi; i++)
	{
		if (phi % i == 0)
		{
			continue;
		}

		flag = prime(i);

		if (flag == 1 && i != p && i != q)
		{
			e[k] = i;
			flag = cd(e[k]);

			if (flag > 0)
			{
				d[k] = flag;
				k++;
			}

			if (k == LOOP_STOP)
			{
				break;
			}
		}
	}
}

/*pomocna funkcija za racunanje privatnih kljuceva*/
uint32_t cd(uint32_t x)
{
	uint32_t k = 1;

	while (1)
	{
		k = k + phi;

		if (k % x == 0)
		{
			return(k / x);
		}
	}
}

