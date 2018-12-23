#pragma once

#define MSG_LOG 0
#define MSG_ERROR 1
#define MSG_DEFAULT 2

class MsgUtil
{
public:
	MsgUtil();
	~MsgUtil();

	static void declare(const char* msg, int type);

};

