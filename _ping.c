#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <regex.h>
#include <stdarg.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>

#include "_ping.h"



#define TRACE_DEBUG(fmt, args...) printf("\033[0;32;32m""%s,%s,%d @ " fmt "\n""\033[m",__FILE__, __FUNCTION__, __LINE__,##args)
#define TRACE_ERROR(fmt, args...) printf("\033[0;32;31m""%s,%s,%d @ " fmt "\n""\033[m",__FILE__, __FUNCTION__, __LINE__,##args)
int IsStringIp(char *str,struct in_addr *s_addr_in)
{

	if(NULL == str){
		TRACE_ERROR("%s is NULL",str);
		return -1;

	}
	return inet_aton(str,s_addr_in);

}
int StringIpToArrayIP(char *ip_str,unsigned char ip[])
{

	int cnt = 0;
	int a[4] = {-1 ,  -1 , -1 , -1};

	
	if(NULL == ip_str){
		TRACE_ERROR("%s not a ipv4's ip",ip_str);
		return -1;
	}
#if 0
	char *ptr = NULL;
	
	if(strlen(ip_str) < 7 || strlen(ip_str) > 15){//not the ipv4 length
		TRACE_ERROR("%s not a ipv4's ip",ip_str);
		return -1;
	}	
	ptr = ip_str;
	while(ptr != NULL && *ptr != '\0'){
		*ptr == '.' ? cnt ++ : cnt;
		ptr ++;
	}
	if(cnt != 3){
		TRACE_ERROR("%s not a ipv4's ip",ip_str);
		return -1;
	}
#else
	if(!IsStringIp){
		TRACE_ERROR("%s not a ipv4's ip",ip_str);
		return -1;		
	}
#endif
	if(sscanf(ip_str,"%d.%d.%d.%d",&a[0],&a[1],&a[2],&a[3]) != 4){
		TRACE_ERROR("%s not a ipv4's ip",ip_str);
		return -1;
	}
	for(cnt = 0 ; cnt < 4 ;cnt ++){
		ip[cnt] = a[cnt];  
	}
#if 0	
	if(!((ip[0] <= 255 && ip[0] >= 0)&&(ip[1] <= 255 && ip[1] >= 0)&&(ip[2] <= 255 && ip[2] >= 0)&&(ip[3] <= 255 && ip[3] >= 0))){
		TRACE_ERROR("%s not a ipv4's ip",ip_str);
		return -1;		
	}
#endif

	return 0;
	
}

double GetRtt(struct timeval *RecvTime, struct timeval *SendTime)
{
	struct timeval sub = *RecvTime;
 
	if ((sub.tv_usec -= SendTime->tv_usec) < 0)
	{
		--(sub.tv_sec);
		sub.tv_usec += 1000000;
	}
	sub.tv_sec -= SendTime->tv_sec;
	
	return sub.tv_sec * 1000.0 + sub.tv_usec / 1000.0; //转换单位为毫秒
}

static unsigned short cal_chksum(unsigned short *addr, int len)
{
	int nleft=len;
	int sum=0;
	unsigned short *w=addr;
	unsigned short answer=0;
	while(nleft>1)
	{
		sum+=*w++;
		nleft-=2;
	}
	if( nleft==1)
	{
		*(unsigned char *)(&answer)=*(unsigned char *)w;
		sum+=answer;
	}
	sum=(sum>>16)+(sum&0xffff);
	sum+=(sum>>16);
	answer=~sum;
	return answer;
}

int ICMP_pack(char* _buf, int packsize, int pack_num)
{
	struct icmp *icmp;
	struct timeval *tval;
	icmp=(struct icmp*)_buf;
	icmp->icmp_type = ICMP_ECHO;
	icmp->icmp_code = 0;
	icmp->icmp_cksum = 0;
	icmp->icmp_seq = pack_num;
	icmp->icmp_id = getpid();
	tval= (struct timeval *)icmp->icmp_data;
	gettimeofday(tval,NULL);
	icmp->icmp_cksum=cal_chksum( (unsigned short *)icmp, packsize);
	return packsize;
}

int ICMP_unpack(char *buf, int len, struct timeval *RecvTime)
{
	int iphdrlen;
	struct ip *ip;
	struct icmp *icmp;
	struct timeval *SendTime;
	if(len <= 0){
		return -1;
	}

	ip = (struct ip *)buf;
	iphdrlen = ip->ip_hl << 2;				/*求ip报头长度,即ip报头的长度标志乘4*/
	icmp = (struct icmp *)(buf + iphdrlen);	/*越过ip报头,指向ICMP报头*/
	len -= iphdrlen;						/*ICMP报头及ICMP数据报的总长度*/

	if(len < 8) {							/*小于ICMP报头长度则不合理*/
		printf("ICMP packets\'s length is less than 8\n");
		return -1;
	}

	/*确保所接收的是我所发的的ICMP的回应*/
	if((icmp->icmp_type == ICMP_ECHOREPLY) && (icmp->icmp_id == getpid())) {
		SendTime = (struct timeval *)icmp->icmp_data;
		printf("%u bytes from %s: icmp_seq=%u ttl=%u time=%.1f ms\n",
				ntohs(ip->ip_len) - iphdrlen,
				inet_ntoa(ip->ip_src),
				icmp->icmp_seq,
				ip->ip_ttl,
				GetRtt(RecvTime, SendTime));		
	}
	else if((icmp->icmp_type == ICMP_ECHO) && (icmp->icmp_id == getpid())){
		//printf("Is my echo, icmp_type = %d, icmp_id = %d\n", icmp->icmp_type, icmp->icmp_id);
		return 0;
	}
	else{
		//printf("Not my response, icmp_type = %d, icmp_id = %d\n", icmp->icmp_type, icmp->icmp_id);
		return -1;
	}

	return 1;
}

int ICMP_request(char *host, int timeout, int trytimes, int packsize)
{
	if(host == NULL || trytimes < 0){
		TRACE_ERROR("host is NULL or tyrtimes = 0");
		return -1;
	}
	
	
	int ipn_sockfd = -1, i = 0;
	struct timeval Stimeout ={ 5, 0 };
	struct timeval Rtimeout ={ timeout, 0 };
	struct in_addr _in_addr = {0};
	timeout = timeout > 0 ? timeout : 10;

#if 0
	//DNS解析出错则直接返回
	ipn = DNS_request(host ,timeout);
	if(ipn == -1){
		return -1;
	}
#endif

	if(!IsStringIp(host,&_in_addr)){
		TRACE_ERROR("%s not a ipv4's ip",host);
		return -1;

	}
	
	//创建UDP socket，并限定超时发送，接收时间
	ipn_sockfd = socket( AF_INET, SOCK_RAW, IPPROTO_ICMP );

	if( ipn_sockfd < 0 ){
		perror( "create socket failed" );
		close( ipn_sockfd );
		return -1;
	}

	if( setsockopt( ipn_sockfd, SOL_SOCKET, SO_SNDTIMEO, ( char * )&Stimeout, sizeof( struct timeval ) ) ){
		perror( "set send timeout" );
		close( ipn_sockfd );
		return -1;
	}

	if( setsockopt( ipn_sockfd, SOL_SOCKET, SO_RCVTIMEO, ( char * )&Rtimeout, sizeof( struct timeval ) ) ){
		perror( "set recv timeout" );
		close( ipn_sockfd );
		return -1;
	}

	struct sockaddr_in dest_addr;
	socklen_t len = sizeof( struct sockaddr_in );
	memset(&dest_addr, 0, sizeof(struct sockaddr_in));
	dest_addr.sin_family = AF_INET;
	dest_addr.sin_addr.s_addr = _in_addr.s_addr;

	int DataPacketSize = packsize + 64;
	char *DataPacket = (char *)calloc(DataPacketSize, sizeof(char));
	int send_fail = 0, ret = 0;

#if 0
	for(i = 0; i < trytimes; i++){
		ret = ICMP_pack(DataPacket, packsize, i);
		ret = sendto( ipn_sockfd, DataPacket, packsize, 0,
				  ( struct sockaddr * )&dest_addr, len ); //发送ICMP请求包

		if( ret != packsize ){
			//perror( "ICMP sendto" );
			//printf("ICMP send a incomplete pack, total:%d, recv:%d\n", packsize, ret);
			if(++send_fail == trytimes){
				close( ipn_sockfd );
				free(&DataPacket);
				return -1;
			}
		}
	}
#endif

	struct sockaddr_in socket_info;
	int icmp_get_res = 0 , recv_ip = -1;
	struct timeval timeo;
	fd_set rset;
	timeo.tv_sec = timeout;
    timeo.tv_usec = 0;
	time_t l_time = 0;
	
	while(1){

		if(icmp_get_res > trytimes && trytimes != 0){
			break;
		}
		icmp_get_res ++;
		
		///////////send////////////////////////
		ret = ICMP_pack(DataPacket, packsize, i);
		TRACE_DEBUG("send %d to %s[0x%x]",DataPacketSize,host,_in_addr.s_addr);
		ret = sendto( ipn_sockfd, DataPacket, packsize, 0,
				  ( struct sockaddr * )&dest_addr, len ); //发送ICMP请求包

		if( ret != packsize ){
			perror( "ICMP sendto" );
			printf("ICMP send a incomplete pack, total:%d, recv:%d\n", packsize, ret);
			close( ipn_sockfd );
			free(DataPacket);
			break;
		}
	
		//////////////////////////////////////
		l_time = time(NULL);
		while(1){
			if(time(NULL) -  l_time > timeout){
				break;
			}
			
			FD_ZERO(&rset);
	        FD_SET(ipn_sockfd, &rset);
	        ret = select(ipn_sockfd + 1, &rset, NULL, NULL, &timeo);
	        printf("ret = %d\n",ret);
	        if(FD_ISSET(ipn_sockfd,&rset)){
	        
				if(ret < 0 ){
		            if(errno == EINTR){
		                continue;
		            }
		        }else if (ret == 0){
		            continue;
		        }else{
					memset(DataPacket, 0, DataPacketSize);
					memset(&socket_info, 0, sizeof(socket_info));
					ret = recvfrom( ipn_sockfd, DataPacket, packsize, 0,
						( struct sockaddr * )&socket_info, &len );
					if(ret != packsize){
						//printf("ICMP recv a incomplete pack, total:%d, recv:%d\n", packsize, ret);
						continue;
					}
					if( ret == -1 ){
						//perror( "ICMP recvfrom" );
					}else{
						recv_ip = socket_info.sin_addr.s_addr;
						if(recv_ip != _in_addr.s_addr){
							continue;
						}
					}
					struct timeval tvDT;
					gettimeofday(&tvDT,NULL);
					
					ret = ICMP_unpack(DataPacket, ret, &tvDT);
					if(ret > 0){
						break;
					}else if(ret == 0){
						continue;
					}
				}
			}
		}
		sleep(1);
	}
	close( ipn_sockfd );
	free(DataPacket);
	return 0;
}

int ping(char *ip, int timeout , int trytimes)
{
	ICMP_request(ip,timeout,trytimes,64);	
}


