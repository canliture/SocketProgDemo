#pragma once

#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <map>
#include <WinSock2.h>

#pragma comment(lib,"ws2_32")


#define LOCALHOST "127.0.0.1"
#define HTTPPORT 80
#define METHOD_GET 0
#define METHOD_HEAD 1

#define HTTP_STATUS_OK				"200 OK"
#define HTTP_STATUS_CREATED			"201 Created"
#define HTTP_STATUS_ACCEPTED		"202 Accepted"
#define HTTP_STATUS_NOCONTENT		"204 No Content"
#define HTTP_STATUS_MOVEDPERM		"301 Moved Permanently"
#define HTTP_STATUS_MOVEDTEMP		"302 Moved Temporarily"
#define HTTP_STATUS_NOTMODIFIED		"304 Not Modified"
#define HTTP_STATUS_BADREQUEST		"400 Bad Request"
#define HTTP_STATUS_UNAUTHORIZED	"401 Unauthorized"
#define HTTP_STATUS_FORBIDDEN		"403 Forbidden"
#define HTTP_STATUS_NOTFOUND		"404 File can not fonund!"
#define HTTP_STATUS_SERVERERROR		"500 Internal Server Error"
#define HTTP_STATUS_NOTIMPLEMENTED	"501 Not Implemented"
#define HTTP_STATUS_BADGATEWAY		"502 Bad Gateway"
#define HTTP_STATUS_UNAVAILABLE		"503 Service Unavailable"

// 连接的Client的信息
typedef struct CLIENT {
	SOCKET		socket;                   // 请求的socket
	SOCKADDR_IN addr;
	int			method;                   // 请求的使用方法：GET或HEAD
	int		    recvBytes;                // 收到的字节数
	int  		sentBytes;                // 发送的字节数
	HANDLE		hFile;                    // 请求连接的文件
	char		szFileName[_MAX_PATH];    // 文件的相对路径
	char		postfix[10];              // 存储扩展名
	char        statusCodeReason[100];    // 头部的status code 以及reason-phrase

	void*        pHttpProtocol;			  // 指向类CHttpProtocol的指针
}CLIENT, *PCLIENT;

// RECV结构体
typedef struct RECV {

	PCLIENT pClient;               // 哪个客户端
	char recvDataBuf[1024];        // 收到了什么数据

	RECV(PCLIENT p, const char* str) {
		pClient = p;
		strcpy(recvDataBuf, str);   // 值得注意的地方：我们直接复制一份recvBuf数据。而非直接使用指针
	}
	~RECV() {
		if (pClient != NULL) {
			delete pClient;
		}
		// delete recvDataBuf;
	}
}RECV, *PRECV;

class HttpProtocol {

public:
	HttpProtocol();
	HttpProtocol(const char* ip, int port);
	HttpProtocol(const char* ip, int port, const char* dir);
	~HttpProtocol();

	void CreateTypeMap();                                  // 创建文件扩展名与Http传输类型的映射
	bool StartHttpSrv();                                   // 开启http服务
	
	friend int analyze(PRECV pRecv);				       // 分析数据，设置客户端的相应字段
	friend void sendHeader(PRECV pRECV);                   // 发送头部
	friend void sendFile(PRECV pRecv);                 // 发送文件

	friend bool GetContenType(PRECV pRecv, LPSTR type);

	friend DWORD WINAPI listenThread(LPVOID param);        // 监听线程 accept
	friend DWORD WINAPI recvThread(LPVOID param);          // 接收线程 recv
	friend DWORD WINAPI sendThread(LPVOID param);          // 发送线程 send

private:
	
	const char* m_ip;		   // ip地址
	int m_port;				   // 端口号
	const char* m_webRootDir;  // web根目录
	
	SOCKET m_listenSocket;     // 监听的Socket
	HANDLE m_pListenThread;    // 监听线程

	std::map<char const*, const char *> m_typeMap;  // 保存content-type和文件后缀的对应关系map
};

