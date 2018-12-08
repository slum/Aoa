//
// PING.C -- ICMPと生ソケットを使用するPingプログラム
//

#include <stdio.h>
#include <stdlib.h>
#include <winsock.h>

#include "ping.h"

// 内部関数
void ReportError(LPCSTR pstrFrom);
int  WaitForEchoReply(SOCKET s);
u_short in_cksum(u_short *addr, int len);

// ICMPエコー要求/応答関数
int		SendEchoRequest(SOCKET, LPSOCKADDR_IN);
DWORD	RecvEchoReply(SOCKET, LPSOCKADDR_IN, u_char *);

// Ping()
// SendEchoRequest()とRecvEchoReply()を
// 呼び出して結果を出力する
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

	// WinSockの初期化
	nRet = WSAStartup(wVersionRequested, &wsaData);
	if (nRet)
	{
		fprintf(stderr, "\nError initializing WinSock\n");
		return FALSE;
	}

	// バージョンのチェック
	if (wsaData.wVersion != wVersionRequested)
	{
		fprintf(stderr, "\nWinSock version not supported\n");
		WSACleanup();
		return FALSE;
	}

	// WinSockの初期化
	nRet = WSAStartup(wVersionRequested, &wsaData);
	if (nRet)
	{
		fprintf(stderr, "\nError initializing WinSock\n");
		WSACleanup();
		return FALSE;
	}

	// 生ソケットの作成
	rawSocket = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
	if (rawSocket == SOCKET_ERROR)
	{
		ReportError("socket()");
		WSACleanup();
		return FALSE;
	}

	// ホストの検索
	lpHost = (LPHOSTENT)gethostbyname(pstrHost);
	if (lpHost == NULL)
	{
		fprintf(stderr, "\nHost not found: %s\n", pstrHost);
		WSACleanup();
		return FALSE;
	}

	// 宛先ソケットアドレスの設定
	saDest.sin_addr.s_addr = *((u_long FAR *) (lpHost->h_addr));
	saDest.sin_family = AF_INET;
	saDest.sin_port = 0;

	// 動作状況をユーザーに表示
	//printf("\nPinging %s [%s] with %d bytes of data:\n",
	//	pstrHost,
	//	inet_ntoa(saDest.sin_addr),
	//	REQ_DATASIZE);

	// Pingを何度か実行
	for (nLoop = 0; nLoop < 1; nLoop++)
	{
		// ICMPエコー要求を送信
		SendEchoRequest(rawSocket, &saDest);

		// select()を使用してデータの受信を待機
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

		// 応答を受信
		dwTimeSent = RecvEchoReply(rawSocket, &saSrc, &cTTL);

		// 経過時間を計算
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

	// WinSockを解放
	WSACleanup();

	return TRUE;
}


// SendEchoRequest()
// エコー要求ヘッダーに情報を
// 設定し、宛先に送信する
int SendEchoRequest(SOCKET s, LPSOCKADDR_IN lpstToAddr)
{
	static ECHOREQUEST echoReq;
	static int nId = 1;
	static int nSeq = 1;
	int nRet;

	// エコー要求に情報を設定
	echoReq.icmpHdr.Type = ICMP_ECHOREQ;
	echoReq.icmpHdr.Code = 0;
	echoReq.icmpHdr.Checksum = 0;
	echoReq.icmpHdr.ID = nId++;
	echoReq.icmpHdr.Seq = nSeq++;

	// 送信データを設定
	for (nRet = 0; nRet < REQ_DATASIZE; nRet++)
		echoReq.cData[nRet] = ' ' + nRet;

	// 送信時のティックカウントを保存
	echoReq.dwTime = GetTickCount();

	// パケット内にデータを入れ、チェックサムを計算
	echoReq.icmpHdr.Checksum = in_cksum((u_short *)&echoReq, sizeof(ECHOREQUEST));

	// エコー要求を送信
	nRet = sendto(s,						// ソケット
		(LPSTR)&echoReq,			// バッファ
		sizeof(ECHOREQUEST),
		0,							// フラグ
		(LPSOCKADDR)lpstToAddr, // 宛先
		sizeof(SOCKADDR_IN));   // アドレスの長さ

	if (nRet == SOCKET_ERROR)
		ReportError("sendto()");
	return (nRet);
}


// RecvEchoReply()
// 着信データを受信して、
// フィールド別に解析する
DWORD RecvEchoReply(SOCKET s, LPSOCKADDR_IN lpsaFrom, u_char *pTTL)
{
	ECHOREPLY echoReply;
	int nRet;
	int nAddrLen = sizeof(struct sockaddr_in);

	// エコー応答を受信
	nRet = recvfrom(s,					// ソケット
		(LPSTR)&echoReply,	// バッファ
		sizeof(ECHOREPLY),	// バッファのサイズ
		0,					// フラグ
		(LPSOCKADDR)lpsaFrom,	// 送信元アドレス
		&nAddrLen);			// アドレス長へのポインタ

							// 戻り値をチェック
	if (nRet == SOCKET_ERROR)
		ReportError("recvfrom()");

	// 送信時とIP TTL（存続時間）を返す
	*pTTL = echoReply.ipHdr.TTL;
	return(echoReply.echoRequest.dwTime);
}

// 発生した動作の報告
void ReportError(LPCSTR pWhere)
{
	fprintf(stderr, "\n%s error: %d\n",
		WSAGetLastError());
}


// WaitForEchoReply()
// select()を使用して、データが
// 読み取り待機中かどうかを判別する
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