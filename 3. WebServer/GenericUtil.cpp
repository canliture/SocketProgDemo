#include "GenericUtil.h"
#include "MsgUtil.h"
#include <WinSock2.h>


GenericUtil::GenericUtil(){}

GenericUtil::~GenericUtil(){}

bool GenericUtil::initWinSock() {

	WORD wVersionRequested = WINSOCK_VERSION;
	WSADATA wsaData;

	int nRet = WSAStartup(wVersionRequested, &wsaData);			 // ����Winsock, ������سɹ��򷵻�0
	if (nRet) {
		MsgUtil::declare("Initialize WinSock Failed", MSG_ERROR);
		return false;
	}

	if (wsaData.wVersion != wVersionRequested) {				 // ���汾
		MsgUtil::declare("Wrong WinSock Version", MSG_ERROR);
		return false;
	}
	return true;
}