#include <WinSock2.h>
#include <vector>
#include <map>
#include <set>

#pragma comment(lib, "Ws2_32.lib")

#define DEFAULT_PORT 9999

typedef struct socketClientStru {
	SOCKET sockConnect;           // 客户端的socket连接
	SOCKADDR_IN sockAddr;         // 客户端的socket地址
	int uuid;                     // 客户端的id
}CLIENT;

// 发送类型
#define SEND_TYPE_BROADCAST 0
#define SEND_TYPE_SINGLE    1
#define SEND_TYPE_SYSTEM    2
#define SEND_TYPE_DIRECT    3
#define SEND_TYPE_GROUP     4

class Server
{
public:
	Server();
	Server(int port);

	CLIENT acceptClient();
	void recvFrom(CLIENT client);
	friend DWORD WINAPI recvThread(LPVOID param);

	// 服务器转发信息
	int sendToAll(CLIENT client, const char* contents, int type);			      // cmd_to_grp <N> <Contents> | <Contents>  发送"广播"信息
 	int sendToUsr(CLIENT client, int uuid, const char* contents, int type);     // cmd_to_usr <N> <Contents>     发送给特定的用户
	int sendToGrp(CLIENT client, int grpId, const char* constents);   // cmd_to_grp <N> <Contents>

	// 客户端查询信息  ： 查询好友，群信息
	int listUsers(CLIENT client);                             // cmd_ls_usr
	int listGrps(CLIENT client);                              // cmd_ls_grp

	// 客户端修改信息  ： 添加/删除好友；创建/加入/退出群
	//int followUsr(CLIENT client);                             // cmd_fw_frd <N>   添加好友
	//int deleteUsr(CLIENT client);                             // cmd_dl_frd <N>   删除好友
	int setUpGrp(CLIENT client);                              // cmd_su_grp <N>   建群
	int inGrp(CLIENT client, int grpId);                      // cmd_in_grp <N>   加入群
	int outGrp(CLIENT client, int grpId);                     // cmd_ot_grp <N>   退出群

	void close();
	~Server();
private:
	int initWinSock();
	int findClientIndex(int id);                              // 通过id来查找某个客户端

private:
	
	SOCKET sockServer;
	SOCKADDR_IN addrSrv;

	int uuId;                                           // 客户端id
	int grpId;										    // 群id

	std::vector<CLIENT> onlineClient;					// 在线的客户端
	std::map<int, std::vector<int> > grpId2UsrIdMap;    // 群id映射->客户端id   [一个群有哪些人]
	std::map<int, std::vector<int> > usrId2GrpIdMap;    // 客户端id->群id	   [一个人加了哪些群]
	//std::map<int, std::vector<int> > usrId2usrIdmap;    // 客户端id->客户端id   [一个人有哪些好友]
	
};

