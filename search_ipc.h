#ifndef _SEARCH_DVR_H_
#define _SEARCH_DVR_H_

#define HICHIP_MULTICAST_PORT (8002)
#define HICHIP_MULTICAST_IPADDR "239.255.255.251"


#ifdef __cplusplus
extern "C" {
#endif

int SearchIpcam(char *bindIP);

#ifdef __cplusplus
};
#endif

#endif //#ifndef _SEARCH_DVR_H_