#include <ws2tcpip.h>

//ICMP �����ֶ�
const BYTE ICMP_ECHO_REQUEST = 8; //�������
const BYTE ICMP_ECHO_REPLY = 0; //����Ӧ��
const BYTE ICMP_TIMEOUT = 11; //���䳬ʱ

//������������
const int DEF_ICMP_DATA_SIZE = 32; //ICMP ����Ĭ�������ֶγ���
const int MAX_ICMP_PACKET_SIZE = 1024;//ICMP ������󳤶ȣ�������ͷ��
const DWORD DEF_ICMP_TIMEOUT = 3000; //����Ӧ��ʱʱ��
const int DEF_MAX_HOP = 30; //�����վ��

//IP ��ͷ
typedef struct
{
	unsigned char hdr_len : 4; //4 λͷ������
	unsigned char version : 4; //4 λ�汾��
	unsigned char tos; //8 λ��������
	unsigned short total_len; //16 λ�ܳ���
	unsigned short identifier; //16 λ��ʶ��
	unsigned short frag_and_flags; //3 λ��־�� 13 λƬƫ��
	unsigned char ttl; //8 λ����ʱ��
	unsigned char protocol; //8 λ�ϲ�Э���
	unsigned short checksum; //16 λУ���
	unsigned long sourceIP; //32 λԴ IP ��ַ
	unsigned long destIP; //32 λĿ�� IP ��ַ
} IP_HEADER;

//ICMP ��ͷ
typedef struct
{
	BYTE type; //8 λ�����ֶ�
	BYTE code; //8 λ�����ֶ�
	USHORT cksum; //16 λУ���
	USHORT id; //16 λ��ʶ��
	USHORT seq; //16 λ���к�
} ICMP_HEADER;

//���Ľ���ṹ
typedef struct
{
	USHORT usSeqNo; //���к�
	DWORD dwRoundTripTime; //����ʱ��
	in_addr dwIPaddr; //���ر��ĵ� IP ��ַ
}DECODE_RESULT;

const static int iTimeout = 3000;

class icmp{

private:
	sockaddr_in destSockAddr;									  // Ŀ�Ķ˵�socket��ַ
	SOCKET sockRaw;												  // ԭʼsocket
	char IcmpSendBuf[sizeof(ICMP_HEADER) + DEF_ICMP_DATA_SIZE];   // ���ͻ�����
	char IcmpRecvBuf[MAX_ICMP_PACKET_SIZE];                       // ���ջ�����
	ICMP_HEADER * pIcmpHeader;									  // IPͷ
	
	USHORT usSeqNo;												  // ICMP�������к�
	int iTTL;													  // TTLֵ

	DECODE_RESULT DecodeResult;                                   // �������ĵĽ������

	sockaddr_in from;                                             // �Զ˵�socket��ַ

public:
	icmp();
	~icmp();

	void _send();					     // ����icmp request echo��Ϣ
	int _recv();                         // ����icmp ��Ϣ

	int tracert(char* hostname);

	// �������ݽ�������
	static USHORT checksum(USHORT *pBuf, int iSize);
	static BOOL DecodeIcmpResponse(char * pBuf, int iPacketSize, DECODE_RESULT &DecodeResult, BYTE ICMP_ECHO_REPLY, BYTE ICMP_TIMEOUT);

};

