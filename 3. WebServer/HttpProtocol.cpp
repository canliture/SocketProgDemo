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

// 格林威治时间的星期转换
const char *week[] = { "Sun,", "Mon,", "Tue,", "Wed,", "Thu,", "Fri,", "Sat," };
// 格林威治时间的月份转换
const char *month[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

HttpProtocol::HttpProtocol() : HttpProtocol(LOCALHOST, HTTPPORT) {}  // 使用默认的构造函数
HttpProtocol::HttpProtocol(const char* ip, int port) {               // 传入服务器提供服务的IP地址，端口号
	m_ip = ip;
	m_port = port;
}

HttpProtocol::HttpProtocol(const char* ip, int port, const char* dir) : HttpProtocol(ip, port) {
	m_webRootDir = dir;

	// 创建文件扩展名http传输类型映射
	CreateTypeMap();
}

HttpProtocol::~HttpProtocol(){}


bool HttpProtocol::StartHttpSrv() {
	
	// 初始化socket
	if (GenericUtil::initWinSock() == false) {          
		return false;
	}

	// 创建套接字
	if ( (m_listenSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED) )== INVALID_SOCKET) {
		MsgUtil::declare("Could not create listen socket", MSG_ERROR);
		return false;
	}

	//if ((m_listenSocket = socket(AF_INET, SOCK_STREAM, 0) ) == INVALID_SOCKET) {
	//	MsgUtil::declare("Could not create listen socket", MSG_ERROR);
	//	return false;
	//}

	// 设置服务器的socket相关联的：< IP地址，Port号 >
	SOCKADDR_IN sockAddr;
	sockAddr.sin_family = AF_INET;
	sockAddr.sin_addr.S_un.S_addr = inet_addr(m_ip);

	LPSERVENT lpServEnt;
	if (m_port != 0) {
		sockAddr.sin_port = htons(m_port);           // 从主机字节顺序转为网络字节顺序赋给sin_port
	} else {
		lpServEnt = getservbyname("http", "tcp");     // 获取已知http 服务的端口，该服务在tcp 协议下注册
		if (lpServEnt != NULL) {
			sockAddr.sin_port = lpServEnt->s_port;
		} else {
			sockAddr.sin_port = htons(HTTPPORT);     // 默认端口HTTPPORT＝80
		}
	}

	// bind
	int iRet;
	if ((iRet = bind(m_listenSocket, (SOCKADDR*)&sockAddr, sizeof(SOCKADDR))) == SOCKET_ERROR) {
		MsgUtil::declare("bind() error", MSG_ERROR);
		closesocket(m_listenSocket); // 断开链接
		return false;
	}

	// listen
	if ((iRet = listen(m_listenSocket, SOMAXCONN)) == SOCKET_ERROR) {
		MsgUtil::declare("listen() error", MSG_ERROR);
		closesocket(m_listenSocket); // 断开链接
		return false;
	}

	// 控制台显示信息：服务器已经开启，<IP, Port>
	char hostname[255];
	gethostname(hostname, sizeof(hostname));        // 获得主机名
	char msg[64];
	sprintf(msg, "WebServer %s<%s:%d> is starting now...", hostname, m_ip, htons(sockAddr.sin_port));
	MsgUtil::declare("========== Http Server is starting now! ==========", MSG_LOG);
	MsgUtil::declare(msg, MSG_LOG);
	
	// 创建listening 进程，接受客户机连接要求
	// 主要执行accept and create RecvThread
	if (!(m_pListenThread = CreateThread(NULL, 0, listenThread, this, 0, NULL))) {
		// 线程创建失败
		MsgUtil::declare("Could not create listening thread", MSG_ERROR);
		closesocket(m_listenSocket); // 断开链接
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

		// 对每个客户端开启一个线程用于recv信息
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
	
	WSABUF			wsabuf;		// 发送/接收缓冲区结构
	WSAOVERLAPPED	over;		// 指向调用重叠操作时指定的WSAOVERLAPPED结构
	DWORD			dwRecv;
	DWORD			dwFlags;
	DWORD			dwRet;
	HANDLE			hEvents[2];
	bool			fPending;
	int				nRet;

	char pBuf[1024];
	int dwBufSize = 1024;
	memset(pBuf, 0, dwBufSize);	// 初始化缓冲区
	wsabuf.buf = (char *)pBuf;
	wsabuf.len = dwBufSize;	// 缓冲区的长度
	over.hEvent = WSACreateEvent();	// 创建一个新的事件对象
	dwFlags = 0;
	fPending = FALSE;

	while (1) {
		// 接收数据
		if ((nRet = WSARecv(pClient->socket, &wsabuf, 1, &dwRecv, &dwFlags, &over, NULL)) != 0) {          // WSARecv
			// 错误代码WSA_IO_PENDING表示重叠操作成功启动
			if (WSAGetLastError() != WSA_IO_PENDING) {
				// 重叠操作未能成功
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
			// 重叠操作未完成
			if (!WSAGetOverlappedResult(pClient->socket, &over, &dwRecv, FALSE, &dwFlags)) {
				CloseHandle(over.hEvent);
				return false;
			}
		}
		pClient->recvBytes += dwRecv;	// 统计接收数量
		// CloseHandle(over.hEvent);

		char buf[1024];
		// 显示接受到的什么数据
		sprintf(buf, "Receive message from <%s, %d>:\n%s\n", inet_ntoa((pClient->addr).sin_addr), (pClient->addr).sin_port, pBuf);
		MsgUtil::declare(buf, MSG_DEFAULT);

		// 创建发送信息线程。
		// 这里穿过去的recvBuf会复制一份，而非直接使用指针。
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
		

		// 统计接收数量
		pClient->recvBytes += strlen(recvBuf);

		// 显示接受到的什么数据
		sprintf(buf, "Receive message from <%s, %d>:\n%s\n", inet_ntoa((pClient->addr).sin_addr), (pClient->addr).sin_port, recvBuf);
		MsgUtil::declare(buf, MSG_DEFAULT);

		

		// 创建发送信息线程。
		// 这里穿过去的recvBuf会复制一份，而非直接使用指针。
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
	// 先分析数据
	if ( (iRet = analyze(pRecv)) == 1) {      // 处理错误
		MsgUtil::declare("Error occurs when analyzing client request", MSG_ERROR);
	}

	// 生成并返回头部
	sendHeader(pRecv);												       // SendHeader
	
	// 向Client 传送数据
	if (pClient->method == METHOD_GET) {
		sendFile(pRecv);												  // SendFile
	}

	return 0;
}

int analyze(PRECV pRecv)
{
	PCLIENT pClient = pRecv->pClient;

	const char* pBuf = pRecv->recvDataBuf;
	// 分析接收到的信息
	char szSeps[] = " \n";
	char *cpToken;
	// 防止非法请求
	if (strstr((const char *)pBuf, "..") != NULL) {
		strcpy(pClient->statusCodeReason, HTTP_STATUS_BADREQUEST);
		return 1;
	}
	// 判断Request的mothed
	cpToken = strtok((char *)pBuf, szSeps);	    // 缓存中字符串分解为一组标记串。	
	if (!_stricmp(cpToken, "GET")) {			// GET命令
		pClient->method = METHOD_GET;
	} else if (!_stricmp(cpToken, "HEAD")) {	// HEAD命令
		pClient->method = METHOD_HEAD;
	} else {
		strcpy(pClient->statusCodeReason, HTTP_STATUS_NOTIMPLEMENTED);
		return 1;
	}

	// 获取Request-URI
	cpToken = strtok(NULL, szSeps);
	if (cpToken == NULL) {
		strcpy(pClient->statusCodeReason, HTTP_STATUS_BADREQUEST);
		return 1;
	}

	// 这里要强转成HttpProtocol类型才能获取其字段，有点迷。。。
	strcpy(pClient->szFileName, ((HttpProtocol*)(pClient->pHttpProtocol))->m_webRootDir);
	if (strlen(cpToken) > 1) {
		strcat(pClient->szFileName, cpToken);	// 把该文件名添加到结尾处形成路径
	} else {
		strcat(pClient->szFileName, "/index.html");
	}
	return 0;
}

void sendHeader(PRECV pRecv) {

	PCLIENT pClient = pRecv->pClient;

	int n = FileExist(pClient);
	if (!n) { // 文件不存在，则返回
		return;
	}
	char Header[2048] = " ";
	char curTime[50] = " ";
	GetCurentTime((char*)curTime);
	// 取得文件长度
	DWORD length;
	length = GetFileSize(pClient->hFile, NULL);
	// 取得文件的last-modified 时间
	char last_modified[60] = " ";
	GetLastModified(pClient->hFile, (char*)last_modified);
	// 取得文件的类型
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

						// 发送头部
	send(pClient->socket, Header, strlen(Header), 0);
}

void sendFile(PRECV pRecv) {

	PCLIENT pClient = pRecv->pClient;
	HttpProtocol* pHttpProtocol = (HttpProtocol*)(pClient->pHttpProtocol);

	int n = FileExist(pClient);
	if (!n) { // 文件不存在，则返回

		char Header[128];
		sprintf((char*)Header,
			"HTTP/1.0 %s\r\n",
			HTTP_STATUS_NOTFOUND);

		// 发送头部
		send(pClient->socket, Header, strlen(Header)+1, 0);

		return;
	}
	
	MsgUtil::declare(&pClient->szFileName[strlen(pHttpProtocol->m_webRootDir)], MSG_LOG);

	static char buf[2048];
	unsigned long readBytes;
	bool fRet;
	int flag = 1;

	// 读写数据直到完成
	while (1) {
		// 从file 中读入到buffer 中
		fRet = ReadFile(pClient->hFile, buf, sizeof(buf), &readBytes, NULL);
		if (!fRet) {  // 如果出错就向客户端发送出错信息
			break;
		}
		// 读到的数据为0个，则算是读取完全
		if (readBytes == 0) {
			break;
		}
		// 将buffer 内容传送给client
		send(pClient->socket, buf, readBytes, 0);
		pClient->sentBytes += readBytes;
	} // end while 

	// 关闭文件
	if (CloseHandle(pClient->hFile)) {
		pClient->hFile = INVALID_HANDLE_VALUE;
	} else {
		MsgUtil::declare("Error occurs when closing file", MSG_ERROR);
	}
}


// 文件是否存在
int FileExist(PCLIENT pClient)
{
	pClient->hFile = CreateFile(pClient->szFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (pClient->hFile == INVALID_HANDLE_VALUE) {				 // 如果文件不存在，则返回出错信息
		strcpy(pClient->statusCodeReason, HTTP_STATUS_NOTFOUND);
		return 0;
	}
	return 1;
}

// 获取本地时间
void GetCurentTime(LPSTR lpszString) {
	// 活动本地时间
	SYSTEMTIME st;
	GetLocalTime(&st);
	// 时间格式化
	wsprintf(lpszString, "%s %02d %s %d %02d:%02d:%02d GMT", week[st.wDayOfWeek], st.wDay, month[st.wMonth - 1],
		st.wYear, st.wHour, st.wMinute, st.wSecond);
}

// 文件上次修改时间 
bool GetLastModified(HANDLE hFile, LPSTR lpszString)
{
	// 获得文件的last-modified 时间
	FILETIME ftCreate, ftAccess, ftWrite;
	SYSTEMTIME stCreate;
	FILETIME ftime;
	// 获得文件的last-modified的UTC时间
	if (!GetFileTime(hFile, &ftCreate, &ftAccess, &ftWrite))
		return false;
	FileTimeToLocalFileTime(&ftWrite, &ftime);
	// UTC时间转化成本地时间
	FileTimeToSystemTime(&ftime, &stCreate);
	// 时间格式化
	wsprintf(lpszString, "%s %02d %s %d %02d:%02d:%02d GMT", week[stCreate.wDayOfWeek],
		stCreate.wDay, month[stCreate.wMonth - 1], stCreate.wYear, stCreate.wHour,
		stCreate.wMinute, stCreate.wSecond);

	return true;
}

// 获取文件名类型
bool GetContenType(PRECV pRecv, LPSTR type)
{

	PCLIENT pClient = pRecv->pClient;

	// 取得文件的类型
	//CString cpToken;
	const char* cpToken = strstr(pClient->szFileName, ".");
	strcpy(pClient->postfix, cpToken);
	// 遍历搜索该文件类型对应的content-type
	std::map<char const*, const char *>::iterator it = ((HttpProtocol*)pClient->pHttpProtocol)->m_typeMap.find(pClient->postfix);
	if (it != ((HttpProtocol*)pClient->pHttpProtocol)->m_typeMap.end()) {
		wsprintf(type, "%s", (*it).second);
	}
	return TRUE;
}

void HttpProtocol::CreateTypeMap() {
	// 初始化map
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

