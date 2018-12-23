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

	err = WSAStartup(wVersionRequested, &wsaData);   // 初始化 WinSock DLL 库
	if (err != 0) {
		return 1;
	}

	if (LOBYTE(wsaData.wVersion) != 1 || HIBYTE(wsaData.wVersion) != 1) {
		WSACleanup();
		return 1;
	}

	/********CREATE A SOCKET************
	建立连接的三个信息:
	1. 地址族 sin_family = AF_INET ； Address Inet即使用IP
	2. IP地址 sin_addr
	3. 端口号 sin_port
	********/

	SOCKET sockClient = socket(AF_INET, SOCK_STREAM, 0);       // 创建一个socket
	SOCKADDR_IN addrSrv;    								   // socket地址结构体
	addrSrv.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");     // 方便测试，IP地址设置为本地回环(Loopback)地址
															   // addrSrv.sin_addr.S_un.S_addr=inet_addr("10.1.11.207");  // 设置为旁边同学的机器来测试
															   //addrSrv.sin_addr.S_un.S_addr = inet_addr("10.1.11.203"); // 设置为另外一个同学的机器来测试
	addrSrv.sin_family = AF_INET;
	addrSrv.sin_port = htons(9999);

	/********CONNECT A SOCKET
	函数需要的信息，socket，socket addr
	********/
	connect(sockClient, (SOCKADDR*)&addrSrv, sizeof(SOCKADDR));

	/********RECEIVE DATA********/
	HANDLE receiveThread = CreateThread(NULL, 0, recvThread, (LPVOID)&sockClient, 0, NULL);
	
	/********SEND DATA ****/
	HANDLE sdThread = CreateThread(NULL, 0, sendThread, (LPVOID)&sockClient, 0, NULL);

	while (1) {}

	/********CLOSE
	良好的Windows Sockets应用程序会通过调用WSACleanup()指出它从Windows Sockets实现中注销.
	本函数因此可以用来释放分配给指定应用程序的资源.
	在一个多线程的环境下,WSACleanup()中止了Windows Sockets在所有线程上的操作.
	***************/
	closesocket(sockClient);
	WSACleanup();

	return 0;
}

DWORD WINAPI recvThread(LPVOID p) {

	Sleep(1000);

	SOCKET* sockClient = (SOCKET*)p;

	while (1) {

		char recvBuf[1024];  // 设置接收消息的缓冲区

		int iResult = recv(*sockClient, recvBuf, 1024, 0);
		if (iResult > 0) {
			printf("%s\n\ncmd>", recvBuf);              // 接收到消息后，直接print打印
		} else {
			printf("\n\n\n接受数据失败/服务器关闭..\n正在退出客户端...\n");
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
		// 获取一行信息    //   私聊：cmd_talk_to <uuid> <内容> |   广播： <内容>
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