#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include <Windows.h>
#include <inttypes.h>

#define BUFLEN 256
#define SLEEP_TIME 15

/* Refer to http://www.coders-hub.com/2013/04/c-code-to-encrypt-and-decrypt-message.html#.Vhs81MryNC1 */

long int p, q, n, t, e[BUFLEN], d[BUFLEN], j, i;
char msg[] = "Tekst koji zelim da enkriptujem/dekriptujem...";
int prime(long int);
void ce();
long int cd(long int);
void encrypt();
void decrypt();

long fileLentgh;
uint32_t en[BUFLEN];
uint8_t sdbuf[BUFLEN];
int blockSize;

int main()
{
	//printf("ORIGINAL MESSAGE IS\n%s", msg);
	p = 41;
	//flag = prime(p);
	q = 43;
	//flag = prime(q);

	/*for (i = 0; msg[i] != '\0'; i++)
	{
		m[i] = msg[i];
	}*/
	n = p * q;
	//t = (p - 1)*(q - 1);

	//FILE *fs = fopen("srv.jpg", "rb");
	FILE *fs = fopen("jednostavno.txt", "rb");
	/*if (fs == NULL)
	{
		printf("ERROR: File not found!!!\n");
	}*/

	//FILE *fr = fopen("srv_pokusaj.jpg", "wb");
	FILE *fr = fopen("tekst.txt", "wb");
	/*if (fr == NULL)
	{
		printf("ERROR: File not found!!!\n");
	}*/

	//idi do kraja
	fseek(fs, 0, SEEK_END);
	//nadji velicinu fajla
	fileLentgh = ftell(fs);
	//vrati na pocetak
	fseek(fs, 0, SEEK_SET);

	printf("Velicina fajla koji se salje je: %ld\n\n\n", fileLentgh);

	//ocisti sdbuf
	memset(sdbuf, NULL, BUFLEN);


	puts("Radim...");

	while ((blockSize = fread(sdbuf, sizeof(char), BUFLEN, fs)) != '\0')
	{
		Sleep(SLEEP_TIME);
		encrypt();

		Sleep(SLEEP_TIME);
		decrypt();

		Sleep(SLEEP_TIME);
		fwrite(sdbuf, sizeof(char), blockSize, fr);

		Sleep(SLEEP_TIME);
		memset(sdbuf, NULL, BUFLEN);

		Sleep(SLEEP_TIME);
	}

	
	fclose(fs);
	fclose(fr);





	return 0;
}

//*********************************************************************************
//*********************************************************************************

void encrypt()
{
	int key = 11;
	int pt, k;

	for(i = 0; i < BUFLEN; i++)
	{
		//uzima vrednost trenutnog bajta
		pt = sdbuf[i];
		k = 1;
		for (j = 0; j < key; j++)
		{
			k = k * pt;
			k = k % n;
		}
		en[i] = k;
	}
}

//*********************************************************************************
//*********************************************************************************

void decrypt()
{
	int key = 611;
	int ct, k;
	char temp;

	for(i = 0; i < BUFLEN; i++)
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