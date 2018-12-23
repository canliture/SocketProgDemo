/* �������ƣ�·��׷��(Tracert)����
ʵ��ԭ��Tracert ����ؼ��Ƕ� IP ͷ������ʱ��(time to live)TTL �ֶε�ʹ��,����ʵ��ʱ����Ŀ
����������һ�� ICMP ����������Ϣ����ʼʱ TTL ���� 1�������������ݱ��ִ�;�еĵ�һ��·����
ʱ��TTL ��ֵ�ͱ���Ϊ 0�����·�����ʱ������˸�·������һ�� ICMP ��ʱ����ķ��ظ�Դ��
����������������ݱ��� TTL ֵ���� 1���Ա� IP ���ܴ��͵���һ��·������������һ��·��������
ICMP ��ʱ����ķ��ظ�Դ�����������ظ�������̣�ֱ�����ݱ��ﵽ���յ�Ŀ����������ʱĿ��
���������� ICMP ����Ӧ����Ϣ��������Դ����ֻ��Է��ص�ÿһ�� ICMP ���Ľ��н��������Ϳ�
���������ݱ���Դ��������Ŀ������;����������·����Ϣ��
*/

#define _CRT_SECURE_NO_WARNINGS

#include "./icmp.h"
#include <stdio.h>

int main() {

	printf("������һ�� IP ��ַ�������� ");
	char IpAddress[32];
	scanf("%s", IpAddress);
	
	if ((new icmp())->tracert(IpAddress) != 0) {
		printf("����� IP ��ַ��������Ч!\n\n");
		system("pause");
		exit(0);
	}

	return 0;
}

/*
int main()
{
	//��ʼ�� Windows sockets ���绷��
	WSADATA wsa;
	WSAStartup(MAKEWORD(2, 2), &wsa);



	// char IpAddress[255];
	char inIpAddress[255];   // �����IPAddress
	char IpAddress[255];     // ʵ�ʵ�IP
	cout << "������һ�� IP ��ַ��������";
	cin >> IpAddress;
	strcpy(inIpAddress, IpAddress);
	//�õ� IP ��ַ
	u_long ulDestIP = inet_addr(inIpAddress);
	//ת�����ɹ�ʱ����������
	if (ulDestIP == INADDR_NONE)
	{
		hostent * pHostent = gethostbyname(IpAddress);
		if (pHostent)
		{
			ulDestIP = (*(in_addr*)pHostent->h_addr).s_addr;
			// ��������IPת��Ϊ�ַ�����ʽ��IPʮ���Ƶ�ַ
			in_addr tmpAddr;
			tmpAddr.S_un.S_addr = ulDestIP;
			strcpy(IpAddress, inet_ntoa(tmpAddr));
		}
		else
		{
			cout << "����� IP ��ַ��������Ч!" << endl;
			WSACleanup();
			return 1;
		}
	}

	cout << "Tracing route to " << inIpAddress;
	if (strcmp(inIpAddress, IpAddress) != 0) {
		cout << " [ " << IpAddress << " ]";
	}
	cout << " with a maximum of 30 hops.\n" << endl;

	// cout<<"Tracing route to "<<IpAddress<<" with a maximum of 30 hops.\n"<<endl;
	//���Ŀ�ض� socket ��ַ
	sockaddr_in destSockAddr;
	ZeroMemory(&destSockAddr, sizeof(sockaddr_in));
	destSockAddr.sin_family = AF_INET;
	destSockAddr.sin_addr.s_addr = ulDestIP;
	//����ԭʼ�׽���
	SOCKET sockRaw = WSASocket(AF_INET, SOCK_RAW, IPPROTO_ICMP, NULL, 0,
		WSA_FLAG_OVERLAPPED);
	//��ʱʱ��
	int iTimeout = 3000;
	//���ճ�ʱ
	setsockopt(sockRaw, SOL_SOCKET, SO_RCVTIMEO, (char *)&iTimeout, sizeof(iTimeout));
	//���ͳ�ʱ
	setsockopt(sockRaw, SOL_SOCKET, SO_SNDTIMEO, (char *)&iTimeout, sizeof(iTimeout));
	//���� ICMP ����������Ϣ������ TTL ������˳���ͱ���
	//ICMP �����ֶ�
	const BYTE ICMP_ECHO_REQUEST = 8; //�������
	const BYTE ICMP_ECHO_REPLY = 0; //����Ӧ��
	const BYTE ICMP_TIMEOUT = 11; //���䳬ʱ
								  //������������
	const int DEF_ICMP_DATA_SIZE = 32; //ICMP ����Ĭ�������ֶγ���
	const int MAX_ICMP_PACKET_SIZE = 1024;//ICMP ������󳤶ȣ�������ͷ��
	const DWORD DEF_ICMP_TIMEOUT = 3000; //����Ӧ��ʱʱ��
	const int DEF_MAX_HOP = 30; //�����վ��
								//��� ICMP ������ÿ�η���ʱ������ֶ�
	char IcmpSendBuf[sizeof(ICMP_HEADER) + DEF_ICMP_DATA_SIZE];//���ͻ�����
	memset(IcmpSendBuf, 0, sizeof(IcmpSendBuf)); //��ʼ�����ͻ�����
	char IcmpRecvBuf[MAX_ICMP_PACKET_SIZE]; //���ջ�����
	memset(IcmpRecvBuf, 0, sizeof(IcmpRecvBuf)); //��ʼ�����ջ�����
	ICMP_HEADER * pIcmpHeader = (ICMP_HEADER*)IcmpSendBuf;
	pIcmpHeader->type = ICMP_ECHO_REQUEST; //����Ϊ�������
	pIcmpHeader->code = 0; //�����ֶ�Ϊ 0
	pIcmpHeader->id = (USHORT)GetCurrentProcessId(); //ID �ֶ�Ϊ��ǰ���̺�
	memset(IcmpSendBuf + sizeof(ICMP_HEADER), 'E', DEF_ICMP_DATA_SIZE);//�����ֶ�
	USHORT usSeqNo = 0; //ICMP �������к�
	int iTTL = 1; //TTL ��ʼֵΪ 1
	BOOL bReachDestHost = FALSE; //ѭ���˳���־
	int iMaxHot = DEF_MAX_HOP; //ѭ����������
	DECODE_RESULT DecodeResult; //���ݸ����Ľ��뺯���Ľṹ������
	while (!bReachDestHost&&iMaxHot--)
	{
		//���� IP ��ͷ�� TTL �ֶ�
		setsockopt(sockRaw, IPPROTO_IP, IP_TTL, (char *)&iTTL, sizeof(iTTL));
		cout << iTTL << flush; //�����ǰ���
							   //��� ICMP ������ÿ�η��ͱ仯���ֶ�
		((ICMP_HEADER *)IcmpSendBuf)->cksum = 0; //У�������Ϊ 0
		((ICMP_HEADER *)IcmpSendBuf)->seq = htons(usSeqNo++); //������к�
		((ICMP_HEADER *)IcmpSendBuf)->cksum = checksum((USHORT *)IcmpSendBuf, sizeof(ICMP_HEADER) + DEF_ICMP_DATA_SIZE); //����У���
																														 //��¼���кź͵�ǰʱ��
		DecodeResult.usSeqNo = ((ICMP_HEADER*)IcmpSendBuf)->seq; //��ǰ���
		DecodeResult.dwRoundTripTime = GetTickCount(); //��ǰʱ��
													   //���� TCP ����������Ϣ
		sendto(sockRaw, IcmpSendBuf, sizeof(IcmpSendBuf), 0, (sockaddr*)&destSockAddr, sizeof(destSockAddr));
		//���� ICMP ����Ĳ����н�������
		sockaddr_in from; //�Զ� socket ��ַ
		int iFromLen = sizeof(from); //��ַ�ṹ��С
		int iReadDataLen; //�������ݳ���
		while (1)
		{
			//��������
			iReadDataLen = recvfrom(sockRaw, IcmpRecvBuf, MAX_ICMP_PACKET_SIZE, 0, (sockaddr*)&from, &iFromLen);
			if (iReadDataLen != SOCKET_ERROR)//�����ݵ���
			{
				//�����ݰ����н���
				if (DecodeIcmpResponse(IcmpRecvBuf, iReadDataLen, DecodeResult, ICMP_ECHO_REPLY, ICMP_TIMEOUT))
				{
					//��� IP ��ַ
					cout << '\t' << inet_ntoa(DecodeResult.dwIPaddr) << endl;

					//����Ŀ�ĵأ��˳�ѭ��
					if (DecodeResult.dwIPaddr.s_addr == destSockAddr.sin_addr.s_addr) {
						bReachDestHost = true;
						cout << "\n" << "Complete tracert.\n" << endl;
					}
					break;
				}
			}
			else if (WSAGetLastError() == WSAETIMEDOUT) //���ճ�ʱ�����*��
			{
				cout << " *" << '\t' << "Request timed out." << endl;
				
				break;
			}//
			else
			{
				break;
			}//
		}
		iTTL++; //���� TTL ֵ
	}

	system("pause");

	return 0;
}

*/

/*
int main(int argc, char ** args)
{
	//��ʼ�� Windows sockets ���绷��
	WSADATA wsa;
	WSAStartup(MAKEWORD(2, 2), &wsa);

	if (argc != 2 || inet_addr(args[1]) == INADDR_NONE) {   //       pings  <IP>
		cout << "Usage: " << endl;
		cout << "  pings <IP>                     scan the LAN's hosts from 0~255.\n\n" << endl;
		system("pause");
		exit(1);
	}

	arg = args[1];

	//threadNum  index*13+1  13*(index+1)
	// 0          [1     ,		  13]
	// 1          [14    ,        26]
	// 2          [27    ,        39]
	for (int i = 0; i < THREAD_NUM; i++) {
		int k = i;
		CreateThread(NULL, 0, handlerThread, &k, 0, NULL);
	}

	getchar();

	system("pause");

	return 0;
}

string getLANIPPrefix(const char* ip) {
	vector<int> ipNum;
	int num = 0;
	for (int i = 0; i < strlen(ip); i++) {
		if (ip[i] == '.' || i + 1 == strlen(ip)) {
			ipNum.push_back((i + 1 == strlen(ip)) ? num * 10 + (ip[i] - '0') : num);
			num = 0;
			continue;
		}
		num = num * 10 + (ip[i] - '0');
	}
	string prefix = "";
	for (int i = 0; i < ipNum.size() - 1; i++) {
		char numBuf[16];
		_itoa(ipNum[i], numBuf, 10);
		prefix = prefix + (prefix == "" ? "" : ".") + numBuf;
	}
	return prefix;
}
*/