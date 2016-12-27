#include <stdio.h>	/* printf()��fprintf()��ɬ�� */
#include <sys/socket.h>	/* socket()��connect()��send()��recv()��ɬ�� */
#include <arpa/inet.h>	/* sockaddr_in��inet_addr()��ɬ�� */
#include <stdlib.h>	/* atoi()��ɬ�� */
#include <string.h>	/* memset()��ɬ�� */
#include <unistd.h>	/* close()��ɬ�� */

#define RCVBUFSIZE 32	/* �����Хåե��Υ����� */

void DieWithError(char *errorMessage);	/* ���顼�����ؿ� */

int main(int argc, char *argv[])
{
	int sock;			/* �����åȥǥ�������ץ� */
	struct sockaddr_in echoServAddr;/* �����������ФΥ��ɥ쥹 */
	unsigned short echoServPort;	/* �����������ФΥݡ����ֹ� */
	char *servIP;			/* �����Ф�IP���ɥ쥹��dotted-quad�� */
	char *echoString;	        /* �����������Ф���������ʸ���� */
	char echoBuffer[RCVBUFSIZE];	/* ������ʸ�����ѤΥХåե� */
	unsigned int echoStringLen;	/* ����������ʸ����Υ����� */
	int bytesRcvd, totalBytesRcvd;	/* ����recv()���ɤ߼����
					   �Х��ȿ������Х��ȿ� */

	if ((argc < 3) || (argc > 4))	/* �����ο�������������ǧ */
	{
		fprintf(stderr, "Usage: %s <Server IP> <Echo Word> [<Echo Port>}\n",
				argv[0]);
		exit(1);
	}

	servIP = argv[1];		/* 1���ܤΰ����������Ф�IP���ɥ쥹�ʥɥå�10��ɽ���� */
	echoString = argv[2];	/* 2���ܤΰ�����������ʸ���� */

	if (argc == 4)
		echoServPort = atoi(argv[3]);	/* ����Υݡ����ֹ椬����л��� */
	else
		echoServPort = 7;	/* 7�ϥ����������ӥ���well-known�ݡ����ֹ� */

	/* TCP�ˤ�뿮�����ι⤤���ȥ꡼�ॽ���åȤ���� */
	if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
		DieWithError("socket() failed");

	/* �����ФΥ��ɥ쥹��¤�Τ���� */
	memset(&echoServAddr, 0, sizeof(echoServAddr));		/* ��¤�Τ˥�������� */
	echoServAddr.sin_family = AF_INET;			/* ���󥿡��ͥåȥ��ɥ쥹�ե��ߥ� */
	echoServAddr.sin_addr.s_addr = inet_addr(servIP);	/* �����Ф�IP���ɥ쥹 */
	echoServAddr.sin_port = htons(echoServPort);		/* �����ФΥݡ����ֹ� */

	/* �����������Фؤ���³���Ω */
	if (connect(sock, (struct sockaddr *) &echoServAddr, sizeof(echoServAddr)) < 0)
		DieWithError("connect() failed");

	echoStringLen = strlen(echoString);	/* ���ϥǡ�����Ĺ����Ĵ�٤� */

	/* ʸ����򥵡��Ф����� */
	if (send(sock, echoString, echoStringLen, 0) != echoStringLen)
		DieWithError("send() sent a different number of bytes than expected");

	/* Ʊ��ʸ����򥵡��Ф������ */
	totalBytesRcvd = 0;
	printf("Received: ");	/* ���������줿ʸ�����ɽ�����뤿��ν��� */
	while (totalBytesRcvd < echoStringLen)
	{
		/* �Хåե���������ã����ޤǡʥ̥�ʸ���Ѥ�1�Х��Ȥ������
			�����Ф���Υǡ������������ */
		if ((bytesRcvd = recv(sock, echoBuffer, RCVBUFSIZE - 1, 0)) <= 0)
			DieWithError("recv() failed or connection closed prematurely");
		totalBytesRcvd += bytesRcvd;	/* ��Х��ȿ��ν��� */
		echoBuffer[bytesRcvd] = '\0' ;	/* ʸ����ν�λ */
		printf(echoBuffer);	/* �������Хåե���ɽ�� */
	}

	printf("\n");	/* �Ǹ�β��Ԥ���� */

	close(sock);
	exit(0);
}

