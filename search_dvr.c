#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

/*
	获取指定网口的本地IP地址，字符串形式返回，如：192.168.1.111
	获取成功返回1，失败返回－1；
*/
int get_netcard_ip(char *netcard , char *ip)
{

	struct ifreq c_ifreq;
	int sock = -1;
	int ret = -1;
	struct in_addr _addr;
	struct sockaddr_in *_sockaddr_in;
	
	memset(&c_ifreq,0,sizeof(struct ifreq));
	sock = socket(AF_INET,SOCK_DGRAM , 0);
	if(sock < 0){
		printf("socket failed!@%s\n",strerror(errno));
		return -1;
	}
	
	if(NULL == netcard){
		printf("netcard NULL\n");
		return -1;
	}
	
	strcpy(c_ifreq.ifr_name,netcard);
	ret = ioctl(sock,SIOCGIFADDR,&c_ifreq);
	_sockaddr_in = (struct sockaddr_in*)&c_ifreq.ifr_addr;
	_addr.s_addr = _sockaddr_in->sin_addr.s_addr;
	printf("%s ip is: %s \n",netcard ,inet_ntoa(_addr));
	
	return 0;
	
}

int get_netcard_mtu(char *netcard)
{

	struct ifreq c_ifreq;
	int sock = -1;
	int ret = -1;

	memset(&c_ifreq,0,sizeof(struct ifreq));
	sock = socket(AF_INET,SOCK_DGRAM , 0);
	if(sock < 0){
		printf("socket failed!@%s\n",strerror(errno));
		return -1;
	}
	
	if(NULL == netcard){
		printf("netcard NULL\n");
		return -1;
	}
	
	strcpy(c_ifreq.ifr_name,netcard);
	ret = ioctl(sock,SIOCGIFMTU,&c_ifreq);
	printf("%s ip is: %d\n",netcard,c_ifreq.ifr_mtu);
	
	return 0;
	
}

#define BUBBLE_PORT 9013
#define BUBBLE_ADDR "255.255.255.255"
#define BUBBLE_SEARCH_STRING "SEARCHDEV"
#define BUBBLE_SEARCH_ACK_STRING "JA"

int SearchDVR(char *bindIP)
{
	struct sockaddr_in _sockaddr_in;
	struct in_addr _in_addr;
	int sock_broadcast = -1;
	int ret = -1;
	fd_set read_set;
	struct timeval tv = {2,0};
	time_t last_time = 0;
	char srbuf[128]={0};
	char ip[64] = {0};
	int port = 0;
	char *ptr = NULL;
	char *ptrd = NULL;
	char id[32] = {0};
	int chn = 0;
	char model[12];
	char version[12];
	int ii = 0;

	last_time = time(NULL);
	sock_broadcast = SOCK_broadcast_udp_init(NULL, 0, 0, 5);
	if(sock_broadcast < 0){
		printf("SOCK_broadcast_udp_init failed!@%s\n",strerror(errno));
		return -1;
	}

	snprintf(srbuf,sizeof(srbuf),"%s",BUBBLE_SEARCH_STRING);
	SOCK_sendto(sock_broadcast, BUBBLE_ADDR, BUBBLE_PORT, srbuf, sizeof(srbuf), 0);
	while(1){
		FD_ZERO(&read_set);
		FD_SET(sock_broadcast,&read_set);

		ret = select(sock_broadcast + 1 ,&read_set,NULL,NULL,&tv);
		if(ret > 0 && FD_ISSET(sock_broadcast,&read_set)){

			memset(srbuf,0,sizeof(srbuf));
			ret = SOCK_recvfrom(sock_broadcast, ip, &port, srbuf, sizeof(srbuf), 0);
			if(ret > 0){

				//JAIP%s&ID%s&PORT%d&HTTP%d&CH%d&MODEL%s&PVER%d
				//JAIP192.168.13.124&ID1731283323&PORT80&HTTP80&CH25&MODELNVR&PVER101
#if 0
				printf("%d => %s\n",ii,srbuf);
#endif				
				ptr = strstr(srbuf,BUBBLE_SEARCH_ACK_STRING);

				if(ptr){
					memset(ip,0,sizeof(ip));
					ptr += 4;
					ptrd = strstr(ptr,"&");
					if(ptrd){
						strncpy(ip,ptr,ptrd - ptr);//ip
					}

					ptr = strstr(ptr,"ID");
					if(ptr){
						
						memset(id,0,sizeof(id));
						ptr += 2;
						ptrd = strstr(ptr,"&");
						if(ptrd){
							strncpy(id,ptr,ptrd - ptr);//id
						}
					}

					port = 80;//port

					ptr = strstr(ptr,"CH");
					if(ptr){
						
						chn = 0;
						ptr += 2;
						ptrd = strstr(ptr,"&");
						char tmp[4];
						if(ptrd){
							memset(tmp,0,sizeof(tmp));
							strncpy(tmp,ptr,ptrd -ptr);//chn
							chn = atoi(tmp);
						}
					}

					ptr = strstr(ptr ,"MODEL");
					if(ptr){
						
						memset(model,0,sizeof(model));
						ptr += 5;
						ptrd = strstr(ptr,"&");
						if(ptrd){
							strncpy(model,ptr,ptrd-ptr);//model
						}else{
							strcpy(model,ptr);
						}
					}

					ptr = strstr(ptr,"VER");
					if(ptr){

						memset(version,0,sizeof(version));
						ptr += 3;
						if(ptr){
							strcpy(version,ptr);
						}
					}
					
				}
				printf("got[%3d] one %2d'chns %8s , ip : %16s %5d ,id :%20s, version: %3s \n",ii,chn,model,ip,port,id,version);
				ii ++;
			}
			
		}
		if(time(NULL) - last_time > 5){
			break;
		}
	}
}

#define NETCARD "eth0"
int main(char argc , char**args)
{
#if 0
	char ip[16];
	get_netcard_ip(NETCARD,ip);
	get_netcard_mtu(NETCARD);
	SearchDVR(ip);
#else
	SearchDVR(NULL);
#endif
}
