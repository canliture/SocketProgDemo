#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include "./utils.h"

utils::utils()
{
}


// ��������IPת��Ϊ�ַ�����ʽ��IPʮ���Ƶ�ַ
char* utils::longIp2StrIp(u_long ulIp) {

	char ipAddr[32];
	in_addr tmpAddr;
	tmpAddr.S_un.S_addr = ulIp;
	strcpy(ipAddr, inet_ntoa(tmpAddr));

	return ipAddr;
}


utils::~utils()
{
}
