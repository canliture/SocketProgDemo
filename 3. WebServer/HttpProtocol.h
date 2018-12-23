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

// ���ӵ�Client����Ϣ
typedef struct CLIENT {
	SOCKET		socket;                   // �����socket
	SOCKADDR_IN addr;
	int			method;                   // �����ʹ�÷�����GET��HEAD
	int		    recvBytes;                // �յ����ֽ���
	int  		sentBytes;                // ���͵��ֽ���
	HANDLE		hFile;                    // �������ӵ��ļ�
	char		szFileName[_MAX_PATH];    // �ļ������·��
	char		postfix[10];              // �洢��չ��
	char        statusCodeReason[100];    // ͷ����status code �Լ�reason-phrase

	void*        pHttpProtocol;			  // ָ����CHttpProtocol��ָ��
}CLIENT, *PCLIENT;

// RECV�ṹ��
typedef struct RECV {

	PCLIENT pClient;               // �ĸ��ͻ���
	char recvDataBuf[1024];        // �յ���ʲô����

	RECV(PCLIENT p, const char* str) {
		pClient = p;
		strcpy(recvDataBuf, str);   // ֵ��ע��ĵط�������ֱ�Ӹ���һ��recvBuf���ݡ�����ֱ��ʹ��ָ��
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

	void CreateTypeMap();                                  // �����ļ���չ����Http�������͵�ӳ��
	bool StartHttpSrv();                                   // ����http����
	
	friend int analyze(PRECV pRecv);				       // �������ݣ����ÿͻ��˵���Ӧ�ֶ�
	friend void sendHeader(PRECV pRECV);                   // ����ͷ��
	friend void sendFile(PRECV pRecv);                 // �����ļ�

	friend bool GetContenType(PRECV pRecv, LPSTR type);

	friend DWORD WINAPI listenThread(LPVOID param);        // �����߳� accept
	friend DWORD WINAPI recvThread(LPVOID param);          // �����߳� recv
	friend DWORD WINAPI sendThread(LPVOID param);          // �����߳� send

private:
	
	const char* m_ip;		   // ip��ַ
	int m_port;				   // �˿ں�
	const char* m_webRootDir;  // web��Ŀ¼
	
	SOCKET m_listenSocket;     // ������Socket
	HANDLE m_pListenThread;    // �����߳�

	std::map<char const*, const char *> m_typeMap;  // ����content-type���ļ���׺�Ķ�Ӧ��ϵmap
};

