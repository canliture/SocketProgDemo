#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS

#include "Server.h"
#include <assert.h>
#include <vector>
#include <string>

int contentPos(std::string str);
int getIdFromClienStr(std::string contentStr);

typedef struct stru {
	Server* server;			     
	CLIENT client;
} RECV_PARAM;

int Server::initWinSock() {
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

	return 0;
}

Server::Server(){
	Server(DEFAULT_PORT);          // 默认端口即可
}

Server::Server(int port) 
{
	uuId = 0;                                           // 客户端id
	grpId = 0;                                          // 群id

	assert(initWinSock() == 0);       // 初始化socket

	sockServer = socket(AF_INET, SOCK_STREAM, 0);
	addrSrv.sin_addr.S_un.S_addr = htonl(INADDR_ANY);      // 主机字节转化成网络字节的函数
	addrSrv.sin_family = AF_INET;
	addrSrv.sin_port = htons(port);

	bind(sockServer, (SOCKADDR*)&addrSrv, sizeof(SOCKADDR));

	listen(sockServer, SOMAXCONN);                                  // 等待连接队列的最大长度，also SOMAXCONN
}

CLIENT Server::acceptClient() {

	CLIENT client;

	memset(&client.sockAddr, 0, sizeof(SOCKADDR_IN));

	int len = sizeof(client.sockAddr);
	client.sockConnect = accept(sockServer, (SOCKADDR*)&client.sockAddr, &len);                       // accept

	if (client.sockConnect == INVALID_SOCKET) {													      // 如果accept出错
		printf("Error accept...");
		Sleep(1000);
		return client;
	}

	client.uuid = uuId++;

	// 保存已经在线的客户端信息
	onlineClient.push_back(client);

	printf("<%s:%d> connects to me...\n", inet_ntoa((client.sockAddr).sin_addr), client.sockAddr.sin_port);   // 在服务端显示一下谁连入了服务器。
	
	return client;
}

int Server::sendToUsr(CLIENT client, int uuid, const char* contents, int type) {
	
	char buf[2048];

	if (type == SEND_TYPE_BROADCAST) {
		sprintf(buf, "    \nYou received <%d. %s:%d>'s broadcast message: %s\n", client.uuid, inet_ntoa(client.sockAddr.sin_addr), client.sockAddr.sin_port, contents);
	} else if (type == SEND_TYPE_SINGLE) {
		sprintf(buf, "    \nYou received <%d. %s:%d>'s message: %s\n", client.uuid, inet_ntoa(client.sockAddr.sin_addr), client.sockAddr.sin_port, contents);
	} else if (type == SEND_TYPE_SYSTEM) {
		sprintf(buf, "    \nSystem Info: %s\n", contents);
	} else if (type == SEND_TYPE_GROUP) {
		sprintf(buf, "    \nYou received group message from <%d. %s:%d>: %s", client.uuid, inet_ntoa(client.sockAddr.sin_addr), client.sockAddr.sin_port, contents);
	} else if (type == SEND_TYPE_DIRECT) {
		sprintf(buf, "    \n%s \n", contents);
	}
	
	int iSendResult = send(onlineClient[findClientIndex(uuid)].sockConnect,  buf, strlen(buf)+1, 0);

	// 如果发送失败，我们认为，服务器已经断开与客户端的连接。
	if (iSendResult == SOCKET_ERROR) {
		
		printf("Send failed: %d\n", WSAGetLastError());
		
		// 断开连接
		closesocket(client.sockConnect);
		
		// 移除无用连接
		onlineClient.erase(onlineClient.begin() + findClientIndex(client.uuid));
		for (int i = 0; i < grpId2UsrIdMap.size(); i++) {
			std::vector<int>& v = grpId2UsrIdMap[i];
			for (int i = 0; i < v.size(); i++) {
				if (client.uuid == v[i]) {
					v.erase(v.begin()+i);
				}
			}
		}
		std::map<int, std::vector<int> >::iterator iter = usrId2GrpIdMap.begin();
		for (; iter != usrId2GrpIdMap.end(); iter++) {
			if (iter->first == client.uuid) {
				usrId2GrpIdMap.erase(iter);
				break;
			}
		}

		// 通知所有用户，已经有用户下线了
		char buf[128];
		sprintf(buf, "A client <%d. %s:%d> has been offline.", client.uuid, inet_ntoa(client.sockAddr.sin_addr), client.sockAddr.sin_port);

		sendToAll(client, buf, SEND_TYPE_SYSTEM);

		return 1;
	}
	return 0;

}

int Server::sendToAll(CLIENT client, const char* contents, int type) {
	for (int j = 0; j < onlineClient.size(); j++) {
		if (type == SEND_TYPE_BROADCAST) {
			sendToUsr(onlineClient[j], onlineClient[j].uuid, contents, SEND_TYPE_BROADCAST);
		} else {
			sendToUsr(onlineClient[j], onlineClient[j].uuid, contents, SEND_TYPE_SYSTEM);
		}
	}
	return 0;
}

int Server::sendToGrp(CLIENT client, int grpId, const char* constents) {
	std::map<int, std::vector<int> >::iterator iter = grpId2UsrIdMap.find(grpId);
	if (iter == grpId2UsrIdMap.end()) {
		sendToUsr(client, client.uuid, "您还没加入该群", SEND_TYPE_SYSTEM);
		return 1;
	}
	std::vector<int> clientV = iter->second;
	for (int i = 0; i < clientV.size(); i++) {
		sendToUsr(client, onlineClient[findClientIndex(clientV[i])].uuid, constents, SEND_TYPE_GROUP);
	}
	return 0;
}

int Server::inGrp(CLIENT client, int grpId) {
	std::map<int, std::vector<int> >::iterator iter = grpId2UsrIdMap.find(grpId);
	if (iter == grpId2UsrIdMap.end()) {
		sendToUsr(client, client.uuid, "不存在该群", SEND_TYPE_SYSTEM);
		return 1;
	}
	std::vector<int>* clientV = &(iter->second);
	for (int i = 0; i < clientV->size(); i++) {
		if (client.uuid == (*clientV)[i]) {
			sendToUsr(client, client.uuid, "您已经是该群群员了", SEND_TYPE_SYSTEM);
			return 1;
		}
	}
	clientV->push_back(client.uuid);
	usrId2GrpIdMap[client.uuid].push_back(grpId);
	return 0;
}

int Server::outGrp(CLIENT client, int grpId) {
	std::map<int, std::vector<int> >::iterator iter = grpId2UsrIdMap.find(grpId);
	if (iter == grpId2UsrIdMap.end()) {
		sendToUsr(client, client.uuid, "不存在该群", SEND_TYPE_SYSTEM);
		return 1;
	}
	std::vector<int>* clientV = &(iter->second);
	for (int i = 0; i < clientV->size(); i++) {
		if (client.uuid == (*clientV)[i]) {
			clientV->erase(clientV->begin() + i);
			
			std::vector<int>* v = &usrId2GrpIdMap[client.uuid];
			for (int j = 0; j < v->size(); j++) {
				if ((*v)[j] == grpId) {
					(*v).erase((*v).begin() + j);
					break;
				}
			}

			return 0;
		}
	}
	return 1;
}

int Server::setUpGrp(CLIENT client) {
	std::vector<int> v;
	v.push_back(client.uuid);
	grpId2UsrIdMap[grpId] = v;                     // 此群有第一个用户
	usrId2GrpIdMap[client.uuid].push_back(grpId);  // 用户属于该群
	return grpId++;
}

int Server::listUsers(CLIENT client) {
	std::string str = "";
	str.append("Online hosts: \n");
	for (int i = 0; i < onlineClient.size(); i++) {
		char buf[32];
		sprintf(buf, "<%d. %s:%d> \n", onlineClient[i].uuid, inet_ntoa(onlineClient[i].sockAddr.sin_addr), onlineClient[i].sockAddr.sin_port);
		str.append(buf);
	}
	sendToUsr(client, client.uuid, str.c_str(), SEND_TYPE_DIRECT);
	return 0;
}

int Server::listGrps(CLIENT client) {

	std::string str = "";
	str.append("All groups: \n");
	std::map<int, std::vector<int> >::iterator iter = grpId2UsrIdMap.begin();
	for (; iter != grpId2UsrIdMap.end(); iter++) {
		char buf[32];
		sprintf(buf, "【%d】th. Group: \n", iter->first);
		str.append(buf);
		std::vector<int> v = iter->second;
		for (int j = 0; j < v.size(); j++) {
			int clientIndex = findClientIndex(v[j]);
			sprintf(buf, "    <%d. %s:%d> \n", onlineClient[clientIndex].uuid, inet_ntoa(onlineClient[clientIndex].sockAddr.sin_addr), onlineClient[clientIndex].sockAddr.sin_port);
			str.append(buf);
		}
	}
	sendToUsr(client, client.uuid, str.c_str(), SEND_TYPE_DIRECT);
	return 0;
}

DWORD WINAPI recvThread(LPVOID p);

void Server::recvFrom(CLIENT client) {
	RECV_PARAM* recvParam = new RECV_PARAM();
	recvParam->server = this;
	recvParam->client = client;
	CreateThread(NULL, 0, recvThread, recvParam, 0, NULL);
}



DWORD WINAPI recvThread(LPVOID p) {
	
	RECV_PARAM recvParam = *(RECV_PARAM*)p;
	CLIENT client = recvParam.client;
	Server* server = recvParam.server;

	char recvBuf[1024];  // 设置接收消息的缓冲区

	while (1) {

		// 接收信息
		int iResult = recv(client.sockConnect, recvBuf, 1024, 0);
		if (iResult <= 0) {
			// printf("接收数据错误/连接断开...\n");
			printf("%s:%d disconnects to me...\n", inet_ntoa((client.sockAddr).sin_addr), client.sockAddr.sin_port);
			closesocket(client.sockConnect);
			
			// 移除无用连接
			server->onlineClient.erase(server->onlineClient.begin() + server->findClientIndex(client.uuid));
			for (int i = 0; i < server->grpId2UsrIdMap.size(); i++) {
				std::vector<int>& v = server->grpId2UsrIdMap[i];
				for (int i = 0; i < v.size(); i++) {
					if (client.uuid == v[i]) {
						v.erase(v.begin() + i);
					}
				}
			}
			std::map<int, std::vector<int> >::iterator iter = server->usrId2GrpIdMap.begin();
			for (; iter != server->usrId2GrpIdMap.end(); iter++) {
				if (iter->first == client.uuid) {
					server->usrId2GrpIdMap.erase(iter);
					break;
				}
			}

			// 通知所有用户，已经有用户下线了
			char buf[128];
			sprintf(buf, "A client <%d. %s:%d> has been offline.", client.uuid, inet_ntoa(client.sockAddr.sin_addr), client.sockAddr.sin_port);
			server->sendToAll(client, buf, SEND_TYPE_SYSTEM);

			break;
		}

		printf("From <%d. %s:%d> : %s\n", client.uuid, inet_ntoa(client.sockAddr.sin_addr), client.sockAddr.sin_port, recvBuf);   // 打印从哪来的数据

		if (strlen(recvBuf) < 10) {
			char sendStr[1024];
			sprintf(sendStr, "\n\nReceive> 收到来自id为%d的广播信息: %s\n\n", client.uuid, std::string(recvBuf).c_str());
			server->sendToAll(client, sendStr, SEND_TYPE_DIRECT);
		}

		// 现在我们要进行转发
		// 如果有格式为：cmd_talk_to <UUID> <内容> ；那么我们进行消息转发
		// 如果格式为：<内容> ；那么我们直接进行广播
		std::string cmd(recvBuf, 10);
		if (cmd == "cmd_to_usr" || cmd == "cmd_to_grp" || 
			cmd == "cmd_in_grp" || cmd == "cmd_ot_grp" || cmd == "cmd_su_grp" ||
			cmd == "cmd_ls_usr" || cmd == "cmd_ls_grp" ||
			cmd == "cmd_fw_frd" || cmd == "cmd_dl_frd") {

			if (strlen(recvBuf) > 10) {
				std::string contents(recvBuf);
				std::string str = contents.substr(11);     // #123123 <内容>
				int id = getIdFromClienStr(str);

				printf("id: %d\n", id);           // 打印id

				if (cmd == "cmd_to_usr") {
					int clientIndex = server->findClientIndex(id);

					if (clientIndex == -1) {
						server->sendToUsr(client, client.uuid, "此用户离线/数据发送失败", SEND_TYPE_SYSTEM);
						continue;
					}
					server->sendToUsr(client, id, str.substr(contentPos(str)).c_str(), SEND_TYPE_SINGLE);
				}
				if (cmd == "cmd_to_grp") {      // 发送信息给群
					server->sendToGrp(client, id, str.substr(contentPos(str)).c_str());
				}
				if (cmd == "cmd_in_grp") {      // 进群
					server->inGrp(client, id);
				}
				if (cmd == "cmd_ot_grp") {      // 退群
					server->outGrp(client, id);
				}
			} else {
				if (cmd == "cmd_su_grp") {
					server->setUpGrp(client);
				}
				if (cmd == "cmd_ls_usr") {
					server->listUsers(client);
				}
				if (cmd == "cmd_ls_grp") {
					server->listGrps(client);
				}
			}
		} else if(cmd == "") {                            // 直接广播出去 usr grp
			char sendStr[1024];
			sprintf(sendStr, "\n\nReceive> 收到来自id为%d的广播信息: %s\n\n", client.uuid, std::string(recvBuf).c_str());
			server->sendToAll(client, sendStr, SEND_TYPE_BROADCAST);
		}
	}
	return 0;
}

int Server::findClientIndex(int id) {             
	
	for (int i = 0; i < onlineClient.size(); i++) {
		if (onlineClient[i].uuid == id) {
			return i;
		}
	}

	return -1;
}
int contentPos(std::string str) {
	int i = 0;
	while (1) {                              // 123_asdasd;
		if (str[i] == ' ' || i >= str.size()) {
			break;
		}
		i++;
	}
	return i + 1;
}

int getIdFromClienStr(std::string contentStr) {

	if (contentStr.size() == 1) {    // "0"
		return contentStr[0] - '0';
	}

	int i = contentPos(contentStr) - 1;  // "10"

	int id = 0;
	for (int index = 0; index < i; index++) {
		id = id * 10 + (contentStr[index] - '0');
	}

	return id;
}

void Server::close() {
	WSACleanup();
}

Server::~Server()
{
}

