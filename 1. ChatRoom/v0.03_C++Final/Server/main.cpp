/**
Socket sample
Server
**/

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS

#include "Server.h"
#include <stdio.h>
#include <string>

std::string getUSAGE();

std::string USAGE = getUSAGE();

int main() {

	Server server = Server(9999);   // 设置本机服务器监听的端口即可
	HANDLE Mutex = CreateMutex(NULL, false, NULL);
	while (1) {
		CLIENT client = server.acceptClient();				  // accept客户端请求
		
		
		server.sendToUsr(client, client.uuid, USAGE.c_str(), SEND_TYPE_DIRECT);         // 接收到用户请求，我们就发送欢迎使用的消息、

		WaitForSingleObject(Mutex, INFINITE);
			char buf[128];
			sprintf(buf, "A new client <%d. %s:%d> enter the chatroom.\n\n", client.uuid, inet_ntoa(client.sockAddr.sin_addr), client.sockAddr.sin_port);
			server.sendToAll(client, buf, SEND_TYPE_SYSTEM);	  // 发送给所有人，有新的客户端连接
		ReleaseMutex(Mutex);

		server.recvFrom(client);                              // 服务器端开始可以接收来自此客户端的信息
	}

	server.close();
}

std::string getUSAGE() {
	static std::string USAGE = "";
	USAGE.append("Welcome to CHAT_WITH_ME_CHATROOM chat room!                            \n");
	USAGE.append("Before you enjoy the chatroom, you need to notice the following tips : \n");
	USAGE.append("                                                                       \n");
	USAGE.append("<N> : represent the number of a client.						         \n");
	USAGE.append("<Contents> : represent the contents we want to send.                   \n\n");
	USAGE.append("<contents>                  -- to all users       [默认广播]信息给所有客户端 \n");
	USAGE.append("cmd_to_usr <N> <Contents>   -- to a user          [私聊]发送消息给人     \n");
	USAGE.append("cmd_to_grp <N> <Contents>   -- to a grp           [群聊]发送消息给群     \n");
	USAGE.append("cmd_in_grp <N>              -- in a group         [加群]加入一个群       \n");
	USAGE.append("cmd_ot_grp <N>              -- 'out' a group      [退群]退出一个群       \n");
	USAGE.append("cmd_su_grp                  -- set up a group     [建群]建立一个群       \n");
	//USAGE.append("cmd_fw_frd <N>              -- follow a friend    加好友                \n");
	//USAGE.append("cmd_dl_frd <N>              -- delete a friend    删好友                \n");
	USAGE.append("cmd_ls_usr                  -- list all the users 列出所有在线用户       \n");
	USAGE.append("cmd_ls_grp                  -- list all the goups 列出所有的群           \n");
	return USAGE;
}