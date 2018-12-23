/**
Socket sample
Client
**/

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS

#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <Winsock2.h>
using namespace std;

#pragma comment(lib,"ws2_32")

DWORD WINAPI recvThread(LPVOID p);
DWORD WINAPI sendThread(LPVOID p);

int main()
{
	/********INITIALIZATION OF WINSOCK********/
	WORD wVersionRequested;
	WSADATA wsaData;
	int err;
	wVersionRequested = MAKEWORD(1, 1);

	err = WSAStartup(wVersionRequested, &wsaData);   // ��ʼ�� WinSock DLL ��
	if (err != 0) {
		return 1;
	}

	if (LOBYTE(wsaData.wVersion) != 1 || HIBYTE(wsaData.wVersion) != 1) {
		WSACleanup();
		return 1;
	}

	/********CREATE A SOCKET************
	�������ӵ�������Ϣ:
	1. ��ַ�� sin_family = AF_INET �� Address Inet��ʹ��IP
	2. IP��ַ sin_addr
	3. �˿ں� sin_port
	********/

	SOCKET sockClient = socket(AF_INET, SOCK_STREAM, 0);       // ����һ��socket
	SOCKADDR_IN addrSrv;    								   // socket��ַ�ṹ��
	addrSrv.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");     // ������ԣ�IP��ַ����Ϊ���ػػ�(Loopback)��ַ
															   // addrSrv.sin_addr.S_un.S_addr=inet_addr("10.1.11.207");  // ����Ϊ�Ա�ͬѧ�Ļ���������
															   //addrSrv.sin_addr.S_un.S_addr = inet_addr("10.1.11.203"); // ����Ϊ����һ��ͬѧ�Ļ���������
	addrSrv.sin_family = AF_INET;
	addrSrv.sin_port = htons(9999);

	/********CONNECT A SOCKET
	������Ҫ����Ϣ��socket��socket addr
	********/
	connect(sockClient, (SOCKADDR*)&addrSrv, sizeof(SOCKADDR));

	/********RECEIVE DATA********/
	HANDLE receiveThread = CreateThread(NULL, 0, recvThread, (LPVOID)&sockClient, 0, NULL);
	
	/********SEND DATA ****/
	HANDLE sdThread = CreateThread(NULL, 0, sendThread, (LPVOID)&sockClient, 0, NULL);

	while (1) {}

	/********CLOSE
	���õ�Windows SocketsӦ�ó����ͨ������WSACleanup()ָ������Windows Socketsʵ����ע��.
	��������˿��������ͷŷ����ָ��Ӧ�ó������Դ.
	��һ�����̵߳Ļ�����,WSACleanup()��ֹ��Windows Sockets�������߳��ϵĲ���.
	***************/
	closesocket(sockClient);
	WSACleanup();

	return 0;
}

DWORD WINAPI recvThread(LPVOID p) {

	Sleep(1000);

	SOCKET* sockClient = (SOCKET*)p;

	while (1) {

		char recvBuf[1024];  // ���ý�����Ϣ�Ļ�����

		int iResult = recv(*sockClient, recvBuf, 1024, 0);
		if (iResult > 0) {
			printf("%s\n\ncmd>", recvBuf);              // ���յ���Ϣ��ֱ��print��ӡ
		} else {
			printf("\n\n\n��������ʧ��/�������ر�..\n�����˳��ͻ���...\n");
			closesocket(*sockClient);
			WSACleanup();
			system("pause");
			exit(0);
			break;
		}
	}

	return 0;
}

DWORD WINAPI sendThread(LPVOID p) {
	SOCKET* sockClient = (SOCKET*)p;
	char s[1024];
	while (1) {
		// ��ȡһ����Ϣ    //   ˽�ģ�cmd_talk_to <uuid> <����> |   �㲥�� <����>
		gets_s(s);		  // gets(s);
		int iResult = send(*sockClient, s, strlen(s) + 1, 0);
		if (SOCKET_ERROR == iResult) {
			// error
			break;
		} else {
			printf("\n\ncmd>");
		}
	}
	return 0;
}