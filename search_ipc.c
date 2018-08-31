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
#include "search_dvr.h"
#include "cross_sock.h"

#define HICHIP_VERSION_CONTENT "{\"Ver\":\"1.1\"}"
static int m_hichip_session_id = 0;

/*
	checksum is : md5("++session++session_id++content++content++")
*/
static int hichip_cal_checksum(char *ret_checksum, int ret_size, char *session_id, char *content, int content_len)
{
	char sumbuffer[8 * 1024];
	int buffer_len = 0;
	if(!session_id || !ret_checksum){
		return -1;
	}
	memset(sumbuffer, 0, sizeof(sumbuffer));
	snprintf(sumbuffer, sizeof(sumbuffer), "++session++%s++content++", session_id);
	buffer_len = strlen(sumbuffer);
	if(content_len && content_len > 0){
		if(content_len + buffer_len  + strlen("++") <= sizeof(sumbuffer)){
			memcpy(sumbuffer + buffer_len, content, content_len);
			buffer_len += content_len;
		}
		else{
			printf("hichip_cal_checksum buffer not enough !\n");
			return -1;
		}
	}
	memcpy(sumbuffer + buffer_len, "++", strlen("++"));
	buffer_len += strlen("++");

	snprintf(ret_checksum, ret_size, "%s", MD5Sum_buffer(sumbuffer, buffer_len));

	return 0;
}

void HICHIP_generate_device_id(char *devid)
{
#define HICHIP_WITH_JUAN_SPEC "JUANLaw"
	int i = 0;
	struct timeval tv;
	char * pdevname = NULL;

	pdevname = getenv("DEVNAME");
	if (pdevname == NULL)
		pdevname = HICHIP_WITH_JUAN_SPEC;
	
	// random seeds
	gettimeofday(&tv, NULL);
	srand(tv.tv_sec ^tv.tv_usec);
	for(i = 0; i < 32; ++i){
		devid[i] = rand() % 26 + ((0 == rand() % 2) ? 'A' : 'a');
	}
	memcpy(devid, pdevname, strlen(pdevname));
}


int HICHIP_generate_search_pack(char *search_type, char *client_id_str, char *device_id, char *sendbuf, int size)
{
	char checksum[128], seeion_id_text[128];
	
	m_hichip_session_id ++;
	snprintf(seeion_id_text, sizeof(seeion_id_text), "%d", m_hichip_session_id);

	hichip_cal_checksum(checksum, sizeof(checksum), seeion_id_text, HICHIP_VERSION_CONTENT, strlen(HICHIP_VERSION_CONTENT));

	memset(sendbuf, 0, size);
	snprintf(sendbuf, size, "SEARCH * HDS/1.1\r\n"
			"CSeq:1\r\n"
			"Client-ID:%s\r\n"
			"Accept-Type:text/HDP\r\n"
			"Content-Length:13\r\n"
			"X-Search-Type:%s\r\n"
			"X-Content-Checksum:%s\r\n"
			"X-Session-Id:%s\r\n", client_id_str, search_type ? search_type : "NVR", checksum, seeion_id_text);


	if(device_id && strlen(device_id)){
		strncat(sendbuf, "Device-ID:", size - strlen(sendbuf));
		strncat(sendbuf, device_id, size - strlen(sendbuf));
		strncat(sendbuf, "\r\n", size - strlen(sendbuf));
	}

	strncat(sendbuf, "\r\n", size - strlen(sendbuf));
	strncat(sendbuf, HICHIP_VERSION_CONTENT, size - strlen(sendbuf));
	strncat(sendbuf, "\r\n", size - strlen(sendbuf));

	return 0;
}


int SearchIpcam(char *bindIP)
{
	struct sockaddr_in _sockaddr_in;
	struct in_addr _in_addr;
	int sock_broadcast = -1;
	int ret = -1;
	fd_set read_set;
	struct timeval tv = {2,0};
	time_t last_time = 0;
	char srbuf[1024]={0};
	char ip[64] = {0};
	int port = 0;
	char *ptr = NULL;
	char *ptrd = NULL;
	char id[32] = {0};
	int chn = 0;
	char model[12];
	char version[12];
	int ii = 0;
	char client_id_str[128];

	last_time = time(NULL);
	sock_broadcast = SOCK_broadcast_udp_init(NULL, 0, 0, 5);
	if(sock_broadcast < 0){
		printf("SOCK_broadcast_udp_init failed!@%s\n",strerror(errno));
		return -1;
	}

	HICHIP_generate_device_id(client_id_str);
	HICHIP_generate_search_pack("NVR", client_id_str, NULL, srbuf, sizeof(srbuf));
	SOCK_sendto(sock_broadcast, HICHIP_MULTICAST_IPADDR, HICHIP_MULTICAST_PORT, srbuf, sizeof(srbuf), 0);

	while(1){
		
		FD_ZERO(&read_set);
		FD_SET(sock_broadcast,&read_set);

		ret = select(sock_broadcast + 1 ,&read_set,NULL,NULL,&tv);
		if(ret > 0 && FD_ISSET(sock_broadcast,&read_set)){

			memset(srbuf,0,sizeof(srbuf));
			ret = SOCK_recvfrom(sock_broadcast, ip, &port, srbuf, sizeof(srbuf), 0);
			if(ret > 0){

				if (strstr(srbuf, "HDS")) {			
					//if(hichip_parse_search_ack(szip, recvbuf, bind_host, hook, customCtx, device_type) == 0){
					//	count++;
					//}
					printf("%s\n\n",srbuf);
				}

				//ptr = strstr(srbuf,BUBBLE_SEARCH_ACK_STRING);

				//printf("got[%3d] one %2d'chns %8s , ip : %16s %5d ,id :%20s, version: %3s \n",ii,chn,model,ip,port,id,version);
				//ii ++;
			}
			
		}
		if(time(NULL) - last_time > 5){
			break;
		}
	}
	return 0;
}

