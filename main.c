#include <stdio.h>
#include <stdlib.h>

#define NETCARD "eth0"
#define IP "192.168.13.123"
int main(char argc , char**args)
{
#if 1
	char ip[16];
	get_netcard_ip(NETCARD,ip);
	get_netcard_mtu(NETCARD);
	//SearchDVR(ip);
	//SearchIpcam(ip);
	ping(IP,5,0);
#else
	//SearchDVR(NULL);
	Modify_IPCAM_IP();
#endif
}

