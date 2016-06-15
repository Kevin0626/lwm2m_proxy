/*
 * CUartPort.cpp
 *
 *  Created on: Jan 16, 2016
 *      Author: Jerry Liu
 */

#include "CRadioUartPort.h"
#include <sys/select.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include "packet_process.h"

extern int UART0_Send(int fd, char *send_buf,int data_len);

#define MAX_PACKET_SIZE 1024

CRadioUartPort::CRadioUartPort() {
	// TODO Auto-generated constructor stub
	m_addr_allocated = 0;
}

CRadioUartPort::~CRadioUartPort() {
	// TODO Auto-generated destructor stub
}


node_t* CRadioUartPort::find_node(uint16_t addr)
{
	std::list<node_t*>::iterator it;
	for (it=m_nodes.begin(); it!= m_nodes.end(); it++)
	{
		node_t* node = ((node_t*)(*it));
		if (node->addr ==  addr)
		{
			return node;
		}
	}
	return NULL;
}

node_t* CRadioUartPort::find_node_ep_addr(const char * epname)
{
	std::list<node_t*>::iterator it;
	for (it=m_nodes.begin(); it!= m_nodes.end(); it++)
	{
		node_t* node = ((node_t*)(*it));
		if (strcmp(node->ep_name, epname) == 0)
		{
			return node;
		}
	}

	return NULL;
}

node_t* CRadioUartPort::find_node_socket(int socket_id)
{
	std::list<node_t*>::iterator it;
	for (it=m_nodes.begin(); it!= m_nodes.end(); it++)
	{
		node_t* node = ((node_t*)(*it));
		if (node->socket_id ==  socket_id)
		{
			return node;
		}
	}

	return NULL;
}

void CRadioUartPort::add_node(node_t * node)
{
	m_nodes.push_front(node);
}


node_t *  CRadioUartPort::allocate_address(string & epname)
{

	node_t * node = find_node_ep_addr(epname.c_str());
	if(node)
		return node;

	// max is 255 nodes
	if(m_addr_allocated == 0xFF)
	{
		printf("failed to allocate address as max node address hit\n");
		return NULL;
	}


	node = (node_t*) malloc(sizeof(node_t));
	if(node == NULL)
	{
		printf("failed to alloc node\n");
		return NULL;
	}
	memset(node, 0, sizeof(node_t));
	node->ep_name = (char*) malloc(epname.length() + 1);
	strcpy(node->ep_name, epname.c_str());

	node->addr = ++m_addr_allocated;

	node->socket_id = -1;

	node->uart = this;

	node->created = time(NULL);

	add_node(node);

	printf("allocate a new node. %d, name=%s\n",node->addr, node->ep_name );

	return NULL;

}

void prv_output_buffer(char * buffer,
                              int length)
{
    int i;
    uint8_t array[16];

    i = 0;
    while (i < length)
    {
        int j;
        fprintf(stderr, "  ");

        memcpy(array, buffer+i, 16);

        for (j = 0 ; j < 16 && i+j < length; j++)
        {
            fprintf(stderr, "%02X ", array[j]);
        }
        while (j < 16)
        {
            fprintf(stderr, "   ");
            j++;
        }
        fprintf(stderr, "  ");
        for (j = 0 ; j < 16 && i+j < length; j++)
        {
            if (isprint(array[j]))
                fprintf(stderr, "%c ", array[j]);
            else
                fprintf(stderr, ". ");
        }
        fprintf(stderr, "\n");

        i += 16;
    }
}


char sync_bytes[] = {0xFA, 0xAB};

int CRadioUartPort::transmit_payload(packet_t *packet)
{
	char buffer[256];
	memcpy(buffer, sync_bytes, sizeof(sync_bytes));
	memcpy(buffer+2, (char * )packet, sizeof(packet_t)+ packet->payload_len);
	int len = sizeof(packet_t)+ packet->payload_len+sizeof(sync_bytes);
	//UART0_Send(m_fd, sync_bytes, 2);

	prv_output_buffer(buffer, len);

	WriteUART(m_fd, (uint8_t*)buffer, len);

	return 0;

	//return write(m_fd, packet, sizeof(packet_t)+ packet->payload_len);
}

static char recv_buffer[256];
bool add_packet_byte(char c)
{
	static int sync_offset = 0;
	static int header_offset = 0;
	static int payload_offset = 0;

	if(sync_offset == 0)
	{
		printf("\n%02x ", c);
	}
	else
	{
		printf("%02x ", c);
	}

	if(sync_offset != sizeof(sync_bytes)){
		recv_buffer[sync_offset] = c;
		sync_offset ++;

		while(sync_offset> 0 && memcmp(recv_buffer, sync_bytes, sync_offset) != 0)
		{
			sync_offset --;
			for(int i=0;i<sync_offset;i++)
				recv_buffer[i] = recv_buffer[i+1];

			printf("invalid sync code: %02x\n", c);
		}

		return false;
	}

	packet_t *packet = (packet_t *) recv_buffer;

	if(header_offset != sizeof(packet_t))
	{
		recv_buffer[header_offset] = c;
		header_offset ++;

		if(header_offset == sizeof(packet_t))
		{
			if(packet->payload_len > 250)
			{
				sync_offset = 0;
				header_offset = 0;
			}
		}

		return false;
	}

	if(payload_offset != packet->payload_len)
	{
		packet->payload[payload_offset] = c;
		payload_offset ++;
	}

	if(payload_offset == packet->payload_len)
	{
		payload_offset = 0;
		sync_offset = 0;
		header_offset = 0;

		printf("\none complete packet received\n");

		return true;
	}

	return false;
}



void CRadioUartPort::handle_passthrough_up(packet_t *packet)
{
	if(packet->dest != 0)
	{
		printf("dest is not zero. %d\n", packet->dest);
		return;
	}

	if(packet->type == T_Addr_Req)
	{
		printf("recieve address reqest\n");

		std::string ep_name((char*)packet->payload, packet->payload_len);
		node_t * node = allocate_address(ep_name);
		int addr = 0;
		if(node != NULL)
			addr = node->addr;

		char buffer[100];
		packet_t * out = (packet_t*) buffer;

		memset(out, 0, sizeof(*out));
		out->type = T_Addr_Resp;
		out->src = 0;
		out->dest = 0xFF;
		sprintf((char*)out->payload,"%s,%d", ep_name.c_str(), addr);
		out->payload_len = strlen((char*)out->payload) + 1;
		transmit_payload(out);
		return;
	}

	if(packet->type  != T_Lwm2m)
	{
		return;
	}

	uint16_t addr = packet->src;
	node_t* node = find_node(addr);

	if(node == NULL)
	{
		char buffer[100];
		packet_t * out = (packet_t*) buffer;

		memset(out, 0, sizeof(*out));
		out->type = T_Invalid_Addr;
		out->src = 0;
		out->dest = addr;
		out->payload_len = 0;
		transmit_payload(out);

		printf("fail to find node for addr %x\n", addr);
		return;
	}

	node->last_up_msg_time = time(NULL);

	if(node->socket_id == -1)
	{
		node->socket_id = create_socket();

		if(node->socket_id == -1)
			return;

		node->is_passthrough = 1;
	}

	if(g_sl == 0){
		get_server_addr();
	}

	if(g_sl == 0)
	{
		return;
	}

	sendto(node->socket_id, packet->payload, packet->payload_len, 0, (struct sockaddr *)&(g_addr), g_sl);

}

int CRadioUartPort::set_fd_passthrough(fd_set &readfds)
{
	int cnt = 1;
    FD_ZERO(&readfds);

    // add the serial port first
    FD_SET(m_fd, &readfds);

	std::list<node_t*>::iterator it;
	for (it=m_nodes.begin(); it!= m_nodes.end(); it++)
	{
		node_t* node = ((node_t*)(*it));
		if (node->is_passthrough && node->socket_id != -1)
		{
			FD_SET(node->socket_id, &readfds);
			cnt ++;
		}
	}

	return cnt;
}



void CRadioUartPort::handle_passthrough_down(fd_set & readfds)
{
    uint8_t buffer[MAX_PACKET_SIZE];
    int numBytes;
 	int cnt = 0;
 	packet_t *packet = (packet_t *) buffer;

 	std::list<node_t*>::iterator it;
 	for (it=m_nodes.begin(); it!= m_nodes.end(); it++)
 	{
 		node_t* node = ((node_t*)(*it));
 		if (node->is_passthrough && node->socket_id != -1
 				&& FD_ISSET(node->socket_id, &readfds))
 		{
  			cnt ++;

			 struct sockaddr_storage addr;
			 socklen_t addrLen;

			 addrLen = sizeof(addr);
			 numBytes = recvfrom(node->socket_id, packet->payload,
					 MAX_PACKET_SIZE -  sizeof(packet_t), 0,
					 (struct sockaddr *)&addr, &addrLen);

			 if (0 > numBytes)
			 {
				 fprintf(stderr, "Error in recvfrom(): %d\r\n", errno);
			 }
			 else if (0 < numBytes)
			 {
				 char s[INET6_ADDRSTRLEN];
				 int port;

				 if (AF_INET == addr.ss_family)
				 {
					 struct sockaddr_in *saddr = (struct sockaddr_in *)&addr;
					 inet_ntop(saddr->sin_family, &saddr->sin_addr, s, INET6_ADDRSTRLEN);
					 port = saddr->sin_port;
				 }
				 else if (AF_INET6 == addr.ss_family)
				 {
					 struct sockaddr_in6 *saddr = (struct sockaddr_in6 *)&addr;
					 inet_ntop(saddr->sin6_family, &saddr->sin6_addr, s, INET6_ADDRSTRLEN);
					 port = saddr->sin6_port;
				 }

				memset(packet, 0, sizeof(*packet));
				packet->type = T_Lwm2m;
				packet->src = 0;
				packet->dest = node->addr;
				packet->payload_len = numBytes;
				transmit_payload(packet);

				node->last_down_msg_time = time(NULL);
  		     }
 		}
 	}
}


void CRadioUartPort::process()
{
    struct timeval tv;
    fd_set readfds;
    int result;

    while (1)
    {
    	if(0 == set_fd_passthrough(readfds))
    	{
    		sleep(1);
    		continue;
    	}

		tv.tv_usec = 0;
		tv.tv_sec = 2;

		result = select(FD_SETSIZE, &readfds, NULL, NULL, &tv);

        if (result < 0)
        {
            if (errno != EINTR)
            {
              fprintf(stderr, "Error in select(): %d\r\n", errno);
            }
        }
        else if (result > 0)
        {
        	if(FD_ISSET(m_fd, &readfds))
        	{
        		char c[256];
        		int n = read(m_fd, c,256);

        		char * p = c;

        		while(n > 0)
        		{
        			if(add_packet_byte(*p))
        			{
        				handle_passthrough_up((packet_t *) recv_buffer);
        			}
        			p++;
        			n--;
        		}
        	}
        	else
        	{
        		handle_passthrough_down(readfds);
        	}
        }
    }

}
