/* 程序名称：路由追踪(Tracert)程序
实现原理：Tracert 程序关键是对 IP 头部生存时间(time to live)TTL 字段的使用,程序实现时是向目
地主机发送一个 ICMP 回显请求消息，初始时 TTL 等于 1，这样当该数据报抵达途中的第一个路由器
时，TTL 的值就被减为 0，导致发生超时错误，因此该路由生成一份 ICMP 超时差错报文返回给源主
机。随后，主机将数据报的 TTL 值递增 1，以便 IP 报能传送到下一个路由器，并由下一个路由器生成
ICMP 超时差错报文返回给源主机。不断重复这个过程，直到数据报达到最终的目地主机，此时目地
主机将返回 ICMP 回显应答消息。这样，源主机只需对返回的每一份 ICMP 报文进行解析处理，就可
以掌握数据报从源主机到达目地主机途中所经过的路由信息。
*/

#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include "./icmp.h"
#include "utils.h"
#include <stdio.h>

int main(int argc, char** args) {

	/***********************************
	***********Tracert******************
	************************************
	printf("请输入一个 IP 地址或域名： ");
	char IpAddress[32];
	scanf("%s", IpAddress);
	if ((new icmp())->tracert(IpAddress) != 0) {
		printf("输入的 IP 地址或域名无效!\n\n");
		system("pause");
		exit(0);
	}
	**********************************/
	
	printf("------Number of Threads are %d.", utils::THREAD_NUM);

	// 做简单测试
	char ip[32] = "10.1.11.206";
	(new icmp())->pings(ip);
	
	/*
	if (argc != 2 || (new icmp())->pings(args[1]) != 0 ) {   //       pings  <IP>
		printf("Usage: \n");
		printf("  pings <IP>                     scan the LAN's hosts from 0~255.\n\n");
	}*/

	printf("\n\n");

	system("pause");
	
	exit(1);

	return 0;
}


