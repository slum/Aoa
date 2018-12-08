//
// PING.C -- ICMP���������åȤ�ʹ�ä���Ping�ץ����
//

#include <stdio.h>
#include <stdlib.h>
#include <winsock.h>

#include "ping.h"

// �ڲ��v��
void ReportError(LPCSTR pstrFrom);
int  WaitForEchoReply(SOCKET s);
u_short in_cksum(u_short *addr, int len);

// ICMP�����`Ҫ��/����v��
int		SendEchoRequest(SOCKET, LPSOCKADDR_IN);
DWORD	RecvEchoReply(SOCKET, LPSOCKADDR_IN, u_char *);

// Ping()
// SendEchoRequest()��RecvEchoReply()��
// ���ӳ����ƽY�����������
BOOL Ping(LPCSTR pstrHost)
{
	SOCKET	  rawSocket;
	LPHOSTENT lpHost;
	struct    sockaddr_in saDest;
	struct    sockaddr_in saSrc;
	DWORD	  dwTimeSent;
	DWORD	  dwElapsed;
	u_char    cTTL;
	int       nLoop;
	int       nRet;
	WSADATA wsaData;
	WORD wVersionRequested = MAKEWORD(1, 1);

	// WinSock�γ��ڻ�
	nRet = WSAStartup(wVersionRequested, &wsaData);
	if (nRet)
	{
		fprintf(stderr, "\nError initializing WinSock\n");
		return FALSE;
	}

	// �Щ`�����Υ����å�
	if (wsaData.wVersion != wVersionRequested)
	{
		fprintf(stderr, "\nWinSock version not supported\n");
		WSACleanup();
		return FALSE;
	}

	// WinSock�γ��ڻ�
	nRet = WSAStartup(wVersionRequested, &wsaData);
	if (nRet)
	{
		fprintf(stderr, "\nError initializing WinSock\n");
		WSACleanup();
		return FALSE;
	}

	// �������åȤ�����
	rawSocket = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
	if (rawSocket == SOCKET_ERROR)
	{
		ReportError("socket()");
		WSACleanup();
		return FALSE;
	}

	// �ۥ��ȤΗ���
	lpHost = (LPHOSTENT)gethostbyname(pstrHost);
	if (lpHost == NULL)
	{
		fprintf(stderr, "\nHost not found: %s\n", pstrHost);
		WSACleanup();
		return FALSE;
	}

	// ���ȥ����åȥ��ɥ쥹���O��
	saDest.sin_addr.s_addr = *((u_long FAR *) (lpHost->h_addr));
	saDest.sin_family = AF_INET;
	saDest.sin_port = 0;

	// ����״�r���`���`�˱�ʾ
	//printf("\nPinging %s [%s] with %d bytes of data:\n",
	//	pstrHost,
	//	inet_ntoa(saDest.sin_addr),
	//	REQ_DATASIZE);

	// Ping��ζȤ��g��
	for (nLoop = 0; nLoop < 1; nLoop++)
	{
		// ICMP�����`Ҫ�������
		SendEchoRequest(rawSocket, &saDest);

		// select()��ʹ�ä��ƥǩ`�������Ť���C
		nRet = WaitForEchoReply(rawSocket);
		if (nRet == SOCKET_ERROR)
		{
			ReportError("select()");
			WSACleanup();
			return FALSE;
		}
		if (!nRet)
		{
			printf("\nTimeOut");
			WSACleanup();
			return FALSE;
		}

		// ��������
		dwTimeSent = RecvEchoReply(rawSocket, &saSrc, &cTTL);

		// �U�^�r�g��Ӌ��
		//dwElapsed = GetTickCount() - dwTimeSent;
		//printf("\nReply from: %s: bytes=%d time=%ldms TTL=%d",
		//	inet_ntoa(saSrc.sin_addr),
		//	REQ_DATASIZE,
		//	dwElapsed,
		//	cTTL);
	}
	printf("\n");
	nRet = closesocket(rawSocket);
	if (nRet == SOCKET_ERROR)
		ReportError("closesocket()");

	// WinSock����
	WSACleanup();

	return TRUE;
}


// SendEchoRequest()
// �����`Ҫ��إå��`������
// �O���������Ȥ����Ť���
int SendEchoRequest(SOCKET s, LPSOCKADDR_IN lpstToAddr)
{
	static ECHOREQUEST echoReq;
	static int nId = 1;
	static int nSeq = 1;
	int nRet;

	// �����`Ҫ��������O��
	echoReq.icmpHdr.Type = ICMP_ECHOREQ;
	echoReq.icmpHdr.Code = 0;
	echoReq.icmpHdr.Checksum = 0;
	echoReq.icmpHdr.ID = nId++;
	echoReq.icmpHdr.Seq = nSeq++;

	// ���ťǩ`�����O��
	for (nRet = 0; nRet < REQ_DATASIZE; nRet++)
		echoReq.cData[nRet] = ' ' + nRet;

	// ���ŕr�Υƥ��å�������Ȥ򱣴�
	echoReq.dwTime = GetTickCount();

	// �ѥ��å��ڤ˥ǩ`������졢�����å������Ӌ��
	echoReq.icmpHdr.Checksum = in_cksum((u_short *)&echoReq, sizeof(ECHOREQUEST));

	// �����`Ҫ�������
	nRet = sendto(s,						// �����å�
		(LPSTR)&echoReq,			// �Хåե�
		sizeof(ECHOREQUEST),
		0,							// �ե饰
		(LPSOCKADDR)lpstToAddr, // ����
		sizeof(SOCKADDR_IN));   // ���ɥ쥹���L��

	if (nRet == SOCKET_ERROR)
		ReportError("sendto()");
	return (nRet);
}


// RecvEchoReply()
// ���ťǩ`�������Ť��ơ�
// �ե��`��Ʉe�˽�������
DWORD RecvEchoReply(SOCKET s, LPSOCKADDR_IN lpsaFrom, u_char *pTTL)
{
	ECHOREPLY echoReply;
	int nRet;
	int nAddrLen = sizeof(struct sockaddr_in);

	// �����`��������
	nRet = recvfrom(s,					// �����å�
		(LPSTR)&echoReply,	// �Хåե�
		sizeof(ECHOREPLY),	// �Хåե��Υ�����
		0,					// �ե饰
		(LPSOCKADDR)lpsaFrom,	// ����Ԫ���ɥ쥹
		&nAddrLen);			// ���ɥ쥹�L�ؤΥݥ���

							// ���ꂎ������å�
	if (nRet == SOCKET_ERROR)
		ReportError("recvfrom()");

	// ���ŕr��IP TTL����A�r�g���򷵤�
	*pTTL = echoReply.ipHdr.TTL;
	return(echoReply.echoRequest.dwTime);
}

// �k�����������Έ��
void ReportError(LPCSTR pWhere)
{
	fprintf(stderr, "\n%s error: %d\n",
		WSAGetLastError());
}


// WaitForEchoReply()
// select()��ʹ�ä��ơ��ǩ`����
// �i��ȡ����C�Ф��ɤ������Єe����
int WaitForEchoReply(SOCKET s)
{
	struct timeval Timeout;
	fd_set readfds;

	readfds.fd_count = 1;
	readfds.fd_array[0] = s;
	Timeout.tv_sec = 5;
	Timeout.tv_usec = 0;

	return(select(1, &readfds, NULL, NULL, &Timeout));
}


//
// Mike Muuss' in_cksum() function
// and his comments from the original
// ping program
//
// * Author -
// *	Mike Muuss
// *	U. S. Army Ballistic Research Laboratory
// *	December, 1983

/*
*			I N _ C K S U M
*
* Checksum routine for Internet Protocol family headers (C Version)
*
*/
u_short in_cksum(u_short *addr, int len)
{
	register int nleft = len;
	register u_short *w = addr;
	register u_short answer;
	register int sum = 0;

	/*
	*  Our algorithm is simple, using a 32 bit accumulator (sum),
	*  we add sequential 16 bit words to it, and at the end, fold
	*  back all the carry bits from the top 16 bits into the lower
	*  16 bits.
	*/
	while (nleft > 1) {
		sum += *w++;
		nleft -= 2;
	}

	/* mop up an odd byte, if necessary */
	if (nleft == 1) {
		u_short	u = 0;

		*(u_char *)(&u) = *(u_char *)w;
		sum += u;
	}

	/*
	* add back carry outs from top 16 bits to low 16 bits
	*/
	sum = (sum >> 16) + (sum & 0xffff);	/* add hi 16 to low 16 */
	sum += (sum >> 16);			/* add carry */
	answer = ~sum;				/* truncate to 16 bits */
	return (answer);
}