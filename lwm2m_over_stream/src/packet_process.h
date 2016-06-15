/*
 * packet_process.h
 *
 *  Created on: Jan 16, 2016
 *      Author: Jerry Liu
 */

#ifndef PACKET_PROCESS_H_
#define PACKET_PROCESS_H_
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <ctype.h>
#include <list>
#include <sys/socket.h>

#define WARNING printf


typedef struct
{
	uint8_t type;
	uint8_t payload_len;
	uint8_t dest;
	uint8_t src;
	uint16_t token;
	uint8_t payload[0];
} __attribute__((__packed__)) packet_t;

enum
{
	T_Beacon = 0,
	T_Lwm2m = 1,
	T_Addr_Req,
	T_Addr_Resp,
	T_Addr_Release,
	T_Invalid_Addr
};


extern socklen_t g_sl;
extern struct sockaddr g_addr;
extern void send_addr(int addr, int);
extern int get_server_addr();
extern int create_socket();
extern void *PassthroughTask(void *argument);
extern int32_t OpenUART(char *devicePath);
extern void CloseUART(int serialPortFd);
extern void WriteUART(int serialPortFd, uint8_t* buf, uint8_t len);



#endif /* PACKET_PROCESS_H_ */
