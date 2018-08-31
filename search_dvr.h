#ifndef _SEARCH_DVR_H_
#define _SEARCH_DVR_H_


#define BUBBLE_PORT (9013)
#define BUBBLE_ADDR "255.255.255.255"
#define BUBBLE_SEARCH_STRING "SEARCHDEV"
#define BUBBLE_SEARCH_ACK_STRING "JA"

#define HICHIP_MULTICAST_PORT (8002)
#define HICHIP_MULTICAST_IPADDR "239.255.255.251"


#ifdef __cplusplus
extern "C" {
#endif

/*！
 *	@file 搜索DVR功能的头文件             
 *	@author 	
 *	@brief获取指定网口的本地IP地址，字符串形式返回，如：192.168.1.111
 *	@param netcard 网卡标识，@param ip返回的IP
 *	@return 获取成功返回0，失败返回－1；
*/
int get_netcard_ip(char *netcard , char *ip);

/*！
 *	@file 搜索DVR功能的头文件             
 *	@author 	
 *	@brief获取指定网口的MTU
 *	@param netcard 网卡标识
 *	@return 获取成功返回mtu，失败返回－1；
*/

int get_netcard_mtu(char *netcard);

/*！
 *	@file 搜索DVR功能的头文件             
 *	@author 	
 *	@brief执行搜索DVR的操作（支持BUBBLE搜索协议的都会搜索到）
 *	@param bindIP 需要绑定的IP，默认为空就可以
 *	@return 获取成功返回0，失败返回－1；
*/

int SearchDVR(char *bindIP);

int Modify_IPCAM_IP();

#ifdef __cplusplus
};
#endif

#endif //#ifndef _SEARCH_DVR_H_