/*
 * CUartPort.h
 *
 *  Created on: Jan 16, 2016
 *      Author: Jerry Liu
 */

#ifndef CRADIOUARTPORT_H_
#define CRADIOUARTPORT_H_

#include <list>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <string>
#include "packet_process.h"


using  std::string;

class CRadioUartPort;

typedef struct
{
	uint16_t addr;
	char * ep_name;
	uint8_t channel;
	uint8_t is_passthrough;
	int socket_id;
	CRadioUartPort * uart;
	time_t created;
	time_t last_up_msg_time;
	time_t last_down_msg_time;
} node_t;


class CRadioUartPort {
public:
	CRadioUartPort();
	virtual ~CRadioUartPort();

public:
	int  m_fd;
	std::string m_name;
	std::list<node_t*> m_nodes;
	int m_addr_allocated;

	node_t* find_node(uint16_t addr);
	node_t* find_node_ep_addr(const char * epname);
	node_t* find_node_socket(int socket_id);
	void add_node(node_t * node);
	int set_fd_passthrough(fd_set &readfds);

	void handle_passthrough_up(packet_t *packet);
	void handle_passthrough_down(fd_set & readfds);

	node_t * allocate_address(string & epname);

	int transmit_payload(packet_t *packet);


	void process();

};

#endif /* CRADIOUARTPORT_H_ */
