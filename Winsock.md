## Winsock 

### 1. socket大体流程

#### 1.1 Server

1. 初始化Winsock（Initialize Winsock）
2. 创建socket（Create a socket）
3. 绑定socket相关信息（Bind the socket）
4. socket监听连入客户端（Listen on the socket for a client）
5. 回应连接请求（Accept a connection from a client）
6. socket收发数据（Receive and send data）
7. 断开连接（Disconnect）

#### 1.2. Client

1. 初始化Winsock（Initialize Winsock）
2. 创建socket（Create a socket）
3. 连接服务端（Connect to the server）
4. 发送，接收数据（Send and receive data）
5. 断开连接（Disconnect）

### 2. 程序基本框架
```C++
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>

#pragma comment(lib, "Ws2_32.lib")

int main() {
  return 0;
}
```

> `Winsock2.h`
> The *Winsock2.h* header file contains most of the Winsock functions, structures, and definitions. The *Ws2tcpip.h* header file contains definitions introduced in the WinSock 2 Protocol-Specific Annex document for TCP/IP that includes newer functions and structures used to retrieve IP addresses.

### 3. 初始化Winsock

```C++
WSADATA wsaData;
int iResult;

// Initialize Winsock
iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
if (iResult != 0) {
    printf("WSAStartup failed: %d\n", iResult);
    return 1;
}
```

其中`WSAData`结构体的定义如下

```
// WSAData 
typedef struct WSAData {
	WORD           wVersion;
	WORD           wHighVersion;
	unsigned short iMaxSockets;	
	unsigned short iMaxUdpDg;
	char           *lpVendorInfo;
	char           szDescription[WSADESCRIPTION_LEN + 1];
	char           szSystemStatus[WSASYS_STATUS_LEN + 1];
} WSADATA;
```

### -------------- ↓ Server

```
// WSAData 
typedef struct WSAData {
	WORD           wVersion;
	WORD           wHighVersion;
	unsigned short iMaxSockets;	
	unsigned short iMaxUdpDg;
	char           *lpVendorInfo;
	char           szDescription[WSADESCRIPTION_LEN + 1];
	char           szSystemStatus[WSASYS_STATUS_LEN + 1];
} WSADATA;
```

### 4. 创建服务端的socket

getaddrinfo函数用来设置sockaddr结构体的值。

>  The **getaddrinfo** function provides protocol-independent translation from an ANSI host name to an address.

```C++
INT WSAAPI getaddrinfo(
  PCSTR           pNodeName,
  PCSTR           pServiceName,
  const ADDRINFOA *pHints,
  PADDRINFOA      *ppResult
);
```
ipv4 的sockaddr:
```C++
struct sockaddr {
        ushort  sa_family;
        char    sa_data[14];
};

struct sockaddr_in {
        short   sin_family;
        u_short sin_port;
        struct  in_addr sin_addr;
        char    sin_zero[8];
};
```

看下面的代码`getaddrinfo`相关代码

```C++
#define DEFAULT_PORT "27015"

struct addrinfo *result = NULL, *ptr = NULL, hints;

ZeroMemory(&hints, sizeof (hints));
hints.ai_family = AF_INET;         // 指定IPv4地址族（IPv4 address family）
hints.ai_socktype = SOCK_STREAM;   // 指定流socket（a stream socket）
hints.ai_protocol = IPPROTO_TCP;   // 指定TCP协议（the TCP protocol）
hints.ai_flags = AI_PASSIVE;       
/*hints.ai_flags = AI_PASSIVE;     // flag 
（1） 通常服务器端在调用getaddrinfo之前，ai_flags设置AI_PASSIVE，用于bind；主机名nodename通常会设置为NULL，返回通配地址[::]。
（2） 客户端调用getaddrinfo时，ai_flags一般不设置AI_PASSIVE，但是主机名nodename和服务名servname（更愿意称之为端口）则应该不为空。
（3） 当然，即使不设置AI_PASSIVE，取出的地址也并非不可以被bind，很多程序中ai_flags直接设置为0，即3个标志位都不设置，这种情况下只要hostname和servname设置的没有问题就可以正确bind。*/


// Resolve the local address and port to be used by the server
iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);      // up ↑ int iResult
if (iResult != 0) {
    printf("getaddrinfo failed: %d\n", iResult);
    WSACleanup();
    return 1;
}
```



创建socket，并且开始监听

```C++
SOCKET ListenSocket = INVALID_SOCKET;     // 创建socket。
// Create a SOCKET for the server to listen for client connections
ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
```

检查是否创建成功：

```C++
if (ListenSocket == INVALID_SOCKET) {
    printf("Error at socket(): %ld\n", WSAGetLastError());
    freeaddrinfo(result);
    WSACleanup();
    return 1;
}
```

### 5. 绑定socket

bind函数：

```C++
int bind(
    SOCKET         s,
    const sockaddr *addr,
    int            namelen
);
```



```C++
// Setup the TCP listening socket
iResult = bind( ListenSocket, result->ai_addr, (int)result->ai_addrlen);
if (iResult == SOCKET_ERROR) {
    printf("bind failed with error: %d\n", WSAGetLastError());
    freeaddrinfo(result);
    closesocket(ListenSocket);
    WSACleanup();
    return 1;
}
// 绑定成功后，result也没用了。释放
freeaddrinfo(result);
```

### 6. 监听socket

```C++
// SOMAXCONN常数；命令winsock 允许最大连接数
if ( listen( ListenSocket, SOMAXCONN ) == SOCKET_ERROR ) {
    printf( "Listen failed with error: %ld\n", WSAGetLastError() );
    closesocket(ListenSocket);
    WSACleanup();
    return 1;
}
```

### 7. 处理连接请求

```C++
SOCKET ClientSocket; // 创建一个临时的Socket对象接收客户端的socket
```

通常情况下服务端程序会被设计成能够监听多个客户端的连接。对于高性能的服务器，多线程处理客户端请求是必要的。

监听多个客户端的连接通常有几种方法。`一种技术就是弄一个死循环来持续监测（使用上面所说的listen函数）客户端的连接请求。`连接一到，服务端程序就调用[**`accept`**](https://docs.microsoft.com/en-us/windows/desktop/api/Winsock2/nf-winsock2-accept), [**`AcceptEx`**](https://msdn.microsoft.com/en-us/library/ms737524(v=VS.85).aspx), or [**`WSAAccept`**](https://docs.microsoft.com/en-us/windows/desktop/api/Winsock2/nf-winsock2-wsaaccept) 函数，然后将剩下的工作传递给另一个线程来处理。



不使用多线程的方法代码：

```C++
ClientSocket = INVALID_SOCKET;

// Accept a client socket
ClientSocket = accept(ListenSocket, NULL, NULL);
if (ClientSocket == INVALID_SOCKET) {
    printf("accept failed: %d\n", WSAGetLastError());
    closesocket(ListenSocket);
    WSACleanup();
    return 1;
}
```

其中accept函数

```C++
SOCKET WSAAPI accept(
  SOCKET   s,
  sockaddr *addr,
  int      *addrlen
);
```

当然还有许多其它的方法来监听并接收多个客户端连接，比如这两个函数：`select`，`WSAPoll`。

### 8. 服务端取出数据并且回应数据

主要的关注点在`recv`函数和`send`函数

下面是两个函数的定义格式，它们的`返回值都是接收/发送的数据byte数`：

```C++
// recv函数
int recv(
  SOCKET s,
  char   *buf,
  int    len,
  int    flags
);
// send 函数
int WSAAPI send(
  SOCKET     s,
  const char *buf,
  int        len,
  int        flags
);

```



```C++
#define DEFAULT_BUFLEN 512

char recvbuf[DEFAULT_BUFLEN];        // 定义接收数据缓冲区
int iResult, iSendResult;            // 
int recvbuflen = DEFAULT_BUFLEN;    

// Receive until the peer shuts down the connection
do {
	// 接收数据
    iResult = recv(ClientSocket, recvbuf, recvbuflen, 0);
    
    if (iResult > 0) {    // 如果收到了数据
        printf("Bytes received: %d\n", iResult);

        // Echo the buffer back to the sender
        iSendResult = send(ClientSocket, recvbuf, iResult, 0);
        
        if (iSendResult == SOCKET_ERROR) {
            printf("send failed: %d\n", WSAGetLastError());
            closesocket(ClientSocket);
            WSACleanup();
            return 1;
        }
        printf("Bytes sent: %d\n", iSendResult);
    } else if (iResult == 0)
        printf("Connection closing...\n");
    else {
        printf("recv failed: %d\n", WSAGetLastError());
        closesocket(ClientSocket);
        WSACleanup();
        return 1;
    }

} while (iResult > 0);
```

### 9. 断开连接

断开并且关闭socket（disconnect and shutdown a socket）：

```C++
// shutdown the send half of the connection since no more data will be sent
// shutdown ，一端关闭连接
iResult = shutdown(ClientSocket, SD_SEND);
if (iResult == SOCKET_ERROR) {
    printf("shutdown failed: %d\n", WSAGetLastError());
    closesocket(ClientSocket);
    WSACleanup();
    return 1;
}

// 当客户端程序已经完成接收数据， 调用closesocket关闭socket
// 当客户端程序完成Windows Sockets DLL的使用，调用WSACleanup释放资源
// cleanup
closesocket(ClientSocket);
WSACleanup();

return 0;     // 整个server端程序结束了

```

### -------------- ↑Server，↓Client

### 10. 创建客户端Socket

1. 声明一个`addrinfo`对象，它包含一个`sockaddr`结构，然后初始化此addrinfo对象。

> Declare an [**addrinfo**](https://msdn.microsoft.com/en-us/library/ms737530(v=VS.85).aspx) object that contains a [**sockaddr**](https://docs.microsoft.com/zh-cn/windows/desktop/WinSock/sockaddr-2) structure and initialize these values. 

```c++
struct addrinfo *result = NULL,
			   *ptr = NULL,
                hints;
ZeroMemory( &hints, sizeof(hints) );
// 设置addrinfo的信息
hints.ai_family = AF_UNSPEC;
hints.ai_socktype = SOCK_STREAM;
hints.ai_protocol = IPPROTO_TCP;
```

2. `getaddrinfo`函数来请求

```C++
#define DEFAULT_PORT "27015"
// Resolve the server address and port
iResult = getaddrinfo(argv[1], DEFAULT_PORT, &hints, &result);
if (iResult != 0) {
    printf("getaddrinfo failed: %d\n", iResult);
    WSACleanup();
    return 1;
}
```

3. 创建一个socket对象

   ```C++
   SOCKET ConnectSocket = INVALID_SOCKET;
   ```

4. 调用socket函数返回`ConnectSocket`变量的值

   ```C++
   // Attempt to connect to the first address returned by
   // the call to getaddrinfo
   ptr = result;
   
   // Create a SOCKET for connecting to server
   ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
   ```

5. 检查错误

   ```C++
   if (ConnectSocket == INVALID_SOCKET) {
       printf("Error at socket(): %ld\n", WSAGetLastError());
       freeaddrinfo(result);
       WSACleanup();
       return 1;
   }
   ```

### 12. 连接Socket

调用connect函数，传递一个已经创建的socket和sockaddr结构体作为参数，同时，检查错误。

```C++
// Connect to server.
iResult = connect( ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
if (iResult == SOCKET_ERROR) {
    closesocket(ConnectSocket);
    ConnectSocket = INVALID_SOCKET;
}

// Should really try the next address returned by getaddrinfo
// if the connect call failed
// But for this simple example we just free the resources
// returned by getaddrinfo and print an error message

freeaddrinfo(result);

if (ConnectSocket == INVALID_SOCKET) {
    printf("Unable to connect to server!\n");
    WSACleanup();
    return 1;
}
```

### 13.  发送，接收数据

一旦建立连接，我们就可以开始收发数据了，关键点在于 [**send**](https://docs.microsoft.com/en-us/windows/desktop/api/Winsock2/nf-winsock2-send) and [**recv**](https://docs.microsoft.com/en-us/windows/desktop/api/winsock/nf-winsock-recv)函数。

```C++
#define DEFAULT_BUFLEN 512

int recvbuflen = DEFAULT_BUFLEN;

char *sendbuf = "this is a test";
char recvbuf[DEFAULT_BUFLEN];

int iResult;

// Send an initial buffer
iResult = send(ConnectSocket, sendbuf, (int) strlen(sendbuf), 0);
if (iResult == SOCKET_ERROR) {
    printf("send failed: %d\n", WSAGetLastError());
    closesocket(ConnectSocket);
    WSACleanup();
    return 1;
}

printf("Bytes Sent: %ld\n", iResult);

// shutdown the connection for sending since no more data will be sent
// the client can still use the ConnectSocket for receiving data
iResult = shutdown(ConnectSocket, SD_SEND);
if (iResult == SOCKET_ERROR) {
    printf("shutdown failed: %d\n", WSAGetLastError());
    closesocket(ConnectSocket);
    WSACleanup();
    return 1;
}

// Receive data until the server closes the connection
do {
    iResult = recv(ConnectSocket, recvbuf, recvbuflen, 0);
    if (iResult > 0)
        printf("Bytes received: %d\n", iResult);
    else if (iResult == 0)
        printf("Connection closed\n");
    else
        printf("recv failed: %d\n", WSAGetLastError());
} while (iResult > 0);
```

### 14.  客户端断开连接

一旦客户端完成发送，接收数据，客户端就能断开服务器连接，并且shutdown socket。

**To disconnect and shutdown a socket**

1. 调用shutdown函数并且指定SD_SEND来关闭客户端socket的发送端。但是`应用程序仍然能够在socket上取出数据`。

```C++
// shutdown the send half of the connection since no more data will be sent
iResult = shutdown(ConnectSocket, SD_SEND);
if (iResult == SOCKET_ERROR) {
    printf("shutdown failed: %d\n", WSAGetLastError());
    closesocket(ConnectSocket);
    WSACleanup();
    return 1;
}
```



2. 当客户端完全接受数据之后，调用closesocket函数关闭socket。当应用程序完成Windows Sockets DLL的使用之后，调用`WSACleanup`函数来释放资源。

   ```C++
   // cleanup
   closesocket(ConnectSocket);
   WSACleanup();
   
   return 0;
   ```



## CHAT_WITH_ME_CHATROOM Proj

```C++
Usage: 
Welcome to CHAT_WITH_ME_CHATROOM chat room! \n
Before you enjoy the chatroom, you need to notice the following tips: \n
<N>: represent the number of a client. \n
<Contents> : represent the contents we want to send  \n

<contents>                  // 发送信息给所有客户端                  \n

// 私聊
cmd_to_usr <N> <Contents>   // to a user        [私聊]发送消息给人  \n

// 群聊
cmd_to_grp <N> <Contents>   // to a grp         发送消息给群        \n
cmd_in_grp <N>			   // in a group       加入一个群          \n
cmd_ot_grp <N>              // 'out' a group    退出一个群          \n
cmd_su_grp   			   // set up a group   建立一个群          \n

// 好友
cmd_fw_frd <N>              // follow a friend    加好友            \n
cmd_dl_frd <N>              // delete a friend    删好友            \n

// 列出服务器相关信息
cmd_ls_usr                  // list all the users 列出所有在线用户   \n
cmd_ls_grp                  // list all the goups 列出所有的群       \n


/**
Welcome to CHAT_WITH_ME_CHATROOM chat room!
Before you enjoy the chatroom, you need to notice the following tips :

<N> : represent the number of a client.
<Contents> : represent the contents we want to send.

<contents>                  -- to all users       [默认广播]信息给所有客户端
cmd_to_usr <N> <Contents>   -- to a user          [私聊]发送消息给人
cmd_to_grp <N> <Contents>   -- to a grp           [群聊]发送消息给群
cmd_in_grp <N>              -- in a group         [加群]加入一个群
cmd_ot_grp <N>              -- 'out' a group      [退群]退出一个群
cmd_su_grp                  -- set up a group     [建群]建立一个群
cmd_ls_usr                  -- list all the users 列出所有在线用户
cmd_ls_grp                  -- list all the goups 列出所有的群
**/


/*******************************************************************/

if(recvBuf < 10){          
	sendToAll(recvBuf);
}
/** 命令分类(全部的情况有10种，)：*/
cmd = recvbuf[0, 9];

if( cmd == ("cmd_to_all" || "cmd_su_grp" || "cmd_ls_usr" || "cmd_ls_grp") ){
    switch(cmd){
    	"cmd_to_all": /*funciont here*/ break;
        "cmd_su_grp": /*funciont here*/ break;
        "cmd_ls_usr": /*funciont here*/ break;
        "cmd_ls_grp": /*funciont here*/ break;
        default: sendToAll();
    }
} else {
	int id = getIdFromRecvBuf();
    
}
```



## tracert代码填坑

- [只有超级用户才能创建原始套接字，这么做可以防止普通用户网网络写自行构造的IP数据报。](https://blog.csdn.net/gqtcgq/article/details/44874349)
- [防火墙可能拦截了ICMP数据包，如果程序不能运行，尝试关闭防火墙。](https://blog.csdn.net/u013271921/article/details/45488173)
  - 当然，一些杀毒软件也会进行一定的安全策略设置。

- 我连接的校园网（wifi：HNUST）tracert 百度，除了最终的地址，其余全部是超时。而连接我手机的热点wifi则可以正常运行程序。
  - 猜测：校园网的路由器可能有一些安全策略将ICMP包拦截在外。

