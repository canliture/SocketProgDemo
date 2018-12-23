#include "HttpProtocol.h"
#include "GenericUtil.h"
#include "MsgUtil.h"

DWORD WINAPI listenThread(LPVOID param); 
DWORD WINAPI recvThread(LPVOID param);
DWORD WINAPI sendThread(LPVOID param);

int analyze(PRECV pRecv);
void sendHeader(PRECV pRECV);
void sendFile(PRECV pRecv);
int FileExist(PCLIENT pClient);
bool GetContenType(PRECV pRecv, LPSTR type);
void GetCurentTime(LPSTR lpszString);
bool GetLastModified(HANDLE hFile, LPSTR lpszString);

// ��������ʱ�������ת��
const char *week[] = { "Sun,", "Mon,", "Tue,", "Wed,", "Thu,", "Fri,", "Sat," };
// ��������ʱ����·�ת��
const char *month[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

HttpProtocol::HttpProtocol() : HttpProtocol(LOCALHOST, HTTPPORT) {}  // ʹ��Ĭ�ϵĹ��캯��
HttpProtocol::HttpProtocol(const char* ip, int port) {               // ����������ṩ�����IP��ַ���˿ں�
	m_ip = ip;
	m_port = port;
}

HttpProtocol::HttpProtocol(const char* ip, int port, const char* dir) : HttpProtocol(ip, port) {
	m_webRootDir = dir;

	// �����ļ���չ��http��������ӳ��
	CreateTypeMap();
}

HttpProtocol::~HttpProtocol(){}


bool HttpProtocol::StartHttpSrv() {
	
	// ��ʼ��socket
	if (GenericUtil::initWinSock() == false) {          
		return false;
	}

	// �����׽���
	if ( (m_listenSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED) )== INVALID_SOCKET) {
		MsgUtil::declare("Could not create listen socket", MSG_ERROR);
		return false;
	}

	//if ((m_listenSocket = socket(AF_INET, SOCK_STREAM, 0) ) == INVALID_SOCKET) {
	//	MsgUtil::declare("Could not create listen socket", MSG_ERROR);
	//	return false;
	//}

	// ���÷�������socket������ģ�< IP��ַ��Port�� >
	SOCKADDR_IN sockAddr;
	sockAddr.sin_family = AF_INET;
	sockAddr.sin_addr.S_un.S_addr = inet_addr(m_ip);

	LPSERVENT lpServEnt;
	if (m_port != 0) {
		sockAddr.sin_port = htons(m_port);           // �������ֽ�˳��תΪ�����ֽ�˳�򸳸�sin_port
	} else {
		lpServEnt = getservbyname("http", "tcp");     // ��ȡ��֪http ����Ķ˿ڣ��÷�����tcp Э����ע��
		if (lpServEnt != NULL) {
			sockAddr.sin_port = lpServEnt->s_port;
		} else {
			sockAddr.sin_port = htons(HTTPPORT);     // Ĭ�϶˿�HTTPPORT��80
		}
	}

	// bind
	int iRet;
	if ((iRet = bind(m_listenSocket, (SOCKADDR*)&sockAddr, sizeof(SOCKADDR))) == SOCKET_ERROR) {
		MsgUtil::declare("bind() error", MSG_ERROR);
		closesocket(m_listenSocket); // �Ͽ�����
		return false;
	}

	// listen
	if ((iRet = listen(m_listenSocket, SOMAXCONN)) == SOCKET_ERROR) {
		MsgUtil::declare("listen() error", MSG_ERROR);
		closesocket(m_listenSocket); // �Ͽ�����
		return false;
	}

	// ����̨��ʾ��Ϣ���������Ѿ�������<IP, Port>
	char hostname[255];
	gethostname(hostname, sizeof(hostname));        // ���������
	char msg[64];
	sprintf(msg, "WebServer %s<%s:%d> is starting now...", hostname, m_ip, htons(sockAddr.sin_port));
	MsgUtil::declare("========== Http Server is starting now! ==========", MSG_LOG);
	MsgUtil::declare(msg, MSG_LOG);
	
	// ����listening ���̣����ܿͻ�������Ҫ��
	// ��Ҫִ��accept and create RecvThread
	if (!(m_pListenThread = CreateThread(NULL, 0, listenThread, this, 0, NULL))) {
		// �̴߳���ʧ��
		MsgUtil::declare("Could not create listening thread", MSG_ERROR);
		closesocket(m_listenSocket); // �Ͽ�����
		return false;
	}

	return true;
}

DWORD WINAPI listenThread(LPVOID param) {
	
	HttpProtocol* pHttpProtocol = (HttpProtocol*)param;

	SOCKET clientSocket;
	SOCKADDR_IN clientAddr;
	int len = sizeof(SOCKADDR_IN);

	while (1) {
		// accept 
		if ((clientSocket = accept(pHttpProtocol->m_listenSocket, (SOCKADDR*)&clientAddr, &len)) == INVALID_SOCKET) {
			MsgUtil::declare("Error accept ... Continue accept.", MSG_ERROR);
			continue;
		}

		// accept successfully 

		char buf[64];
		sprintf(buf, "<%s, %d> is connecting to me...", inet_ntoa(clientAddr.sin_addr), clientAddr.sin_port);
		MsgUtil::declare(buf, MSG_LOG);

		// ��ÿ���ͻ��˿���һ���߳�����recv��Ϣ
		PCLIENT pClient = new CLIENT();
		pClient->socket = clientSocket;
		pClient->addr = clientAddr;
		pClient->hFile = INVALID_HANDLE_VALUE;
		pClient->recvBytes = 0;
		pClient->sentBytes = 0;
		pClient->pHttpProtocol = pHttpProtocol;

		HANDLE handle = CreateThread(NULL, 0, recvThread, pClient, 0, NULL);
	}
}

DWORD WINAPI recvThread(LPVOID param) {
	
	PCLIENT pClient = (PCLIENT)param;
	HttpProtocol *pHttpProtocol = (HttpProtocol *)pClient->pHttpProtocol;
	
	WSABUF			wsabuf;		// ����/���ջ������ṹ
	WSAOVERLAPPED	over;		// ָ������ص�����ʱָ����WSAOVERLAPPED�ṹ
	DWORD			dwRecv;
	DWORD			dwFlags;
	DWORD			dwRet;
	HANDLE			hEvents[2];
	bool			fPending;
	int				nRet;

	char pBuf[1024];
	int dwBufSize = 1024;
	memset(pBuf, 0, dwBufSize);	// ��ʼ��������
	wsabuf.buf = (char *)pBuf;
	wsabuf.len = dwBufSize;	// �������ĳ���
	over.hEvent = WSACreateEvent();	// ����һ���µ��¼�����
	dwFlags = 0;
	fPending = FALSE;

	while (1) {
		// ��������
		if ((nRet = WSARecv(pClient->socket, &wsabuf, 1, &dwRecv, &dwFlags, &over, NULL)) != 0) {          // WSARecv
			// �������WSA_IO_PENDING��ʾ�ص������ɹ�����
			if (WSAGetLastError() != WSA_IO_PENDING) {
				// �ص�����δ�ܳɹ�
				CloseHandle(over.hEvent);
				return false;
			}
			else {
				fPending = true;
			}
		}
		if (fPending) {
			hEvents[0] = over.hEvent;
			// hEvents[1] = pReq->hExit;

			if ((dwRet = WaitForMultipleObjects(2, hEvents, FALSE, INFINITE)) != 0) {
				CloseHandle(over.hEvent);
				return false;
			}
			// �ص�����δ���
			if (!WSAGetOverlappedResult(pClient->socket, &over, &dwRecv, FALSE, &dwFlags)) {
				CloseHandle(over.hEvent);
				return false;
			}
		}
		pClient->recvBytes += dwRecv;	// ͳ�ƽ�������
		// CloseHandle(over.hEvent);

		char buf[1024];
		// ��ʾ���ܵ���ʲô����
		sprintf(buf, "Receive message from <%s, %d>:\n%s\n", inet_ntoa((pClient->addr).sin_addr), (pClient->addr).sin_port, pBuf);
		MsgUtil::declare(buf, MSG_DEFAULT);

		// ����������Ϣ�̡߳�
		// ���ﴩ��ȥ��recvBuf�Ḵ��һ�ݣ�����ֱ��ʹ��ָ�롣
		PRECV pRecv = new RECV(pClient, pBuf);
		HANDLE handle = CreateThread(NULL, 0, sendThread, pRecv, 0, NULL);
	} // end while 
	/*
	char recvBuf[1024];
	char buf[64];
	while (true) {
		
		int iRet = recv(pClient->socket, recvBuf, 1024, 0);
		if ((iRet <= 0)) {
			// error handler
			sprintf(buf, "%s:%d disconnects to me...", inet_ntoa((pClient->addr).sin_addr), (pClient->addr).sin_port);
			MsgUtil::declare(buf, MSG_DEFAULT);
			closesocket(pClient->socket);
			break;
		}
		

		// ͳ�ƽ�������
		pClient->recvBytes += strlen(recvBuf);

		// ��ʾ���ܵ���ʲô����
		sprintf(buf, "Receive message from <%s, %d>:\n%s\n", inet_ntoa((pClient->addr).sin_addr), (pClient->addr).sin_port, recvBuf);
		MsgUtil::declare(buf, MSG_DEFAULT);

		

		// ����������Ϣ�̡߳�
		// ���ﴩ��ȥ��recvBuf�Ḵ��һ�ݣ�����ֱ��ʹ��ָ�롣
		PRECV pRecv = new RECV(pClient, recvBuf);
		HANDLE handle = CreateThread(NULL, 0, sendThread, pRecv, 0, NULL);

	}// end while
	*/
	return 0;
}

DWORD WINAPI sendThread(LPVOID param) {
	
	PRECV pRecv = (PRECV)param;
	
	PCLIENT pClient = pRecv->pClient;

	int iRet;
	// �ȷ�������
	if ( (iRet = analyze(pRecv)) == 1) {      // �������
		MsgUtil::declare("Error occurs when analyzing client request", MSG_ERROR);
	}

	// ���ɲ�����ͷ��
	sendHeader(pRecv);												       // SendHeader
	
	// ��Client ��������
	if (pClient->method == METHOD_GET) {
		sendFile(pRecv);												  // SendFile
	}

	return 0;
}

int analyze(PRECV pRecv)
{
	PCLIENT pClient = pRecv->pClient;

	const char* pBuf = pRecv->recvDataBuf;
	// �������յ�����Ϣ
	char szSeps[] = " \n";
	char *cpToken;
	// ��ֹ�Ƿ�����
	if (strstr((const char *)pBuf, "..") != NULL) {
		strcpy(pClient->statusCodeReason, HTTP_STATUS_BADREQUEST);
		return 1;
	}
	// �ж�Request��mothed
	cpToken = strtok((char *)pBuf, szSeps);	    // �������ַ����ֽ�Ϊһ���Ǵ���	
	if (!_stricmp(cpToken, "GET")) {			// GET����
		pClient->method = METHOD_GET;
	} else if (!_stricmp(cpToken, "HEAD")) {	// HEAD����
		pClient->method = METHOD_HEAD;
	} else {
		strcpy(pClient->statusCodeReason, HTTP_STATUS_NOTIMPLEMENTED);
		return 1;
	}

	// ��ȡRequest-URI
	cpToken = strtok(NULL, szSeps);
	if (cpToken == NULL) {
		strcpy(pClient->statusCodeReason, HTTP_STATUS_BADREQUEST);
		return 1;
	}

	// ����Ҫǿת��HttpProtocol���Ͳ��ܻ�ȡ���ֶΣ��е��ԡ�����
	strcpy(pClient->szFileName, ((HttpProtocol*)(pClient->pHttpProtocol))->m_webRootDir);
	if (strlen(cpToken) > 1) {
		strcat(pClient->szFileName, cpToken);	// �Ѹ��ļ�����ӵ���β���γ�·��
	} else {
		strcat(pClient->szFileName, "/index.html");
	}
	return 0;
}

void sendHeader(PRECV pRecv) {

	PCLIENT pClient = pRecv->pClient;

	int n = FileExist(pClient);
	if (!n) { // �ļ������ڣ��򷵻�
		return;
	}
	char Header[2048] = " ";
	char curTime[50] = " ";
	GetCurentTime((char*)curTime);
	// ȡ���ļ�����
	DWORD length;
	length = GetFileSize(pClient->hFile, NULL);
	// ȡ���ļ���last-modified ʱ��
	char last_modified[60] = " ";
	GetLastModified(pClient->hFile, (char*)last_modified);
	// ȡ���ļ�������
	char ContenType[50] = " ";
	GetContenType(pRecv, (char*)ContenType);

	sprintf((char*)Header,
		"HTTP/1.0 %s\r\nDate: %s\r\nServer: %s\r\nContent-Type: %s\r\nContent-Length: %d\r\nLast-Modified: %s\r\n\r\n",
		HTTP_STATUS_OK,
		curTime, // Date
		"Hi Http Server!", // Server
		ContenType, // Content-Type
		length, // Content-length
		last_modified); // Last-Modified

						// ����ͷ��
	send(pClient->socket, Header, strlen(Header), 0);
}

void sendFile(PRECV pRecv) {

	PCLIENT pClient = pRecv->pClient;
	HttpProtocol* pHttpProtocol = (HttpProtocol*)(pClient->pHttpProtocol);

	int n = FileExist(pClient);
	if (!n) { // �ļ������ڣ��򷵻�

		char Header[128];
		sprintf((char*)Header,
			"HTTP/1.0 %s\r\n",
			HTTP_STATUS_NOTFOUND);

		// ����ͷ��
		send(pClient->socket, Header, strlen(Header)+1, 0);

		return;
	}
	
	MsgUtil::declare(&pClient->szFileName[strlen(pHttpProtocol->m_webRootDir)], MSG_LOG);

	static char buf[2048];
	unsigned long readBytes;
	bool fRet;
	int flag = 1;

	// ��д����ֱ�����
	while (1) {
		// ��file �ж��뵽buffer ��
		fRet = ReadFile(pClient->hFile, buf, sizeof(buf), &readBytes, NULL);
		if (!fRet) {  // ����������ͻ��˷��ͳ�����Ϣ
			break;
		}
		// ����������Ϊ0���������Ƕ�ȡ��ȫ
		if (readBytes == 0) {
			break;
		}
		// ��buffer ���ݴ��͸�client
		send(pClient->socket, buf, readBytes, 0);
		pClient->sentBytes += readBytes;
	} // end while 

	// �ر��ļ�
	if (CloseHandle(pClient->hFile)) {
		pClient->hFile = INVALID_HANDLE_VALUE;
	} else {
		MsgUtil::declare("Error occurs when closing file", MSG_ERROR);
	}
}


// �ļ��Ƿ����
int FileExist(PCLIENT pClient)
{
	pClient->hFile = CreateFile(pClient->szFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (pClient->hFile == INVALID_HANDLE_VALUE) {				 // ����ļ������ڣ��򷵻س�����Ϣ
		strcpy(pClient->statusCodeReason, HTTP_STATUS_NOTFOUND);
		return 0;
	}
	return 1;
}

// ��ȡ����ʱ��
void GetCurentTime(LPSTR lpszString) {
	// �����ʱ��
	SYSTEMTIME st;
	GetLocalTime(&st);
	// ʱ���ʽ��
	wsprintf(lpszString, "%s %02d %s %d %02d:%02d:%02d GMT", week[st.wDayOfWeek], st.wDay, month[st.wMonth - 1],
		st.wYear, st.wHour, st.wMinute, st.wSecond);
}

// �ļ��ϴ��޸�ʱ�� 
bool GetLastModified(HANDLE hFile, LPSTR lpszString)
{
	// ����ļ���last-modified ʱ��
	FILETIME ftCreate, ftAccess, ftWrite;
	SYSTEMTIME stCreate;
	FILETIME ftime;
	// ����ļ���last-modified��UTCʱ��
	if (!GetFileTime(hFile, &ftCreate, &ftAccess, &ftWrite))
		return false;
	FileTimeToLocalFileTime(&ftWrite, &ftime);
	// UTCʱ��ת���ɱ���ʱ��
	FileTimeToSystemTime(&ftime, &stCreate);
	// ʱ���ʽ��
	wsprintf(lpszString, "%s %02d %s %d %02d:%02d:%02d GMT", week[stCreate.wDayOfWeek],
		stCreate.wDay, month[stCreate.wMonth - 1], stCreate.wYear, stCreate.wHour,
		stCreate.wMinute, stCreate.wSecond);

	return true;
}

// ��ȡ�ļ�������
bool GetContenType(PRECV pRecv, LPSTR type)
{

	PCLIENT pClient = pRecv->pClient;

	// ȡ���ļ�������
	//CString cpToken;
	const char* cpToken = strstr(pClient->szFileName, ".");
	strcpy(pClient->postfix, cpToken);
	// �����������ļ����Ͷ�Ӧ��content-type
	std::map<char const*, const char *>::iterator it = ((HttpProtocol*)pClient->pHttpProtocol)->m_typeMap.find(pClient->postfix);
	if (it != ((HttpProtocol*)pClient->pHttpProtocol)->m_typeMap.end()) {
		wsprintf(type, "%s", (*it).second);
	}
	return TRUE;
}

void HttpProtocol::CreateTypeMap() {
	// ��ʼ��map
	m_typeMap[".doc"] = "application/msword";
	m_typeMap[".bin"] = "application/octet-stream";
	m_typeMap[".dll"] = "application/octet-stream";
	m_typeMap[".exe"] = "application/octet-stream";
	m_typeMap[".pdf"] = "application/pdf";
	m_typeMap[".ai"] = "application/postscript";
	m_typeMap[".eps"] = "application/postscript";
	m_typeMap[".ps"] = "application/postscript";
	m_typeMap[".rtf"] = "application/rtf";
	m_typeMap[".fdf"] = "application/vnd.fdf";
	m_typeMap[".arj"] = "application/x-arj";
	m_typeMap[".gz"] = "application/x-gzip";
	m_typeMap[".class"] = "application/x-java-class";
	m_typeMap[".js"] = "application/x-javascript";
	m_typeMap[".lzh"] = "application/x-lzh";
	m_typeMap[".lnk"] = "application/x-ms-shortcut";
	m_typeMap[".tar"] = "application/x-tar";
	m_typeMap[".hlp"] = "application/x-winhelp";
	m_typeMap[".cert"] = "application/x-x509-ca-cert";
	m_typeMap[".zip"] = "application/zip";
	m_typeMap[".cab"] = "application/x-compressed";
	m_typeMap[".arj"] = "application/x-compressed";
	m_typeMap[".aif"] = "audio/aiff";
	m_typeMap[".aifc"] = "audio/aiff";
	m_typeMap[".aiff"] = "audio/aiff";
	m_typeMap[".au"] = "audio/basic";
	m_typeMap[".snd"] = "audio/basic";
	m_typeMap[".mid"] = "audio/midi";
	m_typeMap[".rmi"] = "audio/midi";
	m_typeMap[".mp3"] = "audio/mpeg";
	m_typeMap[".vox"] = "audio/voxware";
	m_typeMap[".wav"] = "audio/wav";
	m_typeMap[".ra"] = "audio/x-pn-realaudio";
	m_typeMap[".ram"] = "audio/x-pn-realaudio";
	m_typeMap[".bmp"] = "image/bmp";
	m_typeMap[".gif"] = "image/gif";
	m_typeMap[".jpeg"] = "image/jpeg";
	m_typeMap[".jpg"] = "image/jpeg";
	m_typeMap[".tif"] = "image/tiff";
	m_typeMap[".tiff"] = "image/tiff";
	m_typeMap[".xbm"] = "image/xbm";
	m_typeMap[".wrl"] = "model/vrml";
	m_typeMap[".htm"] = "text/html";
	m_typeMap[".html"] = "text/html";
	m_typeMap[".c"] = "text/plain";
	m_typeMap[".cpp"] = "text/plain";
	m_typeMap[".def"] = "text/plain";
	m_typeMap[".h"] = "text/plain";
	m_typeMap[".txt"] = "text/plain";
	m_typeMap[".rtx"] = "text/richtext";
	m_typeMap[".rtf"] = "text/richtext";
	m_typeMap[".java"] = "text/x-java-source";
	m_typeMap[".css"] = "text/css";
	m_typeMap[".mpeg"] = "video/mpeg";
	m_typeMap[".mpg"] = "video/mpeg";
	m_typeMap[".mpe"] = "video/mpeg";
	m_typeMap[".avi"] = "video/msvideo";
	m_typeMap[".mov"] = "video/quicktime";
	m_typeMap[".qt"] = "video/quicktime";
	m_typeMap[".shtml"] = "wwwserver/html-ssi";
	m_typeMap[".asa"] = "wwwserver/isapi";
	m_typeMap[".asp"] = "wwwserver/isapi";
	m_typeMap[".cfm"] = "wwwserver/isapi";
	m_typeMap[".dbm"] = "wwwserver/isapi";
	m_typeMap[".isa"] = "wwwserver/isapi";
	m_typeMap[".plx"] = "wwwserver/isapi";
	m_typeMap[".url"] = "wwwserver/isapi";
	m_typeMap[".cgi"] = "wwwserver/isapi";
	m_typeMap[".php"] = "wwwserver/isapi";
	m_typeMap[".wcgi"] = "wwwserver/isapi";
}

