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
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include "./icmp.h"
#include "utils.h"
#include <stdio.h>

int main(int argc, char** args) {

	/***********************************
	***********Tracert******************
	************************************
	printf("������һ�� IP ��ַ�������� ");
	char IpAddress[32];
	scanf("%s", IpAddress);
	if ((new icmp())->tracert(IpAddress) != 0) {
		printf("����� IP ��ַ��������Ч!\n\n");
		system("pause");
		exit(0);
	}
	**********************************/
	
	printf("------Number of Threads are %d.", utils::THREAD_NUM);

	// ���򵥲���
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


