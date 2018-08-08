#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <time.h>
#include <ctype.h>

//#include "cross.h"
#include "cross_sock.h"

#if defined(_WIN32) || defined(_WIN64)
#pragma comment (lib,"WS2_32.lib")
#endif

#ifndef TRUE
#define TRUE	(1)
#endif
#ifndef FALSE
#define FALSE	(0)
#endif

#ifndef NULL
#define NULL ((void *)0)
#endif

typedef struct ether_packet {
    unsigned char targ_hw_addr[6];
    unsigned char src_hw_addr[6];
    unsigned short frame_type;
    //unsigned char *data;
} EtherPack_t;

static int _get_hw_addr(unsigned char* buf, char* str)
{
    int i;
    char c, val;
    for (i = 0; i < 6; i++) {
        if (!(c = tolower(*str++))) {
            printf("Invalid hardware address!\n");
            return -1;
        }
        if (isdigit(c)) {
            val = c - '0';
        } else if (c >= 'a' && c <= 'f') {
            val = c - 'a' + 10;
        } else if (c >= 'A' && c <= 'F') {
            val = c - 'A' + 10;
        } else {
            printf("Invalid hardware address!\n");
            return -1;
        }
        *buf = val << 4;
        if (!(c = tolower(*str++))) {
            printf("Invalid hardware address!\n");
            return -1;
        }
        if (isdigit(c)) {
            val = c - '0';
        } else if (c >= 'a' && c <= 'f') {
            val = c - 'a' + 10;
        } else if (c >= 'A' && c <= 'F') {
            val = c - 'A' + 10;
        } else {
            printf("Invalid hardware address!\n");
            return -1;
        }
        *buf++ |= val;
        if (*str == ':') {
            str++;
        } else {
            if (i < 5) {
                printf("Invalid hardware address 5 , %c!\n", *str);
                return -1;
            }
        }
    }
    if (i < 6) {
        printf("Invalid hardware address 6!\n");
        return -1;
    }

    return 0;
}


SOCK_t SOCK_new(int af,int type,int protocal)
{
    SOCK_t sock;
#if defined(_WIN32) || defined(_WIN64)
    WSADATA wsaData;
    int ret;
    ret=WSAStartup(MAKEWORD(1,1),&wsaData);
    if(ret!=0) {
        printf("WSAStartup failed!\n");
        return -1;
    }
    sock=socket(af,type,protocal);
    if(sock == INVALID_SOCKET) {
        printf("create socket failed!\n");
        WSACleanup( );
        return -1;
    }
#else
    sock = socket(af,type,protocal);
    if(sock < 0) {
        printf("create socket failed!\n");
        return -1;
    }
#endif

    return sock;
}


SOCK_t SOCK_tcp_listen(char *bindip, int listen_port, int backlog, int reuseAddr)
{
    int ret;
    int on = 1;
    SOCKADDR_IN_t addr;

    SOCK_t sock=SOCK_new(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        return -1;
    }

	if (reuseAddr != 0) {
	    ret=setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on));
	    if(ret < 0) {
	        printf("SOCK-ERROR: set port reuse failed");
	        SOCK_close(sock);
	        return -1;
	    }
	}

    memset(&addr,0,sizeof(addr));
    addr.sin_family=AF_INET;
    addr.sin_port=htons((unsigned short)listen_port);
    addr.sin_addr.s_addr= bindip ? inet_addr(bindip) : INADDR_ANY;
    ret= bind(sock,(SOCKADDR_t *)&addr,sizeof(SOCKADDR_t));
    if (ret<0) {
        printf("SOCK-ERROR: bind failed @ SOCK_ERR=%d",SOCK_ERR);
        SOCK_close(sock);
        return -1;
    }

	if (backlog <= 0)
		backlog = 32;
    ret = listen(sock,backlog);
    if (ret<0) {
        printf("SOCK-ERROR: listen failed @ SOCK_ERR=%d",SOCK_ERR);
        SOCK_close(sock);
        return -1;
    } else {
        printf("SOCK-INFO: listen start success @%d(sock:%d)\n",listen_port, sock);
    }

    return sock;
}

SOCK_t SOCK_tcp_connect(char *szip,int port,int connTimeoutMS)
{
    int ret;
    SOCK_t sock;
    SOCKADDR_IN_t addr;
    fd_set w_set;
    unsigned long ul=1;
    struct timeval tm;
    int error=-1;
    int len=sizeof(int);

    sock=SOCK_new(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
        return -1;

	// set unlock mode
    if (SOCK_IOCTL(sock,FIONBIO,&ul) != 0){
        printf("SOCK-ERROR: ioctl set unblock mode failed.\n");
		SOCK_close(sock);
		return -1;
    }

    memset(&addr, 0, sizeof(SOCKADDR_IN_t));
    addr.sin_family = AF_INET;
    addr.sin_port = htons((unsigned short)port);
    addr.sin_addr.s_addr = inet_addr(szip);
	error = 0;
    ret = connect(sock,(SOCKADDR_t *)&addr,sizeof(SOCKADDR_IN_t));
    if(ret < 0) {
        if(SOCK_EINPROGRESS != SOCK_ERR) {
            printf("SOCK-ERR: Connect to %s:%d error(\"%d\")!\n", szip, port, SOCK_ERR);
			error = 1;
        } else {
	        tm.tv_sec = connTimeoutMS / 1000;
	        tm.tv_usec= (connTimeoutMS % 1000) * 1000;
	        FD_ZERO(&w_set);
	        FD_SET(sock,&w_set);
	        if(select(sock+1,NULL,&w_set,NULL,&tm) > 0) {				
                if(FD_ISSET(sock, &w_set)) {
		            if (getsockopt(sock,SOL_SOCKET, SO_ERROR, (SOCKOPTARG_t *)&error, (socklen_t *)&len) != 0)
						error = 1;
					else {
			            if(error != 0)
			                error = 1;
					}
                } else 
                	error = 1;
	        } else {
	            error = 1;
	        }
        }
    }

    if(error != 0) {
        printf("SOCK-ERROR: connect to %s:%d failed.",szip,port);
        SOCK_close(sock);
        return -1;
    }

    ul=0;
    if (SOCK_IOCTL(sock, FIONBIO, &ul) != 0) {
        printf("SOCK-ERROR: ioctl set block mode failed.\n");
        SOCK_close(sock);
		return -1;
	}

    return sock;
}

int SOCK_tcp_connect2(char *target_ip, int target_port, int rw_timeout)
{
#if defined(_WIN32) || defined(_WIN64)
	return SOCK_tcp_connect(target_ip,target_port,rw_timeout);
#else
	int ret = 0;
	struct sockaddr_in peer_addr;
	unsigned long unblock_flag = 0;

	int sock = socket(AF_INET, SOCK_STREAM, 0);
	if ( sock <= 0){
		printf("SOCK-ERR: open socket failed\n");
	}
	
	peer_addr.sin_family = AF_INET;
	peer_addr.sin_port = htons(target_port);
	peer_addr.sin_addr.s_addr = inet_addr(target_ip);
	bzero(&(peer_addr.sin_zero), 8);
	
	unblock_flag = 1;
	ioctl(sock, FIONBIO, (typeof(unblock_flag)*)(&unblock_flag)); // unblocked connect mode

	///////////////////////////////////////////////////////
	ret = connect(sock, (struct sockaddr *)&peer_addr, sizeof(struct sockaddr_in));
	if(ret < 0){
		if(EINPROGRESS != errno){
			printf("SOCK-ERR: Connect to %s:%d error(\"%s\")!\n", target_ip,target_port,strerror(errno));
			close(sock);
			return -1;
		}else{
			struct timeval poll_timeo;
			fd_set wfd_set;
			FD_ZERO(&wfd_set);
			FD_SET(sock, &wfd_set);
			poll_timeo.tv_sec = 2;
			poll_timeo.tv_usec = 0;
			ret = select(sock + 1, NULL, &wfd_set, NULL, &poll_timeo);
			if(ret <= 0){
				close(sock);
				printf("SOCK-ERR: Connect select error(\"%s\")!\n", strerror(errno));
				return -1;
			}else{
				if(FD_ISSET(sock, &wfd_set)){
					int sock_error;
					SOCKLEN_t sock_err_len = sizeof(sock_error);
					if(getsockopt(sock, SOL_SOCKET, SO_ERROR, (char *)&sock_error, &sock_err_len) < 0){
						printf("SOCK-ERR: getsockopt: SO_ERROR failed(\"%s\")!\n", strerror(errno));
						close(sock);
						return -1;
					}
					if(0 != sock_error){
						printf("SOCK-ERR: getsockopt: SO_ERROR not zero(\"%s\")!\n", strerror(errno));
						close(sock);
						return -1;
					}
				}
			}
		}
	}

	unblock_flag = 0;
	ioctl(sock, FIONBIO, (typeof(unblock_flag)*)(&unblock_flag));

	do{
		struct timeval sock_timeo;
		int on = 1;
		sock_timeo.tv_sec = rw_timeout/1000;
		sock_timeo.tv_usec = (rw_timeout%1000)*1000;
		setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &sock_timeo, sizeof(sock_timeo));
		setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &sock_timeo, sizeof(sock_timeo));
		//setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (SOCKOPTARG_t *) &on, sizeof(on));
	}while(0);

	return sock;
#endif
}


SOCK_t SOCK_accept(SOCK_t listenfd, char *addrip, int *port)
{
    SOCK_t sock = -1;
    SOCKLEN_t len;
    SOCKADDR_IN_t addr;

	if (listenfd <= 0) return -1;
	
    len = sizeof(addr);
    sock = accept(listenfd, (SOCKADDR_t *)&addr, &len);
    if (sock < 0) {
        printf("SOCK-ERROR: accept failed @ SOCK_ERR=%d",SOCK_ERR);
        return -1;
    }

    if (addrip) {
		char temp[64];
		memset(temp, 0, sizeof(temp));
		if (inet_ntop(AF_INET, &addr.sin_addr, temp, sizeof(temp)) == NULL) {
            printf("SOCK-ERROR: inet_ntop failed @ SOCK_ERR=%d\n", SOCK_ERR);
            close(sock);
            return -1;
		} else {
			strcpy(addrip, temp);
		}
    }
    if (port)
        *port = addr.sin_port;

    printf("sock accept @(%s:%d) fd:%d\n", addrip ? addrip : "", port ? (*port) : 0, sock);

    return sock;
}


int SOCK_set_rwtimeout(SOCK_t sock,int rTimeoutMS, int wTimeoutMS)
{
	int ret;
#if defined(_WIN32) || defined(_WIN64)
	int r_timeo = rTimeoutMS;
	int w_timeo = rTimeoutMS;
#else
	struct timeval r_timeo = { rTimeoutMS/1000, (rTimeoutMS%1000)*1000 };
	struct timeval w_timeo = { wTimeoutMS/1000, (wTimeoutMS%1000)*1000 };
#endif

	if (sock > 0) {
		if (wTimeoutMS > 0) {
			ret=setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (SOCKOPTARG_t *) &w_timeo, sizeof(w_timeo));
			if(ret < 0) {
				printf("SOCK-ERROR: set send timeout failed.");
				return -1;
			}
		}
		//set receive timeout
		if (rTimeoutMS > 0) {
			ret=setsockopt(sock,SOL_SOCKET,SO_RCVTIMEO,(SOCKOPTARG_t *) &r_timeo,sizeof(r_timeo));
			if(ret < 0) {
				printf("SOCK-ERROR: set receive timeout failed.");
				return -1;
			}
		}
		return 0;
	}
	return -1;
}

int SOCK_set_tcp_nodelay(SOCK_t sock,int nodelay /* 1 for enable, 0: disable */)
{
#if defined(_WIN32) || defined(_WIN64)
	return -1;
#else
    if (sock > 0) {
	    if(setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (SOCKOPTARG_t *) &nodelay, sizeof(nodelay)) < 0){
	     	printf("SOCK-ERROR: set nodelay failed.");
	    	return -1;
	    }
		return 0;
    }else
    	return -1;
#endif
}

int SOCK_set_buf_size(SOCK_t sock, unsigned int sndBufSize, unsigned int rcvBufSize) /*if 0 , use default*/
{
    int ret = 0;
    unsigned int buf_size;
    SOCKLEN_t optlen;

	if (sock <= 0)
		return -1;
	
    // Get buffer size
    if(sndBufSize > 0) {
        optlen = sizeof(buf_size);
        ret = getsockopt(sock, SOL_SOCKET, SO_SNDBUF, (char *)&buf_size, &optlen);
        if(ret < 0) {
            printf("SOCK-ERROR: get buffer size failed");
            return -1;
        } else {
            printf("SOCK-DEBUG: send buffer size = %u\n", buf_size);
        }
        // Set buffer size
        ret = setsockopt(sock, SOL_SOCKET, SO_SNDBUF, (char *)&sndBufSize, sizeof(sndBufSize));
        if(ret < 0) {
            printf("SOCK-ERROR: set buffer size:%u failed",sndBufSize);
            return -1;
        }
    }
    if(rcvBufSize > 0) {
        optlen = sizeof(buf_size);
        ret = getsockopt(sock, SOL_SOCKET, SO_RCVBUF, (char *)&buf_size, &optlen);
        if(ret < 0) {
            printf("SOCK-ERROR: get recv buffer size failed");
            return -1;
        } else {
            printf("SOCK-DEBUG: recv buffer size = %u\n", buf_size);
        }
        // Set buffer size
        ret = setsockopt(sock, SOL_SOCKET, SO_RCVBUF, (char *)&rcvBufSize, sizeof(rcvBufSize));
        if(ret < 0) {
            printf("SOCK-ERROR: set recv buffer size:%u failed",rcvBufSize);
            return -1;
        }
    }

    return 0;
}

int SOCK_set_broadcast(SOCK_t sock, int so_broadcast)
{
    int ret;
	if (sock > 0) {
	    ret = setsockopt(sock,SOL_SOCKET,SO_BROADCAST,(char *)&so_broadcast,sizeof(so_broadcast));
	    if(ret < 0) {
	        printf("set broadcast failed1\n");
	        return -1;
	    }
		return 0;
	}
    return -1;
}

int SOCK_set_rwlowat(SOCK_t sock, int readLowat, int writeLowat)
{
    int ret;
	if (sock > 0) {
		if (readLowat > 0) {
		    ret = setsockopt(sock,SOL_SOCKET,SO_RCVLOWAT,(char *)&readLowat,sizeof(readLowat));
		    if(ret < 0) {
		        printf("SOCK_set_rcvlowat failed1\n");
		        return -1;
		    }
		}
		if (writeLowat > 0) {
			ret = setsockopt(sock,SOL_SOCKET,SO_SNDLOWAT,(char *)&writeLowat,sizeof(writeLowat));
			if(ret < 0) {
				printf("SOCK_set_sndlowat failed1\n");
				return -1;
			}
		}
	    return 0;
	} else
		return -1;
}

int SOCK_set_unblockmode(SOCK_t sock,int unblockingMode)
{
	if (sock > 0) {
	    if (SOCK_IOCTL(sock, FIONBIO, &unblockingMode) != 0){
	        printf("SOCK-ERROR: ioctl set block mode failed.\n");
			return -1;
	    }
		return 0;
	}else
		return -1;
}

SOCK_t SOCK_udp_init(char *bindip, int port, int bReuseAddr,int rwTimeoutMS)
{
    int ret;
    int on = 1;
    SOCK_t sock=-1;
    SOCKADDR_IN_t my_addr;

	sock=SOCK_new(AF_INET, SOCK_DGRAM, 0);
    if (sock <= 0 ) {
        return -1;
    }

    // set addr reuse
    if(bReuseAddr != 0) {
	    ret=setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on));
	    if(ret < 0) {
	        printf("SOCK-ERROR: set port reuse failed");
	        SOCK_close(sock);
	        return -1;
	    }
    }
	
	if (SOCK_set_rwtimeout(sock, rwTimeoutMS, rwTimeoutMS) < 0) {
		SOCK_close(sock);
		return -1;
	}

    memset(&my_addr,0,sizeof(my_addr));
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons((unsigned short)port);
    my_addr.sin_addr.s_addr = bindip ? inet_addr(bindip) : INADDR_ANY;
    //bind
    ret = bind(sock, (SOCKADDR_t*)&my_addr, sizeof(SOCKADDR_t));
    if(ret < 0) {
        printf("SOCK-ERROR: udp bind failed");
        SOCK_close(sock);
        return -1;
    }
    //printf("SOCK-DEBUG:create udp port:%d(sock:%d) ok.",port,sock);

    return sock;
}

SOCK_t SOCK_broadcast_udp_init(char *bindip, int port, int bReuseAddr,int rwTimeoutMS)
{
    int ret;
    int on = 1;
    SOCK_t sock=SOCK_new(AF_INET, SOCK_DGRAM, 0);
    SOCKADDR_IN_t my_addr;

    if (sock <=0 ) {
        return -1;
    }

    // set addr reuse
    if (bReuseAddr != 0) {
	    on = 1;
	    ret=setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on));
	    if(ret < 0) {
	        printf("SOCK-ERROR: set port reuse failed");
	        SOCK_close(sock);
	        return -1;
	    }
    }

    on = 1;
    ret=setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (char *)&on, sizeof(on));
    if(ret < 0) {
        printf("SOCK-ERROR: set broadcast failed");
        SOCK_close(sock);
        return -1;
    }

	if (SOCK_set_rwtimeout(sock, rwTimeoutMS, rwTimeoutMS) < 0) {
		SOCK_close(sock);
		return -1;
	}

    //
    memset(&my_addr,0,sizeof(my_addr));
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons((unsigned short)port);
    my_addr.sin_addr.s_addr = bindip ? inet_addr(bindip) : INADDR_ANY;
    //bind
    ret = bind(sock, (SOCKADDR_t*)&my_addr, sizeof(SOCKADDR_t));
    if(ret < 0) {
        printf("SOCK-ERROR: udp bind failed");
        SOCK_close(sock);
        return -1;
    }
    //printf("SOCK-DEBUG:create udp port:%d(sock:%d) ok.",port,sock);

    return sock;
}


SOCK_t SOCK_multicast_udp_init(char *group, int port, int bReuseAddr,int rwTimeoutMS, char *binda)
{
    int ret;
    int on = 1;
    struct ip_mreq mcaddr;
    SOCK_t sock=SOCK_new(AF_INET, SOCK_DGRAM, 0);
    SOCKADDR_IN_t my_addr;

    if (sock <= 0) {
        return -1;
    }

	if (bReuseAddr != 0) {
	    // set addr reuse
	    ret=setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on));
	    if(ret < 0) {
	        printf("SOCK-ERROR: set port reuse failed");
	        SOCK_close(sock);
	        return -1;
	    }
	}
	
	if (SOCK_set_rwtimeout(sock, rwTimeoutMS, rwTimeoutMS) < 0) {
		SOCK_close(sock);
		return -1;
	}

    //
    memset(&my_addr,0,sizeof(my_addr));
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons((unsigned short)port);
    my_addr.sin_addr.s_addr = INADDR_ANY;
    //my_addr.sin_addr.s_addr = binda ? inet_addr(binda) : INADDR_ANY;
    //bind
    ret = bind(sock, (SOCKADDR_t*)&my_addr, sizeof(SOCKADDR_t));
    if(ret < 0) {
        printf("SOCK-ERROR: udp bind %s failed errno:%d\n", binda ? binda : "", SOCK_ERR);
        SOCK_close(sock);
        return -1;
    }
    //printf("SOCK-INFO: udp bind %s ok\n", binda ? binda : "");

    memset(&mcaddr,0,sizeof(struct ip_mreq));
    // set src ip
    mcaddr.imr_interface.s_addr = binda ? inet_addr(binda) : INADDR_ANY;
    // set multicast address
    mcaddr.imr_multiaddr.s_addr = inet_addr(group);
    //add membership
    if(setsockopt(sock,IPPROTO_IP,IP_ADD_MEMBERSHIP,(char *)&mcaddr,sizeof(struct ip_mreq))<0) {
        printf("add to membership failed,errno:%d!\n",SOCK_ERR);
        SOCK_close(sock);
        return -1;
    }
    //printf("add to membership:%s ok!\n",group);

    return sock;
}

SOCK_t SOCK_raw_init(int protocal, int rwTimeoutMS)
{
#if defined(_WIN32) || defined(_WIN64)
    printf("ERROR : not implement!!!!\n");
    return -1;
#else
    int sock;

    sock = SOCK_new(AF_INET, SOCK_PACKET, htons(protocal));
    if (sock < 0) {
        return -1;
    }

	if (SOCK_set_rwtimeout(sock, rwTimeoutMS, rwTimeoutMS) < 0) {
		SOCK_close(sock);
		return -1;
	}

    return sock;
#endif
}

/*
	if target_mac is NULL, given should contain the ether header
	else you should given target_mac & frame_type
*/
int SOCK_raw_sendto(SOCK_t sock, char *bind_ether, char *target_mac, int frame_type, char *buf, int size)
{
#if defined(_WIN32) || defined(_WIN64)
    printf("ERROR : not implement!!!!\n");
    return -1;
#else
    int ret;
    SOCKADDR_t addr;
    int data_size = size;
    char *pbuf = buf;

    if (bind_ether == NULL) return -1;

    if (target_mac) {
        EtherPack_t *ether;
        char thiz_mac[32];

        pbuf = (char  *)malloc(size + sizeof(EtherPack_t));
        if (pbuf == NULL) return -1;

        ether = (EtherPack_t *)pbuf;
        if (SOCK_get_ether_ip(bind_ether, NULL, NULL, thiz_mac) < 0) {
            free(pbuf);
            return -1;
        }
        if (_get_hw_addr(ether->src_hw_addr, thiz_mac) < 0) {
            free(pbuf);
            return -1;
        }
        if (_get_hw_addr(ether->targ_hw_addr, target_mac) < 0) {
            free(pbuf);
            return -1;
        }
        ether->frame_type = htons(frame_type);

        memcpy(pbuf + sizeof(EtherPack_t) , buf, size);
        data_size += sizeof(EtherPack_t);
    }

    memset(&addr, 0, sizeof(SOCKADDR_t));
    strcpy(addr.sa_data, bind_ether);

    ret=sendto(sock,pbuf,data_size,0,(SOCKADDR_t *)&addr,sizeof(SOCKADDR_IN_t));
    if (target_mac)
        free(pbuf);
    if(ret != data_size) {
        printf("SOCK-ERROR: raw packet send failed, SOCK_ERR:%d\n", SOCK_ERR);
        return RET_FAIL;
    }

    return RET_OK;
#endif

}

int SOCK_recv(SOCK_t sock,void *buf,unsigned int size, int flag)
{
    int ret=0;
    ret=recv(sock,buf,size,flag);
    if(ret < 0) {
        printf("SOCK-ERROR: read failed, errno: %d", SOCK_ERR);
        return -1;
    } else if(ret == 0) {
        printf("SOCK-ERROR: peer is shut down");
        return -1;
    }
    //printf("SOCK-DEBUG:tcp recv %d\n",ret);
    //buf[ret] = 0;
    return ret;
}

/*
* recv all available data until socket buffer empty
*/
unsigned int SOCK_unblock_recv_n(SOCK_t sock,void *buf,unsigned int bufSize, int flag)
{
    int ret=0;
    unsigned int received=0;
    char *pbuf=buf;
	
    if(bufSize <= 0) return 0;
	
    for(;;) {
        ret=recv(sock, pbuf, bufSize-received, flag);
        if (ret < 0) {
            if (SOCK_ERR==SOCK_EINTR) {
                printf("SOCK-DEBUG: tcp recv error SOCK_EINTR \n");
                continue;
            } else if (SOCK_ERR==SOCK_EAGAIN) {
                //printf("SOCK-DEBUG: tcp recv error SOCK_EAGAIN in nonblocking mode, %u/%u\n", received, size);
                return received;
            } else if (SOCK_ERR==SOCK_ETIMEOUT) {
                printf("SOCK-ERROR:  tcp recv time out \n");
                return 0;
            }
            printf("SOCK-ERROR: tcp recv error @%d \n",SOCK_ERR);
            return -1;
        } else if (ret==0) {
            return -1;
        } else {
            pbuf+=ret;
            received+=ret;
        }
        if(received == bufSize) break;
    }
    //buf[received]=0;
    //printf("SOCK-DEBUG: tcp recv %d\n",received);
    return received;
}

/*
* recv until reach the the size to be read or error occur
*/
unsigned int SOCK_recv_n(SOCK_t sock, void *buf,unsigned int needToReadSize,int flag)
{
    int ret=0;
    unsigned int received=0;
    char *pbuf=buf;
	
    if(needToReadSize <= 0) return 0;
	
    for(;;) {
        ret=recv(sock, pbuf,needToReadSize-received, flag);
        if (ret < 0) {
            if (SOCK_ERR==SOCK_EINTR) {
                printf("SOCK-DEBUG: tcp recv error SOCK_EINTR \n");
                continue;
            } else if (SOCK_ERR==SOCK_EAGAIN) {
                printf("SOCK-DEBUG: tcp recv error SOCK_EAGAIN in nonblocking mode, %u/%u\n", received, needToReadSize);
                return -1;
            } else if (SOCK_ERR==SOCK_ETIMEOUT) {
                printf("SOCK-ERROR:  tcp recv time out \n");
                return 0;
            }
            printf("SOCK-ERROR: tcp recv error @%d \n",SOCK_ERR);
            return -1;
        } else if (ret==0) {
            //printf("peer close\n");
            return received;
        } else {
            pbuf+=ret;
            received+=ret;
        }
        if(received == needToReadSize) break;
    }
    //buf[received]=0;
    //printf("SOCK-DEBUG: tcp recv %d\n",received);
    return received;
}

unsigned int SOCK_recv_n0(SOCK_t sock, void *buf,unsigned int needToReadSize, int timeout_s, unsigned int lowat,int flag)
{
    unsigned int received=0;
	fd_set rset;
	fd_set errset;
	int ret;
	struct timeval timeo;
	unsigned int remind = needToReadSize;
	char *ptr = buf;
	
    if(needToReadSize <= 0) return 0;
	
	timeo.tv_sec = timeout_s;
	timeo.tv_usec = 0;

	if (lowat == 0)
		lowat = needToReadSize;
	SOCK_set_rwlowat(sock, lowat, 0);

	while(remind > 0) {
		FD_ZERO(&rset);
		FD_SET(sock, &rset);
		FD_ZERO(&errset);
		FD_SET(sock, &errset);
		ret = select(sock + 1, &rset, NULL, &errset, & timeo);
		if(ret > 0) {
			if(FD_ISSET(sock, &errset)) {
				printf("sock write error!\n");
				break;
			}
			if(FD_ISSET(sock, &rset)) {
				ret=recv(sock, ptr,remind, flag);
				if (ret < 0) {
					if (SOCK_ERR==SOCK_EINTR) {
						printf("SOCK-DEBUG: tcp recv error SOCK_EINTR \n");
						continue;
					} else if (SOCK_ERR==SOCK_EAGAIN) {
						printf("SOCK-DEBUG: tcp recv error SOCK_EAGAIN in nonblocking mode, %u/%u\n", received, needToReadSize);
						return -1;
					} else if (SOCK_ERR==SOCK_ETIMEOUT) {
						printf("SOCK-ERROR:  tcp recv time out \n");
						return 0;
					}
					printf("SOCK-ERROR: tcp recv error @%d \n",SOCK_ERR);
					return -1;
				} else if (ret==0) {
					printf("peer close\n");
					return received;
				} else {
					ptr+=ret;
					remind -= ret;
					received += ret;
				}
				if(received >= needToReadSize) break;


			} else {
				goto ERR_EXIT;
			}
		} else if(ret == 0) {
			goto ERR_EXIT;
		} else {
			if (SOCK_ERR==SOCK_EINTR) {
				printf("SOCK-DEBUG: tcp recv error SOCK_EINTR\n");
				continue;
			}
			goto ERR_EXIT;
		}
	}

	//printf("sock send (size:%d) ok\n", size);
	return RET_OK;

ERR_EXIT:
	printf("sock recv (size:%u) failed #errno:%d\n", needToReadSize, SOCK_ERR);
	return RET_FAIL;

}


int SOCK_send(SOCK_t sock,void *buf,unsigned int size, int flag)
{
    fd_set wset;
    fd_set errset;
    int ret, timeout = 0;
    struct timeval timeo;
    unsigned int remind = size;
    char *ptr = buf;

    while(remind > 0) {
        timeo.tv_sec = 1;
        timeo.tv_usec = 0;
        FD_ZERO(&wset);
        FD_SET(sock, &wset);
        FD_ZERO(&errset);
        FD_SET(sock, &errset);
        ret = select(sock + 1, NULL, &wset, &errset, & timeo);
        if(ret > 0) {
            if(FD_ISSET(sock, &errset)) {
				printf("sock write error!\n");
				break;
			}
            else if(FD_ISSET(sock, &wset)) {
                ret=send(sock, ptr, remind, flag);
                if(ret < 0) {
                    if(SOCK_ERR == SOCK_EAGAIN) {
#if defined(_WIN32) || defined(_WIN64)
						Sleep(3);
#else
                        usleep(3000);
#endif
                        continue;
                    } else if(SOCK_ERR == SOCK_EINTR) {
                        continue;
                    } else {
                        goto ERR_EXIT;
                    }
                } else {
                    ptr += ret;
                    remind -= ret;
                }

            } else {
                goto ERR_EXIT;
            }
        } else if(ret == 0) {
        	timeout++;
			if (timeout > (DEFAULT_SOCK_TIMEOUT/1000)) {
                goto ERR_EXIT;
			}
            continue;
        } else {
            goto ERR_EXIT;
        }
    }

    //printf("sock send (size:%d) ok\n", size);
    return RET_OK;

ERR_EXIT:
    printf("sock send (size:%u) failed #errno:%d\n", size, SOCK_ERR);
    return RET_FAIL;
}

#if 0
int SOCK_send2(SOCK_t sock,char *buf,unsigned int size)
{
    fd_set wset;
    int ret;
    struct timeval timeo;
    unsigned int remind = size;
    char *ptr = buf;

    while(remind > 0) {
        timeo.tv_sec = 3;
        timeo.tv_usec = 0;
        FD_ZERO(&wset);
        FD_SET(sock, &wset);
        ret = select(sock + 1, NULL, &wset, NULL, & timeo);
        if(ret > 0) {
            if(FD_ISSET(sock, &wset)) {
                ret=send(sock, ptr, (remind >= 1280) ? 1280 : remind,0);
                if(ret < 0) {
                    if(SOCK_ERR == SOCK_EAGAIN) {
                        sleep_ms_c(3);
                        continue;
                    } else if(SOCK_ERR == SOCK_EINTR) {
                        continue;
                    } else {
                        goto ERR_EXIT;
                    }
                } else {
                    ptr += ret;
                    remind -= ret;
                }
            } else {
                goto ERR_EXIT;
            }
        } else if( ret == 0) {
            continue;
        } else {
            goto ERR_EXIT;
        }
    }

    //printf("sock send (size:%d) ok\n", size);
    return RET_OK;

ERR_EXIT:
    printf("sock send (size:%d) failed #errno:%d\n", size, SOCK_ERR);
    return RET_FAIL;
}
#endif


int SOCK_unblock_recvfrom(SOCK_t sock,char *ip,int *port,void *buf,unsigned int size, int flags)
{
    int ret;
    SOCKADDR_IN_t addr;
    SOCKLEN_t addrlen=sizeof(SOCKADDR_IN_t);
    memset(&addr, 0, sizeof(SOCKADDR_IN_t));

    ret=recvfrom(sock,buf,size,flags,(SOCKADDR_t *)&addr,&addrlen);
    if(ret < 0) {
        if (SOCK_ERR == SOCK_EAGAIN) {
            return SOCK_EAGAIN;
        }
        printf("SOCK-ERROR: udp recvfrom failed,size:%u sock:%d buf:%p SOCK_ERR:%d",
               size,sock,buf,SOCK_ERR);
        return RET_FAIL;
    } else if(ret == 0) {
        printf("SOCK-ERROR: peer is shut down");
        return RET_FAIL;
    }
    //printf("udp recvfrom(%s:%d) success,size:%d",inet_ntoa(addr.sin_addr),ntohs(addr.sin_port),ret);
    //printf("SOCK-DEBUG: udp recvfrom(%s:%d) success,size:%d",inet_ntoa(addr.sin_addr),ntohs(addr.sin_port),ret);

	if (ip) {
		char temp[64];
		memset(temp, 0, sizeof(temp));
		if (inet_ntop(AF_INET, &addr.sin_addr, temp, sizeof(temp)) == NULL) {
            printf("SOCK-ERROR: inet_ntop failed @ SOCK_ERR=%d\n", SOCK_ERR);
            return RET_FAIL;
		} else {
			strcpy(ip, temp);
		}
	}
	if (port)
    	*port = ntohs(addr.sin_port);

    return ret;
}

int SOCK_recvfrom(SOCK_t sock,char *ip,int *port,void *buf,unsigned int size, int flags)
{
    int ret;
    SOCKADDR_IN_t addr;
    SOCKLEN_t addrlen=sizeof(SOCKADDR_IN_t);
    memset(&addr, 0, sizeof(SOCKADDR_IN_t));

    ret=recvfrom(sock,buf,size,flags,(SOCKADDR_t *)&addr,&addrlen);
    if(ret < 0) {
        printf("SOCK-ERROR: udp recvfrom failed,size:%u sock:%d buf:%p SOCK_ERR:%d",
               size,sock,buf,SOCK_ERR);
        return RET_FAIL;
    } else if(ret == 0) {
        printf("SOCK-ERROR: peer is shut down");
        return RET_FAIL;
    }
    //printf("udp recvfrom(%s:%d) success,size:%d",inet_ntoa(addr.sin_addr),ntohs(addr.sin_port),ret);
    //printf("SOCK-DEBUG: udp recvfrom(%s:%d) success,size:%d",inet_ntoa(addr.sin_addr),ntohs(addr.sin_port),ret);

	if (ip) {
		char temp[64];
		memset(temp, 0, sizeof(temp));
		if (inet_ntop(AF_INET, &addr.sin_addr, temp, sizeof(temp)) == NULL) {
            printf("SOCK-ERROR: inet_ntop failed @ SOCK_ERR=%d\n", SOCK_ERR);
            return RET_FAIL;
		} else {
			strcpy(ip, temp);
		}
	}
	if (port)
    	*port = ntohs(addr.sin_port);

    return ret;
}


int SOCK_sendto(SOCK_t sock,char *ip,int port,void *buf,unsigned int size, int flags)
{
    int ret;
    SOCKADDR_IN_t addr;
    memset(&addr, 0, sizeof(SOCKADDR_IN_t));
    addr.sin_family = AF_INET;
    addr.sin_port = htons((unsigned short)port);
    addr.sin_addr.s_addr = inet_addr(ip);

    ret=sendto(sock,buf,size, flags,(SOCKADDR_t *)&addr,sizeof(SOCKADDR_IN_t));
    if(ret != size) {
        printf("SOCK-ERROR: udp send to %s:%d failed,size:%u sock:%d buf:%p SOCK_ERR:%d\n",
               ip,port,size,sock,buf,SOCK_ERR);
        return RET_FAIL;
    }
    //printf("SOCK-DEBUG: udp send to%s:%d uccess,size:%d\n",ip,port,size);
    return RET_OK;
}

int SOCK_getpeername(SOCK_t sock,char *peer)
{
    SOCKADDR_t addr;
    SOCKADDR_IN_t *addr_in=(SOCKADDR_IN_t *)&addr;
    SOCKLEN_t sock_len=sizeof(SOCKADDR_t);
    if(getpeername(sock,&addr,&sock_len)!=0) {
        printf("SOCK-ERROR: get peer name failed");
        return RET_FAIL;
    }
	if (peer) {
		char temp[64];
		memset(temp, 0, sizeof(temp));
		if (inet_ntop(AF_INET, &addr_in->sin_addr, temp, sizeof(temp)) == NULL) {
			printf("SOCK-ERROR: inet_ntop failed @ SOCK_ERR=%d\n", SOCK_ERR);
			return RET_FAIL;
		} else {
			strcpy(peer, temp);
		}
	}
    //printf("SOCK-DEBUG: peer name:%s",peer);

    return RET_OK;
}

int SOCK_getsockname(SOCK_t sock,char *ip)
{
    SOCKADDR_t addr;
    SOCKADDR_IN_t *addr_in=(SOCKADDR_IN_t *)&addr;
    SOCKLEN_t sock_len=sizeof(SOCKADDR_t);
    if(getsockname(sock,&addr,&sock_len)!=0) {
        printf("SOCK-ERROR: get peer name failed");
        return RET_FAIL;
    }
	if (ip) {
		char temp[64];
		memset(temp, 0, sizeof(temp));
		if (inet_ntop(AF_INET, &addr_in->sin_addr, temp, sizeof(temp)) == NULL) {
			printf("SOCK-ERROR: inet_ntop failed @ SOCK_ERR=%d\n", SOCK_ERR);
			return RET_FAIL;
		} else {
			strcpy(ip, temp);
		}
	}
    //printf("SOCK-DEBUG: sock name:%s",ip);

    return RET_OK;
}

int SOCK_getsockport(SOCK_t sock,unsigned short * const port)
{
    SOCKADDR_t addr;
    SOCKADDR_IN_t *addr_in=(SOCKADDR_IN_t *)&addr;
    SOCKLEN_t sock_len=sizeof(SOCKADDR_t);
    if(getsockname(sock,&addr,&sock_len)!=0) {
        printf("SOCK-ERROR: get peer name failed");
        return RET_FAIL;
    }
    *port = ntohs(addr_in->sin_port);
    //printf("SOCK-PORT: sock port:%d",*port);

    return RET_OK;
}

int SOCK_gethostbyname(char *name,char *ip)
{
    struct hostent *hent;
    struct in_addr addr;
    int i;
	char temp[64];
	const char *ptr = NULL;

    hent = gethostbyname(name);
    if(hent == NULL) {
        printf("gethostbyname failed!\n");
        return -1;
    }
#if 0
    for(i = 0; hent->h_aliases[i]; i++){
        printf("aliase%d %s\n",i+1,hent->h_aliases[i]);
    }
    printf("hostname: %s addrtype:%d\naddress list: ", hent->h_name,hent->h_addrtype);
#endif
    if(hent->h_addrtype == AF_INET) {
        for(i = 0; hent->h_addr_list[i]; i++) {
#if defined(_WIN32) || defined(_WIN64)
            addr.s_addr = *(u_long *)hent->h_addr_list[i];
#else
            addr.s_addr = *(unsigned int *)hent->h_addr_list[i];
#endif
			ptr = inet_ntop(AF_INET, &addr, temp, sizeof(temp));
            printf("<<%d>> %s\n", i+1, ptr ? ptr : "invalid ip");

            if(i == 0 && ptr) {
                strcpy(ip, ptr);
            }
        }

    } else {
        return -1;
    }
    return 0;
}

int SOCK_getallhostip(void (*f_Add)(char *ip))
{
#if defined(_WIN32) || defined(_WIN64)
    printf("not support!!!!!!!!!!\n");
    return -1;
#else
#ifdef HAVE_IFADDRS_H
    struct ifaddrs * ifAddrStruct=NULL,*ifaddr_bak;
    void * tmpAddrPtr=NULL;

    getifaddrs(&ifAddrStruct);
    ifaddr_bak = ifAddrStruct;

    while (ifAddrStruct!=NULL) {
        if (ifAddrStruct->ifa_addr->sa_family==AF_INET) { // check it is IP4
            // is a valid IP4 Address
            tmpAddrPtr=&((struct sockaddr_in *)ifAddrStruct->ifa_addr)->sin_addr;
            char addressBuffer[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);
            printf("%s IP Address %s\n", ifAddrStruct->ifa_name, addressBuffer);
            if(strcmp(ifAddrStruct->ifa_name,"lo")!=0 ) {
                if(f_Add) f_Add(addressBuffer);
            }
        } else if (ifAddrStruct->ifa_addr->sa_family==AF_INET6) { // check it is IP6
            // is a valid IP6 Address
            tmpAddrPtr=&((struct sockaddr_in *)ifAddrStruct->ifa_addr)->sin_addr;
            char addressBuffer[INET6_ADDRSTRLEN];
            inet_ntop(AF_INET6, tmpAddrPtr, addressBuffer, INET6_ADDRSTRLEN);
            printf("%s IP Address %s\n", ifAddrStruct->ifa_name, addressBuffer);
        }
        ifAddrStruct=ifAddrStruct->ifa_next;
    }
    if(ifaddr_bak) freeifaddrs(ifaddr_bak);
    return 0;
#else
    printf("not support!!!!!!!!!!\n");
    return -1;
#endif
#endif
    return 0;
}



int SOCK_getsockinfo(SOCK_t sock,char *ip,char *mask)
{
#if defined(_WIN32) || defined(_WIN64)
    return -1;
#else
    SOCKADDR_IN_t *my_ip;
    SOCKADDR_IN_t *addr;
    SOCKADDR_IN_t myip;
    struct ifreq ifr;
	char temp[64];

    my_ip = &myip;
    // get local ip of eth0
    strcpy(ifr.ifr_name, "eth0");
    if(ioctl(sock, SIOCGIFADDR, &ifr) < 0) {
        printf("SOCK-ERROR: get local ip failed");
        return -1;
    }
    my_ip->sin_addr = ((struct sockaddr_in *)(&ifr.ifr_addr))->sin_addr;
	if (ip) {
		memset(temp, 0, sizeof(temp));
		if (inet_ntop(AF_INET, &my_ip->sin_addr, temp, sizeof(temp)) == NULL) {
			printf("SOCK-ERROR: inet_ntop failed @ SOCK_ERR=%d\n", SOCK_ERR);
			return RET_FAIL;
		} else {
			strcpy(ip, temp);
		}
	}

    // get local netmask
    if( ioctl( sock, SIOCGIFNETMASK, &ifr) == -1 ) {
        printf("SOCK-ERROR: get netmask failed");
        return -1;
    }
    addr = (struct sockaddr_in *) & (ifr.ifr_addr);
	if (mask) {
		memset(temp, 0, sizeof(temp));
		if (inet_ntop(AF_INET, &addr->sin_addr, temp, sizeof(temp)) == NULL) {
			printf("SOCK-ERROR: inet_ntop failed @ SOCK_ERR=%d\n", SOCK_ERR);
			return RET_FAIL;
		} else {
			strcpy(mask, temp);
		}
	}

    //printf("SOCK-INFO: sockinfo: ip:%s mask:%s",ip?ip:"null",mask?mask:"null");
    return 0;
#endif
}

int SOCK_get_blockmode(SOCK_t sock)
{
    int ret=0;
#if defined(_WIN32) || defined(_WIN64)
    return -1;
#else
    ret= fcntl(sock,F_GETFL,0);
    if(ret < 0) {
        printf("SOCK-ERROR: SOCK fcntl failed\n");
        return RET_FAIL;
    }
    if(ret & O_NONBLOCK) {
        printf("SOCK-INFO: SOCK in nonblock mode\n");
        return FALSE;
    } else {
        printf("SOCK-INFO: SOCK in block mode\n");
        return TRUE;
    }
#endif
}




int SOCK_isreservedip(char *szIp)
{
    int ret;
    int flag = FALSE;
    int ip[4];
    ret=sscanf(szIp,"%d.%d.%d.%d",&ip[0],&ip[1],&ip[2],&ip[3]);
    if(ret != 4) {
        printf("SOCK-ERROR: ipaddr:%s invalid",szIp);
        return -1;
    }
    if(ip[0] == 10) flag = TRUE;
    if((ip[0] == 192) && (ip[1] == 168)) flag = TRUE;
    if((ip[0] == 172) && (ip[1] >= 16) && (ip[1] <= 31)) flag = TRUE;

    return flag;
}

// multicast ip address
// use for router and other function:224.0.0.0~224.0.0.255
// reserved: 224.0.1.0~238.255.255.255
// 239.0.0.0~239.255.255.255
int SOCK_add_membership(SOCK_t sock,char *multi_ip)
{
    struct ip_mreq mcaddr;
    memset(&mcaddr,0,sizeof(struct ip_mreq));
    // set src ip
    mcaddr.imr_interface.s_addr = htonl(INADDR_ANY);
    // set multicast address
    mcaddr.imr_multiaddr.s_addr = inet_addr(multi_ip);
    //if(inet_pton(AF_INET,multi_ip,&mcaddr.imr_multiaddr) <=0){
    //	printf("wrong multicast ipaddress!\n");
    //	return -1;
    //}
    //add membership
    if(setsockopt(sock,IPPROTO_IP,IP_ADD_MEMBERSHIP,(char *)&mcaddr,sizeof(struct ip_mreq))<0) {
        printf("add to membership failed,errno:%d!\n",SOCK_ERR);
        return -1;
    }
    //printf("add to membership:%s ok!\n",multi_ip);
    return 0;
}


int SOCK_get_ether_ip(char *eth, char * const ip, char * const netmask, char * const mac)
{
#if defined(_WIN32) || defined(_WIN64)
    printf("ERROR: %s unsupperted in this os!\n", __FUNCTION__);
    return -1;
#else
    int sock;
	char temp[64];
    //
    struct sockaddr_in sin;
    struct ifreq ifr;

    if(ip) 			ip[0] = 0;
    if(netmask) 	netmask[0] = 0;
    if(mac) 		mac[0]= 0;

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock == -1) {
        perror("socket");
        return -1;
    }

    strncpy(ifr.ifr_name, eth, IFNAMSIZ);
    ifr.ifr_name[IFNAMSIZ - 1] = 0;

    if(ip) {
        if (ioctl(sock, SIOCGIFADDR, &ifr) < 0) {
            perror("ioctl");
            close(sock);
            return -1;
        }
        memcpy(&sin, &ifr.ifr_addr, sizeof(sin));
		memset(temp, 0, sizeof(temp));
		if (inet_ntop(AF_INET, &sin.sin_addr, temp, sizeof(temp)) == NULL) {
			printf("SOCK-ERROR: inet_ntop failed @ SOCK_ERR=%d\n", SOCK_ERR);
			close(sock);
			return RET_FAIL;
		} else {
			strcpy(ip, temp);
		}
    }

    if(netmask) {
        if (ioctl(sock, SIOCGIFNETMASK, &ifr)< 0) {
            perror("ioctl");
            close(sock);
            return -1;
        }
        memcpy(&sin, &ifr.ifr_addr, sizeof(sin));
		memset(temp, 0, sizeof(temp));
		if (inet_ntop(AF_INET, &sin.sin_addr, temp, sizeof(temp)) == NULL) {
			printf("SOCK-ERROR: inet_ntop failed @ SOCK_ERR=%d\n", SOCK_ERR);
			close(sock);
			return RET_FAIL;
		} else {
			strcpy(netmask, temp);
		}
    }

    if(mac) {
        if (ioctl(sock, SIOCGIFHWADDR, &ifr) < 0) {
            perror("ioctl");
            close(sock);
            return -1;
        }
        sprintf(mac, "%02X:%02X:%02X:%02X:%02X:%02X", ifr.ifr_hwaddr.sa_data[0] & 0xff,ifr.ifr_hwaddr.sa_data[1] & 0xff,ifr.ifr_hwaddr.sa_data[2] & 0xff,
                ifr.ifr_hwaddr.sa_data[3] & 0xff,ifr.ifr_hwaddr.sa_data[4] & 0xff,ifr.ifr_hwaddr.sa_data[5] & 0xff);
    }

    close(sock);
    return 0;
#endif
}

int SOCK_get_gateway(char * const gateway)
{
#if defined(_WIN32) || defined(_WIN64)
    printf("ERROR: %s unsupperted in this os!\n", __FUNCTION__);
    return -1;
#else
    FILE *fp;
    char buf[256]; // 128 is enough for linux
    char iface[16];
    unsigned char _gateway[4];
    unsigned long dest_addr;
    unsigned long gate_addr;
    int is_found = 0;

    gateway[0] = 0;
    fp = fopen("/proc/net/route", "r");
    if(fp != NULL) {
        fgets(buf, sizeof(buf), fp);
        while (fgets(buf, sizeof(buf), fp)) {
            if (sscanf(buf, "%15s\t%lX\t%lX", iface, &dest_addr, &gate_addr) != 3 || dest_addr != 0) {
                continue;
            } else {
                is_found = 1;
                break;
            }
        }
        fclose(fp);
    }


    if(is_found == 1) {
        int i;
        for(i = 0; i < 4; i++) {
            *(_gateway+i) = (gate_addr >> (i*8)) & 0xff;
        }
        sprintf(gateway, "%d.%d.%d.%d", _gateway[0], _gateway[1], _gateway[2], _gateway[3]);
        return 0;
    } else {
        return -1;
    }
#endif
}


int SOCK_keep_alive(int sock, int keepidle, int keepintvl, int keepcnt)
{
#if defined(_WIN32) || defined(_WIN64)
    printf("ERROR: %s unsupperted in this os!\n", __FUNCTION__);
    return -1;
#else
    int ret;
    int keepalive = 1;
    ret=setsockopt(sock,SOL_SOCKET,SO_KEEPALIVE,(SOCKOPTARG_t *) &keepalive,sizeof(keepalive));
    if(ret < 0) {
        printf("SOCK-ERROR: set sockopt failed, error:%d\n", SOCK_ERR);
        return -1;
    }
    ret=setsockopt(sock,SOL_TCP,TCP_KEEPIDLE,(SOCKOPTARG_t *) &keepidle,sizeof(keepidle));
    if(ret < 0) {
        printf("SOCK-ERROR: set sockopt failed, error:%d\n", SOCK_ERR);
        return -1;
    }
    ret=setsockopt(sock,SOL_TCP,TCP_KEEPINTVL,(SOCKOPTARG_t *) &keepintvl,sizeof(keepintvl));
    if(ret < 0) {
        printf("SOCK-ERROR: set sockopt failed, error:%d\n", SOCK_ERR);
        return -1;
    }
    ret=setsockopt(sock,SOL_TCP,TCP_KEEPCNT,(SOCKOPTARG_t *) &keepcnt,sizeof(keepcnt));
    if(ret < 0) {
        printf("SOCK-ERROR: set sockopt failed, error:%d\n", SOCK_ERR);
        return -1;
    }
    return 0;
#endif
}

int SOCK_get_ip_only(char *eth, unsigned char * const ip)
{
#if defined(_WIN32) || defined(_WIN64)
    printf("ERROR: %s unsupperted in this os!\n", __FUNCTION__);
    return -1;
#else
    int sock;
    //
    struct sockaddr_in sin;
    struct ifreq ifr;

    if(ip)                  ip[0] = 0;

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock == -1) {
        perror("socket");
        return -1;
    }

    strncpy(ifr.ifr_name, eth, IFNAMSIZ);
    ifr.ifr_name[IFNAMSIZ - 1] = 0;

    if(ip) {
        if (ioctl(sock, SIOCGIFADDR, &ifr) < 0) {
            perror("ioctl");
            close(sock);
            return -1;
        }
        memcpy(&sin, &ifr.ifr_addr, sizeof(sin));
        memcpy(ip, &sin.sin_addr, sizeof(sin.sin_addr));
    }

    close(sock);
    return 0;
#endif
}

int SOCK_check_nic(char *nic_name)
{
#if defined(_WIN32) || defined(_WIN64)
    printf("ERROR: %s unsupperted in this os!\n", __FUNCTION__);
    return -1;
#else
    struct ifreq ifr;
    int skfd = socket(AF_INET, SOCK_DGRAM, 0);

    if ( skfd < 0 ) return 0;
    strcpy(ifr.ifr_name, nic_name);
    if (ioctl(skfd, SIOCGIFFLAGS, &ifr) < 0) {
        close(skfd);
        return 0;
    }
    close(skfd);
    if(ifr.ifr_flags & IFF_RUNNING) {
        return 1;
    } else {
        return 0;
    }
#endif
}


static void _check_and_delete_file(const char *path)
{
#if defined(_WIN32) || defined(_WIN64)
    DeleteFile(path);
#else
    unlink(path);
#endif
}


SOCK_t SOCK_domain_listen(const char *name)
{
#if defined(_WIN32) || defined(_WIN64)
    char szip[64];
    int port;
    if (name == NULL) return -1;
    if (sscanf(name, "%[^:]:%d", szip, &port) != 2) return -1;
    return SOCK_tcp_listen("127.0.0.1", port, 0, 0);
#else
    int ret;
    SOCKADDR_UN_t addr;
    SOCK_t sock;

    // check name is exist or not, if exist ,delete it
    _check_and_delete_file(name);

    sock=SOCK_new(AF_UNIX, SOCK_STREAM, 0);
    if (sock < 0)
        return -1;

    memset(&addr,0,sizeof(addr));
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, name);
    ret= bind(sock,(SOCKADDR_t *)&addr,offsetof(SOCKADDR_UN_t, sun_path) + strlen(name));
    if (ret<0) {
        printf("SOCK-ERROR: bind failed @ SOCK_ERR=%d",SOCK_ERR);
        SOCK_close(sock);
        return -1;
    }
    ret = listen(sock, 5);
    if (ret<0) {
        printf("SOCK-ERROR: listen failed @ SOCK_ERR=%d",SOCK_ERR);
        SOCK_close(sock);
        return -1;
    } else {
        printf("SOCK-INFO: listen start success @%s(sock:%d)\n",name, sock);
    }

    return sock;
#endif
}

SOCK_t SOCK_domain_connect(const char *localPath, const char *serverPath)
{
#if defined(_WIN32) || defined(_WIN64)
    char peer_szip[64], local_szip[64];
    int peer_port, local_port;

    if (localPath == NULL || serverPath == NULL) return -1;
    if (sscanf(localPath, "%[^:]:%d", local_szip, &local_port) != 2) return -1;
    if (sscanf(serverPath, "%[^:]:%d", peer_szip, &peer_port) != 2) return -1;
    return SOCK_tcp_connect(peer_szip, peer_port, 200);
#else
    int ret;
    SOCK_t sock;
    SOCKADDR_UN_t addr;
    SOCKADDR_UN_t local_addr;

    // check localPath is exist or not, if exist ,delete it
    _check_and_delete_file(localPath);
    sock=SOCK_new(AF_UNIX, SOCK_STREAM, 0);

    memset(&local_addr, 0, sizeof(SOCKADDR_IN_t));
    local_addr.sun_family = AF_UNIX;
    strcpy(local_addr.sun_path, localPath);
    ret= bind(sock,(SOCKADDR_t *)&local_addr,offsetof(SOCKADDR_UN_t, sun_path) + strlen(localPath));
    if (ret<0) {
        printf("SOCK-ERROR: bind failed @ SOCK_ERR=%d,close(%d)\n",SOCK_ERR,sock);
		SOCK_close(sock);
        return -1;
    }

    memset(&addr, 0, sizeof(SOCKADDR_IN_t));
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, serverPath);
    ret = connect(sock,(SOCKADDR_t *)&addr,offsetof(SOCKADDR_UN_t, sun_path) + strlen(serverPath));
    if (ret < 0) {
        printf("SOCK-ERROR: connect failed @ SOCK_ERR=%d,close(%d)\n",SOCK_ERR,sock);
		SOCK_close(sock);
        return -1;
    }

    return sock;
#endif
}

int SOCK_domain_send(SOCK_t sock, void *data, int size)
{
    return -1;
}

SOCK_t SOCK_domain_accept(SOCK_t listenfd, char *peer_path)
{
#if defined(_WIN32) || defined(_WIN64)
    char szip[64];
    int port;
    SOCK_t sock = SOCK_accept(listenfd, szip, &port);
    if ((sock > 0) && peer_path) {
        sprintf(peer_path, "%s:%d", szip, port);
    }
    return sock;
#else
    SOCK_t sock = -1;
    SOCKLEN_t len;
    SOCKADDR_UN_t addr;
    char path[150];

    len = sizeof(addr);
    sock = accept(listenfd, (SOCKADDR_t *)&addr, &len);
    if (sock < 0) {
        printf("SOCK-ERROR: accept failed @ SOCK_ERR=%d",SOCK_ERR);
        return -1;
    }
    strncpy(path, addr.sun_path, len - offsetof(SOCKADDR_UN_t, sun_path));
    if (peer_path) {
        strncpy(peer_path, addr.sun_path, len - offsetof(SOCKADDR_UN_t, sun_path));
    }
    //FIX ME , delete this file

    //printf("sock accept @%s fd:%d\n", path, sock);

    return sock;
#endif
}





#ifdef SOCK_TEST

static void *do_recv(void *param)
{
	SOCK_t client = *((SOCK_t *)param);
    fd_set rset;
    char linestr[65000];
    struct timeval timeout;
	int ret;
	
	for (;;) {
		FD_ZERO(&rset);
		FD_SET(client, &rset);
		timeout.tv_sec = 1;
		timeout.tv_usec = 0;
		ret = select( client + 1, &rset, NULL, NULL, &timeout);
		if (ret < 0) {
			printf("select error : %d \n" , SOCK_ERR);
			break;
		} else if (ret == 0) {
			printf("select timeout \n");
		} else {
			if ((ret = SOCK_recv(client, linestr, sizeof(linestr), 0)) <= 0) {
				SOCK_close(client);
				break;
			} 
			else {
//				printf("recv %d :\n%s\n", ret, linestr);
				printf(".");
			}
		}
	}
	return NULL;
}

int main(int argc, char *argv[])
{
    int ret;
    SOCK_t sock;
    char linestr[65000];
    char ip[20] = "127.0.0.1";
    unsigned short port =0;
	int size = 128;
    char path[128], path1[128], path2[128];
    fd_set wset, rset;
    struct timeval timeout;
    const char *usage=\
                      "/******************sock module test engin********************/\r\n"
                      "/****************** -tcplisten *****************************/\r\n"
                      "/****************** -tcpsend ******************************/\r\n"
                      "/****************** -unixlisten *****************************/\r\n"
                      "/****************** -unixsend ******************************/\r\n"
                      "/****************** -udplisten *****************************/\r\n"
                      "/****************** -udpsend *****************************/\r\n";
    if(argc < 2) {
        printf(usage);
        return 0;
    }
    if(argc >= 3) {
        if (strstr(argv[1], "unix") != NULL)
            strcpy(path, argv[2]);
        else
            port = atoi(argv[2]);
    }
    if(argc >= 4) {
        strcpy(ip, argv[3]);
    }
	if (argc >= 5) {
		size = atoi(argv[4]);
	}
	memset(linestr, 0, sizeof(linestr));
	memset(linestr, 'a', size);
	
    printf("ip: %s port:%u cmd:%s\n", ip, port, argv[1]);
    if (strcmp(argv[1], "-tcplisten") == 0) {
        sock = SOCK_tcp_listen(ip, port, 8, TRUE);
        if (sock) {
            for (; ; ) {
                FD_ZERO(&rset);
                FD_SET(sock, &rset);
                timeout.tv_sec = 1;
                timeout.tv_usec = 0;
                ret = select( sock + 1, &rset, NULL, NULL, &timeout);
                if (ret < 0) {
                    printf("select error : %d \n" , SOCK_ERR);
                    break;
                } else if (ret == 0) {
                    printf("select timeout \n");
                } else {
                	char szip[32];
					int port;
					pthread_t pid;
                	SOCK_t client = SOCK_accept(sock, szip, &port);
					printf("accept from %s:%d\n", szip, port);
					pthread_create(&pid, NULL, do_recv, &client);
					sleep(2);
                }
            }
        }
    } else if (strcmp(argv[1], "-tcpsend") == 0) {
        sock = SOCK_tcp_connect(ip, port, 2000);
        if (sock > 0) {
           // while(gets(linestr) != NULL) {
           for (;;){
           //     printf("getstr:%s\n", linestr);
           //     if (strcmp(linestr, "quit")  == 0) break;
                SOCK_send(sock, linestr, strlen(linestr), 0);
            }
            printf("getstr failed\n");
        }
    } else if (strcmp(argv[1], "-unixlisten") == 0) {
        sock = SOCK_domain_listen(path);
        if (sock) {
            for (; ; ) {
                FD_ZERO(&rset);
                FD_SET(sock, &rset);
                timeout.tv_sec = 1;
                timeout.tv_usec = 0;
                ret = select( sock + 1, &rset, NULL, NULL, &timeout);
                if (ret < 0) {
                    printf("select error : %d \n" , SOCK_ERR);
                    break;
                } else if (ret == 0) {
                    printf("select timeout \n");
                } else {
                    SOCK_t cfd = SOCK_domain_accept(sock, NULL);
                    if (cfd > 0) {
                        do {
                            if ((ret = SOCK_recv(cfd, linestr, sizeof(linestr), 0)) <= 0) {
                                //break;
                            } else
                                printf("recv %d :\n%s\n", ret, linestr);
                        } while(1 == 1);
                    }
                }
            }
        }
    } else if (strcmp(argv[1], "-unixsend") == 0) {
        sock = SOCK_domain_connect("./testunix.sock", path);
        if (sock > 0) {
            while(gets(linestr) != NULL) {
                printf("getstr:%s\n", linestr);
                if (strcmp(linestr, "quit")  == 0) break;
                if (SOCK_send(sock, linestr, strlen(linestr), 0) == 0)
                    printf("send done!\n");
            }
            printf("getstr failed\n");
        }
    } else if (strcmp(argv[1], "-udplisten") == 0) {
        sock = SOCK_udp_init(NULL, port, TRUE, 5000);
        if (sock) {
            for (; ; ) {
                FD_ZERO(&rset);
                FD_SET(sock, &rset);
                timeout.tv_sec = 1;
                timeout.tv_usec = 0;
                ret = select( sock + 1, &rset, NULL, NULL, &timeout);
                if (ret < 0) {
                    printf("select error : %d \n" , SOCK_ERR);
                    break;
                } else if (ret == 0) {
                    printf("select timeout \n");
                } else {
                    if ((ret = SOCK_recvfrom(sock, NULL, NULL, linestr, sizeof(linestr), 0)) <= 0) {
                        break;
                    }
                    printf("recv %d :\n%s\n", ret, linestr);
                }
            }
        }
    } else if (strcmp(argv[1], "-udpsend") == 0) {
        sock = SOCK_udp_init(NULL, 0, TRUE, 5000);
        if (sock > 0) {
            while(gets(linestr) != NULL) {
                if (strcmp(linestr, "quit")  == 0) break;
                SOCK_sendto(sock, ip, port, linestr, strlen(linestr), 0);
            }
        }
    }

    while(getchar() != 'q') ;
    return 0;
}
#endif


