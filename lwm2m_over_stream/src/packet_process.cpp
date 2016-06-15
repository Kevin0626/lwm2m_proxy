/*
 * packet_process.cpp
 *
 *  Created on: Jan 16, 2016
 *      Author: Jerry Liu
 */


#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include "packet_process.h"
#include <sys/select.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>

#include "CRadioUartPort.h"



socklen_t g_sl = 0;
struct sockaddr g_addr;

int get_server_addr()
{
    struct addrinfo hints;
    struct addrinfo *servinfo = NULL;
    struct addrinfo *p;
    int s;
    struct sockaddr *sa;
    socklen_t sl;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;

    if (0 != getaddrinfo("localhost", "5683", &hints, &servinfo) || servinfo == NULL)
    {
    	WARNING("passthrough:fail to get server addr: %d", errno);
    	return 0;
    }

    // we test the various addresses
    s = -1;
    for(p = servinfo ; p != NULL && s == -1 ; p = p->ai_next)
    {
        s = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (s >= 0)
        {
            sa = p->ai_addr;
            sl = p->ai_addrlen;
            if (-1 == connect(s, p->ai_addr, p->ai_addrlen))
            {
            	WARNING("passthrough: connect return -1. sock=%d, err=%d", s, errno);
                close(s);
                s = -1;
            }
        }
    }
    if (s >= 0)
    {
    	memcpy(&g_addr,sa, sl);
    	g_sl = sl;
    	close(s);

    	WARNING("passthrough: got the lite server addr");
    }

    if (NULL != servinfo) {
        free(servinfo);
    }

    return s;
}



int create_socket()
{
    int s = -1;
    struct addrinfo hints;
    struct addrinfo *res;
    struct addrinfo *p;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;

    //if (0 != getaddrinfo(NULL, portStr, &hints, &res))
    if (0 != getaddrinfo("127.0.0.1", NULL, &hints, &res))
    {
        return -1;
    }

    for(p = res ; p != NULL && s == -1 ; p = p->ai_next)
    {
        s = socket(p->ai_family, p->ai_socktype | SOCK_CLOEXEC, p->ai_protocol);
        if (s >= 0)
        {
            if (-1 == bind(s, p->ai_addr, p->ai_addrlen))
            {
                close(s);
                s = -1;
            }
			break;
			printf( "create_socket:bind success, the port is ");
        }
    }

    freeaddrinfo(res);

    return s;
}


void handle_address_request(packet_t *)
{

}

void *PassthroughTask(void *argument)
{
	CRadioUartPort * port = (CRadioUartPort *) argument;
	uint8_t addr = 0;
	while(addr < 5)
	{
		addr++;

		//send_addr(addr, port->m_fd);

		//sleep(1);
	}

	printf("start process....\n\n");
	port->process();

	return NULL;
}

