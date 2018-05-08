#include <stdio.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <inttypes.h>
#include <time.h>
#include <Windows.h>

#pragma comment(lib, "ws2_32.lib") //Winsock Library

#define SWU_BR_SERVERPORT  29170
#define SWU_BR_CLIENTPORT  29171
//#define SWUP_ZFAS_IP_ADDRESS "fd53:7cb8:383:3::4f"	//zFAS ploca
//#define SWUP_MIB_ZR_IP_ADDRESS "fd53:7cb8:383:3::73"	//PC
#define BUFLEN 512
#define zFAS_IPv4 "192.168.122.60"
//#define PC_IPv4 "192.168.122.40"
//#define PC2_IPv4 "192.168.122.88"

//static const uint8_t zfasAddr[16] = {0xfd, 0x53, 0x7c, 0xb8, 0x03, 0x83, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x4f};

int main()
{
	//variables
	WSADATA wsa;
	SOCKET s;
	struct sockaddr_in server;
	char message[] = "Start";
	char respondOK[] = "Let's communicate!";
	char serverReply[BUFLEN];
	int recvSize;

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
	serverReply[recvSize] = '\0';
	puts(serverReply);
	puts("\n");

	if (strcmp(serverReply, respondOK) == 0)
	{
		puts("We are connected now, CHEERS! :)\n\n");
	}

	//receive file---------------------------------------------------------------------
	//---------------------------------------------------------------------------------

	char* fr_name = "bmp.bmp";
	char revbuf[BUFLEN];
	int fr_block_sz = 0;
	FILE *fr = fopen(fr_name, "wb");

	if (fr == NULL)
	{
		printf("File %s cannot be opened!\n", fr_name);
	}
	

	memset(revbuf, 0, BUFLEN);
	int i;

	fseek(fr, 0, SEEK_SET);
	
	while((fr_block_sz = recv(s, revbuf, sizeof(revbuf), 0)) != 0)
	{
		Sleep(10);
		fwrite(revbuf, sizeof(char), fr_block_sz, fr);
		Sleep(10);

		for (i = 0; i < sizeof(revbuf); i++)
		{
			if (revbuf[i] == EOF)
			{
				break;
				break;
			}
		}

		printf("%d\t", fr_block_sz);
		
		memset(revbuf, 0, BUFLEN);
		
	}
	puts("");
	puts("izasao iz while-a\n");



	fclose(fr);


	closesocket(s);
	WSACleanup();

	return 0;
}
