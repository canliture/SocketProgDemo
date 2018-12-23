#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include "icmp.h"
#include "utils.h"
#include <stdlib.h>
#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
using namespace std;

#pragma comment(lib, "Ws2_32.lib")

using namespace std;


icmp::icmp()
{
	WSADATA wsa;
	WSAStartup(MAKEWORD(2, 2), &wsa);

	// 一般变量
	USHORT usSeqNo = 0; // ICMP 报文序列号
	int iTTL = 1;	    // TTL 初始值为 1 

	// 初始化目的地地址
	ZeroMemory(&destSockAddr, sizeof(sockaddr_in));
	destSockAddr.sin_family = AF_INET;
	// destSockAddr.sin_addr.s_addr = ulDestIP;

	// 初始化原始socket
	sockRaw = WSASocket(AF_INET, SOCK_RAW, IPPROTO_ICMP, NULL, 0, WSA_FLAG_OVERLAPPED);

	// 设置Socket参数：超时参数，报文TTL
	setsockopt(sockRaw, SOL_SOCKET, SO_RCVTIMEO, (char *)&iTimeout, sizeof(iTimeout));  // 接收超时
	setsockopt(sockRaw, SOL_SOCKET, SO_SNDTIMEO, (char *)&iTimeout, sizeof(iTimeout));  // 发送超时																
	setsockopt(sockRaw, IPPROTO_IP, IP_TTL, (char *)&iTTL, sizeof(iTTL));

	// 初始化缓冲区
	memset(IcmpSendBuf, 0, sizeof(IcmpSendBuf)); //初始化发送缓冲区
	memset(IcmpRecvBuf, 0, sizeof(IcmpRecvBuf)); //初始化接收缓冲区

	// 设置IP数据包参数，可以直接写入发送缓冲区中；方法如下
	pIcmpHeader = (ICMP_HEADER*)IcmpSendBuf;          
	pIcmpHeader->type = ICMP_ECHO_REQUEST;								// 类型为请求回显
	pIcmpHeader->code = 0;												// 代码字段为 0
	pIcmpHeader->id = (USHORT)GetCurrentProcessId();					// ID 字段为当前进程号
	pIcmpHeader->cksum = 0; //校验和先置为 0
	pIcmpHeader->seq = htons(usSeqNo++); //填充序列号
	pIcmpHeader->cksum = checksum((USHORT *)IcmpSendBuf, sizeof(ICMP_HEADER) + DEF_ICMP_DATA_SIZE); //计算校验和
	memset(IcmpSendBuf + sizeof(ICMP_HEADER), 'E', DEF_ICMP_DATA_SIZE); // 数据字段


	DecodeResult.usSeqNo = ((ICMP_HEADER*)IcmpSendBuf)->seq; //当前序号
	DecodeResult.dwRoundTripTime = GetTickCount(); //当前时间
}


void icmp::_send() {
	sendto(sockRaw, IcmpSendBuf, sizeof(IcmpSendBuf), 0, (sockaddr*)&destSockAddr, sizeof(destSockAddr));
}

int icmp::_recv() {
	int iFromLen = sizeof(from);
	int iReadDataLen = recvfrom(sockRaw, IcmpRecvBuf, MAX_ICMP_PACKET_SIZE, 0, (sockaddr*)&from, &iFromLen);
	return iReadDataLen;
}

int icmp::tracert(char* hostname) {

	u_long ulDestIP = inet_addr(hostname);
	
	char ipAddr[32];
	strcpy(ipAddr, hostname);

	//转换不成功时按域名解析
	if (ulDestIP == INADDR_NONE)
	{
		hostent * pHostent = gethostbyname(hostname);
		if (pHostent) {

			ulDestIP = (*(in_addr*)pHostent->h_addr).s_addr;
			
			// ipAddr = utils::longIp2StrIp(ulDestIP);
			strcpy(ipAddr, utils::longIp2StrIp(ulDestIP));

		} else {
			WSACleanup();
			return 1;
		}
	}

	printf("Tracing route to %s", hostname);
	if (strcmp(hostname, ipAddr) != 0) {
		printf(" [ %s ]", ipAddr);
	}
	printf(" with a maximum of 30 hops.\n\n");

	destSockAddr.sin_addr.s_addr = ulDestIP;      // 这里要设置地址

	USHORT usSeqNo = 0; //ICMP 报文序列号
	int iTTL = 1; //TTL 初始值为 1

	BOOL bReachDestHost = FALSE;
	int iMaxHot = DEF_MAX_HOP;
	
	while (!bReachDestHost&&iMaxHot--)
	{
		
		setsockopt(sockRaw, IPPROTO_IP, IP_TTL, (char *)&iTTL, sizeof(iTTL));  //设置 IP 报头的 TTL 字段

		printf("%d   ", iTTL);       // 当前的ttl
		
		//填充 ICMP 报文中每次发送变化的字段
		((ICMP_HEADER *)IcmpSendBuf)->cksum = 0; //校验和先置为 0
		((ICMP_HEADER *)IcmpSendBuf)->seq = htons(usSeqNo++); //填充序列号
		((ICMP_HEADER *)IcmpSendBuf)->cksum = checksum((USHORT *)IcmpSendBuf, sizeof(ICMP_HEADER) + DEF_ICMP_DATA_SIZE); //计算校验和

		//记录序列号和当前时间
		DecodeResult.usSeqNo = ((ICMP_HEADER*)IcmpSendBuf)->seq; //当前序号
		DecodeResult.dwRoundTripTime = GetTickCount(); //当前时间
		
		//发送 TCP 回显请求信息
		_send();

		while (1) {
			
			int iReadDataLen = _recv();
			if (iReadDataLen != SOCKET_ERROR) {    // 有数据到达
			
				//对数据包进行解码
				if (DecodeIcmpResponse(IcmpRecvBuf, iReadDataLen, DecodeResult, ICMP_ECHO_REPLY, ICMP_TIMEOUT)) {

					//输出 IP 地址
					cout << '\t' << inet_ntoa(DecodeResult.dwIPaddr) << endl;

					//到达目的地，退出循环
					if (DecodeResult.dwIPaddr.s_addr == destSockAddr.sin_addr.s_addr) {
						bReachDestHost = true;
						cout << "\n" << "Complete tracert...\n\n" << endl;
					}
				}
			} else if (WSAGetLastError() == WSAETIMEDOUT) {            //接收超时，输出*号 
				cout << " *" << '\t' << "Request timed out." << endl;
			}
			break;
		}
		iTTL++; //递增 TTL 值
	}

	system("pause");

	return 0;
}

//计算网际校验和函数
USHORT icmp::checksum(USHORT *pBuf, int iSize)
{
	unsigned long cksum = 0;
	while (iSize>1)
	{
		cksum += *pBuf++;
		iSize -= sizeof(USHORT);
	}
	if (iSize)
	{
		cksum += *(UCHAR *)pBuf;
	}
	cksum = (cksum >> 16) + (cksum & 0xffff);
	cksum += (cksum >> 16);
	return (USHORT)(~cksum);
}

//对数据包进行解码
BOOL icmp::DecodeIcmpResponse(char * pBuf, int iPacketSize, DECODE_RESULT &DecodeResult, BYTE ICMP_ECHO_REPLY, BYTE ICMP_TIMEOUT)
{
	//检查数据报大小的合法性
	IP_HEADER* pIpHdr = (IP_HEADER*)pBuf;
	int iIpHdrLen = pIpHdr->hdr_len * 4;
	if (iPacketSize < (int)(iIpHdrLen + sizeof(ICMP_HEADER)))
		return FALSE;
	//根据 ICMP 报文类型提取 ID 字段和序列号字段
	ICMP_HEADER *pIcmpHdr = (ICMP_HEADER *)(pBuf + iIpHdrLen);
	USHORT usID, usSquNo;
	if (pIcmpHdr->type == ICMP_ECHO_REPLY) //ICMP 回显应答报文
	{
		usID = pIcmpHdr->id; //报文 ID
		usSquNo = pIcmpHdr->seq; //报文序列号
	}
	else if (pIcmpHdr->type == ICMP_TIMEOUT)//ICMP 超时差错报文
	{
		char * pInnerIpHdr = pBuf + iIpHdrLen + sizeof(ICMP_HEADER); //载荷中的 IP 头
		int iInnerIPHdrLen = ((IP_HEADER *)pInnerIpHdr)->hdr_len * 4; //载荷中的 IP 头长
		ICMP_HEADER * pInnerIcmpHdr = (ICMP_HEADER *)(pInnerIpHdr + iInnerIPHdrLen);//载荷中的 ICMP 头
		usID = pInnerIcmpHdr->id;	  // 报文 ID
		usSquNo = pInnerIcmpHdr->seq; // 序列号
	}
	else
	{
		return false;
	}
	//检查 ID 和序列号以确定收到期待数据报
	if (usID != (USHORT)GetCurrentProcessId() || usSquNo != DecodeResult.usSeqNo)
	{
		return false;
	}

	//记录 IP 地址并计算往返时间
	DecodeResult.dwIPaddr.s_addr = pIpHdr->sourceIP;
	DecodeResult.dwRoundTripTime = GetTickCount() - DecodeResult.dwRoundTripTime;
	//处理正确收到的 ICMP 数据报
	if (pIcmpHdr->type == ICMP_ECHO_REPLY || pIcmpHdr->type == ICMP_TIMEOUT)
	{
		//输出往返时间信息
		if (DecodeResult.dwRoundTripTime)
			cout << " " << DecodeResult.dwRoundTripTime << "ms" << flush;
		else
			cout << " " << "<1ms" << flush;
	}
	return true;
}

icmp::~icmp()
{
}

