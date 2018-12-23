/**
Socket sample
Server
**/

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <Winsock2.h>
#include <vector>
#include <string>
#include <iostream>
using namespace std;

#pragma comment(lib,"ws2_32")       // Link  ws2_32.lib library file

DWORD WINAPI sendSystemLevelMessage(LPVOID p);
DWORD WINAPI recvThread(LPVOID p);
int broadcastAllClient(const char* str);
int getId(const char* contents);
int contentPos(const char* contents);
string getUserListInfo();
int findClient(int id);

struct requestHandlerStruct {
	SOCKET sockConn;           
	SOCKADDR_IN addrClient;    
	int id;
};

vector<requestHandlerStruct> v;
int uuid = 0;

int main()
{
	/********INITIALIZATION OF WINSOCK********/
	WORD wVersionRequested;         // 使用的 WinSock 的版本
	WSADATA wsaData;
	int err;
	wVersionRequested = MAKEWORD(1, 1);
	err = WSAStartup(wVersionRequested, &wsaData);  // 初始化 WinSock DLL
	if (err != 0) {
		return 1;
	}
	if (LOBYTE(wsaData.wVersion) != 1 || HIBYTE(wsaData.wVersion) != 1) {
		WSACleanup();
		return 1;
	}

	/********CREATE A SOCKET
	建立连接的三个信息：
	1. 地址族 sin_family = AF_INET ； Address Inet即使用IP
	2. IP地址 sin_addr
	3. 端口号 sin_port
	********/

	SOCKET sockSrv = socket(AF_INET, SOCK_STREAM, 0);
	SOCKADDR_IN addrSrv;
	addrSrv.sin_addr.S_un.S_addr = htonl(INADDR_ANY);      // 主机字节转化成网络字节的函数
	addrSrv.sin_family = AF_INET;
	addrSrv.sin_port = htons(9999);

	/********BIND A SOCKET********/
	bind(sockSrv, (SOCKADDR*)&addrSrv, sizeof(SOCKADDR)); // <ip , port>

	/********BIND A SOCKET********/
	listen(sockSrv, SOMAXCONN);                                  // 等待连接队列的最大长度，also SOMAXCONN


	SOCKADDR_IN addrClient;
	int len = sizeof(SOCKADDR);

	// 创建线程，能够给客户端发送系统级别的消息...
	HANDLE sendSystemMessage = CreateThread(NULL, 0, sendSystemLevelMessage, NULL, 0, NULL);

	SOCKET sockConn;

	// 下面是循环等待接收客户端的连接
	while (1) {

		memset(&addrClient, 0, sizeof(SOCKADDR_IN)); 

		sockConn = accept(sockSrv, (SOCKADDR*)&addrClient, &len);     // accept ，一个socket连接

		if (INVALID_SOCKET == sockConn) {
			printf("Error accept...");
			Sleep(1000);
			continue;
		}

		// 在服务端显示一下谁连入了服务器。
		printf("%s:%d connects to me...\n", inet_ntoa((addrClient).sin_addr), addrClient.sin_port);

		// 发送给客户端欢迎信息，告诉用户此程序的用法
		string welcomeInfo = "Welcome to CHAT_WITH_ME_CHATROOM!\n";
		welcomeInfo.append("Usage: \n1. 私聊: cmd_talk_to <uuid> <内容>\n");
		welcomeInfo.append("2. 广播： <内容>\n");
		welcomeInfo.append("更多功能尽请期待...\n");
		send(sockConn, welcomeInfo.c_str(), strlen(welcomeInfo.c_str()) + 1, 0);

		struct requestHandlerStruct p;             // 指针害死人，不用指针。
		p.sockConn = sockConn;
		p.addrClient = addrClient;
		p.id = uuid++;            // 分配一个uuid

		// 加入
		v.push_back(p);

		// 广播给所有连接的客户端，已经有新的客户端连入服务器了
		broadcastAllClient( getUserListInfo().c_str() );

		// 开启接收线程，接收客户端信息。
		// 我们对特定的信息进行转发。
		HANDLE receiveThread = CreateThread(NULL, 0, recvThread, (LPVOID)&p, 0, NULL);

	}

	WSACleanup();

	return 0;
}

int broadcastAllClient(const char* str) {
	for (int j = 0; j < v.size(); j++) {
		send((v[j].sockConn), str, strlen(str) + 1, 0);
	}
	return 0;
}

int sendAClient(const char* contents, int id) {

	cout << "根据接受内容来发送数据 < Contents: " << contents << ", id: " << id  << " > " << endl;

	int clientIndex = findClient(id);
	char sendStr[1024];
	sprintf(sendStr, "\n\nReceive> 收到来自id为%d的信息: %s\n\n", v[clientIndex].id, string(contents).c_str());
	int iSendResult = send(v[clientIndex].sockConn, sendStr, strlen(sendStr)+1, 0);

	// 如果发送失败，我们认为，服务器已经断开与客户端的连接。
	if (iSendResult == SOCKET_ERROR) {
		printf("Send failed: %d\n", WSAGetLastError());
		// 断开连接
		closesocket(v[clientIndex].sockConn);
		// 移除无用连接
		v.erase(v.begin() + clientIndex);
		// 通知所有用户，已经有用户下线了
		broadcastAllClient(getUserListInfo().c_str());
	}

	return 0;
}

DWORD WINAPI sendSystemLevelMessage(LPVOID p) {

	while (1) {
		
		char s[1024];
		while (1) {
			
			gets_s(s);		  // gets(s);

			char sendStr[1024];
			sprintf(sendStr, "\n\nReceive> 收到来自系统的信息: %s\n\n", s/*string(s).c_str()*/);

			broadcastAllClient(sendStr);
		}

	}
}

DWORD WINAPI recvThread(LPVOID p) {

	requestHandlerStruct request = *(requestHandlerStruct*)p;

	char recvBuf[1024];  // 设置接收消息的缓冲区

	while (1) {

		// 接收信息
		int iResult = recv(request.sockConn, recvBuf, 1024, 0);
		if ( iResult <= 0) {
			// printf("接收数据错误/连接断开...\n");
			printf("%s:%d disconnects to me...\n", inet_ntoa((request.addrClient).sin_addr), request.addrClient.sin_port);  
			closesocket(request.sockConn);
			// 移除无用连接
			int clientIndex = findClient(request.id);
			v.erase(v.begin() + clientIndex);
			// 通知所有用户，已经有用户下线了
			broadcastAllClient(getUserListInfo().c_str());
			break;
		} // v

		// 端口数据处理
		char port[16];
		_itoa(request.addrClient.sin_port, port, 10);    // convert

														 // 显示客户端发来的数据
		cout << "From " << inet_ntoa(request.addrClient.sin_addr) << ": " << port << " < " << recvBuf << " >" << endl;

		// 现在我们要进行转发
		// 如果有格式为：cmd_talk_to <UUID> <内容> ；那么我们进行消息转发
		// 如果格式为：<内容> ；那么我们直接进行广播
		string cmd(recvBuf, 11);   
		if (cmd == "cmd_talk_to") {                // 如果发送给特定的用户，则转发

			string contents(recvBuf);
			string str = contents.substr(12);     // #123123 这是内容   
			int id = getId(str.c_str());     
			
			//  printf("id: %d\n", id);           // 打印id

			sendAClient(str.substr(contentPos(str.c_str())).c_str(), id);

		} else {                            // 直接广播出去 usr grp
			char sendStr[1024];
			sprintf(sendStr, "\n\nReceive> 收到来自id为%d的广播信息: %s\n\n", request.id, string(recvBuf).c_str());
			broadcastAllClient(sendStr);
		}
	}

	return 0;
}

int getId(const char* contents) {          // #123123 这是内容

	int i = contentPos(contents) - 1;

	int id = 0;
	for (int index = 0; index < i; index++) {
		id = id * 10 + (contents[index] - '0');
	}

	return id;
}

int contentPos(const char* contents) {      // #123123 这是内容
	int i = 0;
	while (1) {                              // 123_asdasd;
		if (contents[i] == ' ') {
			break;
		}
		i++;
	}
	return i + 1;
}

string getUserListInfo() {
	string userList = "\n有客户端加入\/退出，当前在线客户端列表如下：\n";
	for (int i = 0; i < v.size(); i++) {
		char des[16];
		// 序号
		_itoa(v[i].id, des, 10);							// convert 
		userList.append(des);
		userList.append(". ");
		// IP地址
		userList.append(inet_ntoa(((v[i].addrClient)).sin_addr));
		userList.append(":");
		// 端口号
		_itoa(((v[i].addrClient)).sin_port, des, 10);    // convert
		userList.append(des);
		userList.append("\n");
	}

	return userList;
}

int findClient(int id) {

	for (int i = 0; i < v.size(); i++) {
		if (v[i].id == id) {
			return i;
		}
	}
	return -1;
}
