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

	// һ�����
	USHORT usSeqNo = 0; // ICMP �������к�
	int iTTL = 1;	    // TTL ��ʼֵΪ 1 

	// ��ʼ��Ŀ�ĵص�ַ
	ZeroMemory(&destSockAddr, sizeof(sockaddr_in));
	destSockAddr.sin_family = AF_INET;
	// destSockAddr.sin_addr.s_addr = ulDestIP;

	// ��ʼ��ԭʼsocket
	sockRaw = WSASocket(AF_INET, SOCK_RAW, IPPROTO_ICMP, NULL, 0, WSA_FLAG_OVERLAPPED);

	// ����Socket��������ʱ����������TTL
	setsockopt(sockRaw, SOL_SOCKET, SO_RCVTIMEO, (char *)&iTimeout, sizeof(iTimeout));  // ���ճ�ʱ
	setsockopt(sockRaw, SOL_SOCKET, SO_SNDTIMEO, (char *)&iTimeout, sizeof(iTimeout));  // ���ͳ�ʱ																
	setsockopt(sockRaw, IPPROTO_IP, IP_TTL, (char *)&iTTL, sizeof(iTTL));

	// ��ʼ��������
	memset(IcmpSendBuf, 0, sizeof(IcmpSendBuf)); //��ʼ�����ͻ�����
	memset(IcmpRecvBuf, 0, sizeof(IcmpRecvBuf)); //��ʼ�����ջ�����

	// ����IP���ݰ�����������ֱ��д�뷢�ͻ������У���������
	pIcmpHeader = (ICMP_HEADER*)IcmpSendBuf;          
	pIcmpHeader->type = ICMP_ECHO_REQUEST;								// ����Ϊ�������
	pIcmpHeader->code = 0;												// �����ֶ�Ϊ 0
	pIcmpHeader->id = (USHORT)GetCurrentProcessId();					// ID �ֶ�Ϊ��ǰ���̺�
	pIcmpHeader->cksum = 0; //У�������Ϊ 0
	pIcmpHeader->seq = htons(usSeqNo++); //������к�
	pIcmpHeader->cksum = checksum((USHORT *)IcmpSendBuf, sizeof(ICMP_HEADER) + DEF_ICMP_DATA_SIZE); //����У���
	memset(IcmpSendBuf + sizeof(ICMP_HEADER), 'E', DEF_ICMP_DATA_SIZE); // �����ֶ�


	DecodeResult.usSeqNo = ((ICMP_HEADER*)IcmpSendBuf)->seq; //��ǰ���
	DecodeResult.dwRoundTripTime = GetTickCount(); //��ǰʱ��
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

	//ת�����ɹ�ʱ����������
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

	destSockAddr.sin_addr.s_addr = ulDestIP;      // ����Ҫ���õ�ַ

	USHORT usSeqNo = 0; //ICMP �������к�
	int iTTL = 1; //TTL ��ʼֵΪ 1

	BOOL bReachDestHost = FALSE;
	int iMaxHot = DEF_MAX_HOP;
	
	while (!bReachDestHost&&iMaxHot--)
	{
		
		setsockopt(sockRaw, IPPROTO_IP, IP_TTL, (char *)&iTTL, sizeof(iTTL));  //���� IP ��ͷ�� TTL �ֶ�

		printf("%d   ", iTTL);       // ��ǰ��ttl
		
		//��� ICMP ������ÿ�η��ͱ仯���ֶ�
		((ICMP_HEADER *)IcmpSendBuf)->cksum = 0; //У�������Ϊ 0
		((ICMP_HEADER *)IcmpSendBuf)->seq = htons(usSeqNo++); //������к�
		((ICMP_HEADER *)IcmpSendBuf)->cksum = checksum((USHORT *)IcmpSendBuf, sizeof(ICMP_HEADER) + DEF_ICMP_DATA_SIZE); //����У���

		//��¼���кź͵�ǰʱ��
		DecodeResult.usSeqNo = ((ICMP_HEADER*)IcmpSendBuf)->seq; //��ǰ���
		DecodeResult.dwRoundTripTime = GetTickCount(); //��ǰʱ��
		
		//���� TCP ����������Ϣ
		_send();

		while (1) {
			
			int iReadDataLen = _recv();
			if (iReadDataLen != SOCKET_ERROR) {    // �����ݵ���
			
				//�����ݰ����н���
				if (DecodeIcmpResponse(IcmpRecvBuf, iReadDataLen, DecodeResult, ICMP_ECHO_REPLY, ICMP_TIMEOUT)) {

					//��� IP ��ַ
					cout << '\t' << inet_ntoa(DecodeResult.dwIPaddr) << endl;

					//����Ŀ�ĵأ��˳�ѭ��
					if (DecodeResult.dwIPaddr.s_addr == destSockAddr.sin_addr.s_addr) {
						bReachDestHost = true;
						cout << "\n" << "Complete tracert...\n\n" << endl;
					}
				}
			} else if (WSAGetLastError() == WSAETIMEDOUT) {            //���ճ�ʱ�����*�� 
				cout << " *" << '\t' << "Request timed out." << endl;
			}
			break;
		}
		iTTL++; //���� TTL ֵ
	}

	system("pause");

	return 0;
}

//��������У��ͺ���
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

//�����ݰ����н���
BOOL icmp::DecodeIcmpResponse(char * pBuf, int iPacketSize, DECODE_RESULT &DecodeResult, BYTE ICMP_ECHO_REPLY, BYTE ICMP_TIMEOUT)
{
	//������ݱ���С�ĺϷ���
	IP_HEADER* pIpHdr = (IP_HEADER*)pBuf;
	int iIpHdrLen = pIpHdr->hdr_len * 4;
	if (iPacketSize < (int)(iIpHdrLen + sizeof(ICMP_HEADER)))
		return FALSE;
	//���� ICMP ����������ȡ ID �ֶκ����к��ֶ�
	ICMP_HEADER *pIcmpHdr = (ICMP_HEADER *)(pBuf + iIpHdrLen);
	USHORT usID, usSquNo;
	if (pIcmpHdr->type == ICMP_ECHO_REPLY) //ICMP ����Ӧ����
	{
		usID = pIcmpHdr->id; //���� ID
		usSquNo = pIcmpHdr->seq; //�������к�
	}
	else if (pIcmpHdr->type == ICMP_TIMEOUT)//ICMP ��ʱ�����
	{
		char * pInnerIpHdr = pBuf + iIpHdrLen + sizeof(ICMP_HEADER); //�غ��е� IP ͷ
		int iInnerIPHdrLen = ((IP_HEADER *)pInnerIpHdr)->hdr_len * 4; //�غ��е� IP ͷ��
		ICMP_HEADER * pInnerIcmpHdr = (ICMP_HEADER *)(pInnerIpHdr + iInnerIPHdrLen);//�غ��е� ICMP ͷ
		usID = pInnerIcmpHdr->id;	  // ���� ID
		usSquNo = pInnerIcmpHdr->seq; // ���к�
	}
	else
	{
		return false;
	}
	//��� ID �����к���ȷ���յ��ڴ����ݱ�
	if (usID != (USHORT)GetCurrentProcessId() || usSquNo != DecodeResult.usSeqNo)
	{
		return false;
	}

	//��¼ IP ��ַ����������ʱ��
	DecodeResult.dwIPaddr.s_addr = pIpHdr->sourceIP;
	DecodeResult.dwRoundTripTime = GetTickCount() - DecodeResult.dwRoundTripTime;
	//������ȷ�յ��� ICMP ���ݱ�
	if (pIcmpHdr->type == ICMP_ECHO_REPLY || pIcmpHdr->type == ICMP_TIMEOUT)
	{
		//�������ʱ����Ϣ
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

