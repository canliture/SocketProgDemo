#include "HttpProtocol.h"
#include <stdio.h>

int main(int argc, char** args) {

	char rootDir[128] = "g:/";
	char ip[16] = "127.0.0.1";
	int port = 80;/*
				  printf("Server IP? ");
				  scanf("%s", ip);                   // ����IP
				  printf("Server Port? ");
				  scanf("%d", &port);				   // ����˿�
				  printf("Web Root Directory? ");
				  scanf("%s", rootDir);              // ����Web��Ŀ¼
				  */

	HttpProtocol* httpServer = new HttpProtocol(ip, port, rootDir);
	httpServer->StartHttpSrv();

	getchar();
	getchar();

	// system("pause");
}