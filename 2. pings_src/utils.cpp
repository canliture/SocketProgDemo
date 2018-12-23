#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include "./utils.h"
#include <string>
#include <vector>

utils::utils()
{
}


// 将整数的IP转化为字符串形式的IP十进制地址
char* utils::longIp2StrIp(u_long ulIp) {

	char ipAddr[32];
	in_addr tmpAddr;
	tmpAddr.S_un.S_addr = ulIp;
	strcpy(ipAddr, inet_ntoa(tmpAddr));

	return ipAddr;
}

std::string utils::getLANIpPrefix(const char* ip) {
	std::vector<int> ipNum;
	int num = 0;
	for (int i = 0; i < strlen(ip); i++) {
		if (ip[i] == '.' || i + 1 == strlen(ip)) {
			ipNum.push_back((i + 1 == strlen(ip)) ? num * 10 + (ip[i] - '0') : num);
			num = 0;
			continue;
		}
		num = num * 10 + (ip[i] - '0');
	}
	std::string prefix = "";
	for (int i = 0; i < ipNum.size() - 1; i++) {
		char numBuf[16];
		_itoa(ipNum[i], numBuf, 10);
		prefix = prefix + (prefix == "" ? "" : ".") + numBuf;
	}
	return prefix+".";
}


std::vector<int> utils::get4IPNum(const char* ip) {  // 192.168.254.226
	std::vector<int> v;
	int num = 0;
	for (int i = 0; i < strlen(ip); i++) {
		if (v[i] != '.') {
			num = num * 10 + (v[i] - '0');
		} else {
			v.push_back(num);
			num = 0;
		}
	}
	v.push_back(num);

	return v;
}

bool utils::equal(std::vector<int> v1, std::vector<int> v2) {

	for (int i = 0; i < 4; i++) {
		if (v1[i] != v2[i]) {
			return false;
			break;
		}
	}
	return true;
}

int utils::add(std::vector<int>& IP4Num) {         // 192.168.254.226   
												   // 192.168.254.255
	int i = 3;
	bool carry = true;
	while (i >= 0 && carry) {
		int judge = IP4Num[i] + 1;
		if (judge > 255) {
			IP4Num[i] = 0;
		} else {
			IP4Num[i] = judge;
			carry = false;
		}
		i++;
	}
	
	if (carry && (i == -1)) {
		return 1;
	}

	return 0;
}

utils::~utils()
{
}
