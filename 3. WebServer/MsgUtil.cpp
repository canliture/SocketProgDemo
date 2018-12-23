#include "MsgUtil.h"
#include <stdio.h>


MsgUtil::MsgUtil(){}
MsgUtil::~MsgUtil(){}

void MsgUtil::declare(const char* msg, int type) {

	const char* typeMsg = NULL;
	switch (type) {
	case MSG_LOG:
		typeMsg = "Log msg"; break;
	case MSG_ERROR:
		typeMsg = "Error msg"; break;
	default:
		typeMsg = "General msg"; break;
	}
	printf("%s: %s\n", typeMsg, (char*)msg);
}
