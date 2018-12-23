#include <ws2tcpip.h>
#include <string>
#include <vector>

class utils
{
public:
	utils();
	
	const static int THREAD_NUM = 20;
	const static int GAP_THREAD_PER = 13;
	
	static char* longIp2StrIp(u_long ip);                  // 将u_long 型Ip地址转化为string类型的Ip
	static std::string getLANIpPrefix(const char* ip);     // 获取局域网前缀：比如192.168.1.48 -> 192.168.1.
	 
	static std::vector<int> get4IPNum(const char* ip);

	static bool equal(std::vector<int> start4IPNum, std::vector<int> end4IPNum);

	static int add(std::vector<int>& start4IPNum);
	
	~utils();
};

