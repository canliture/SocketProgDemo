#include <ws2tcpip.h>

//ICMP 类型字段
const BYTE ICMP_ECHO_REQUEST = 8; //请求回显
const BYTE ICMP_ECHO_REPLY = 0; //回显应答
const BYTE ICMP_TIMEOUT = 11; //传输超时

//其他常量定义
const int DEF_ICMP_DATA_SIZE = 32; //ICMP 报文默认数据字段长度
const int MAX_ICMP_PACKET_SIZE = 1024;//ICMP 报文最大长度（包括报头）
const DWORD DEF_ICMP_TIMEOUT = 3000; //回显应答超时时间
const int DEF_MAX_HOP = 30; //最大跳站数

//IP 报头
typedef struct
{
	unsigned char hdr_len : 4; //4 位头部长度
	unsigned char version : 4; //4 位版本号
	unsigned char tos; //8 位服务类型
	unsigned short total_len; //16 位总长度
	unsigned short identifier; //16 位标识符
	unsigned short frag_and_flags; //3 位标志加 13 位片偏移
	unsigned char ttl; //8 位生存时间
	unsigned char protocol; //8 位上层协议号
	unsigned short checksum; //16 位校验和
	unsigned long sourceIP; //32 位源 IP 地址
	unsigned long destIP; //32 位目的 IP 地址
} IP_HEADER;

//ICMP 报头
typedef struct
{
	BYTE type; //8 位类型字段
	BYTE code; //8 位代码字段
	USHORT cksum; //16 位校验和
	USHORT id; //16 位标识符
	USHORT seq; //16 位序列号
} ICMP_HEADER;

//报文解码结构
typedef struct
{
	USHORT usSeqNo; //序列号
	DWORD dwRoundTripTime; //往返时间
	in_addr dwIPaddr; //返回报文的 IP 地址
}DECODE_RESULT;

const static int iTimeout = 3000;

class icmp{

private:
	sockaddr_in destSockAddr;									  // 目的端的socket地址
	SOCKET sockRaw;												  // 原始socket
	char IcmpSendBuf[sizeof(ICMP_HEADER) + DEF_ICMP_DATA_SIZE];   // 发送缓冲区
	char IcmpRecvBuf[MAX_ICMP_PACKET_SIZE];                       // 接收缓冲区
	ICMP_HEADER * pIcmpHeader;									  // IP头
	
	USHORT usSeqNo;												  // ICMP报文序列号
	int iTTL;													  // TTL值

	DECODE_RESULT DecodeResult;                                   // 传给报文的解码参数

	sockaddr_in from;                                             // 对端的socket地址

public:
	icmp();
	~icmp();

	void _send();					     // 发送icmp request echo信息
	int _recv();                         // 接收icmp 信息

	int tracert(char* hostname);

	// 两个数据解析函数
	static USHORT checksum(USHORT *pBuf, int iSize);
	static BOOL DecodeIcmpResponse(char * pBuf, int iPacketSize, DECODE_RESULT &DecodeResult, BYTE ICMP_ECHO_REPLY, BYTE ICMP_TIMEOUT);

};

