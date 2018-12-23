#include "HttpProtocol.h"
#include <stdio.h>

int main(int argc, char** args) {

	char rootDir[128] = "g:/";
	char ip[16] = "127.0.0.1";
	int port = 80;/*
				  printf("Server IP? ");
				  scanf("%s", ip);                   // 输入IP
				  printf("Server Port? ");
				  scanf("%d", &port);				   // 输入端口
				  printf("Web Root Directory? ");
				  scanf("%s", rootDir);              // 输入Web根目录
				  */

	HttpProtocol* httpServer = new HttpProtocol(ip, port, rootDir);
	httpServer->StartHttpSrv();

	getchar();
	getchar();

	// system("pause");
}