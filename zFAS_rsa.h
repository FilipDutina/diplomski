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
#include <math.h>

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


struct public_key_class 
{
	int64_t modulus;
	int64_t exponent;
};


//******************************************************************************************************************************************************************************
//******************************************************************************************************************************************************************************
int64_t rsa_modExp(int64_t b, int64_t e, int64_t m)
{
	if (/*b < 0 ||*/ e < 0 || m <= 0) 
	{
		exit(1);
		puts("PUKO SAM BRATE");
	}

	b = b % m;

	if (e == 0)
	{
		return 1;
	}
	if (e == 1)
	{
		return b;
	}
	if (e % 2 == 0) 
	{
		return (rsa_modExp(b * b % m, e / 2, m) % m);
	}
	if (e % 2 == 1) 
	{
		return (b * rsa_modExp(b, (e - 1), m) % m);
	}
}

uint8_t *rsa_encrypt(const int8_t *message, const uint32_t message_size, const struct public_key_class *publ)
{
	int64_t *encrypted = malloc(sizeof(int64_t)*message_size);
	if (encrypted == NULL) 
	{
		fprintf(stderr, "Error: Heap allocation failed.\n");
		return NULL;
	}
	int64_t i = 0;
	for (i = 0; i < message_size; i++) 
	{
		encrypted[i] = rsa_modExp(message[i], publ->exponent, publ->modulus);
	}


	return encrypted;
}