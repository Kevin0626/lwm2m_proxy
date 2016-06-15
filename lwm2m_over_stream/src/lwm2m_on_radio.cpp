//============================================================================
// Name        : lwm2m_on_radio.cpp
// Author      : 
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C++, Ansi-style
//============================================================================

#include <iostream>
#include <stdlib.h>
#include <stdio.h>

#include "packet_process.h"
#include "CRadioUartPort.h"

using namespace std;
std::list<CRadioUartPort*> g_ports;

extern int UART0_Open(char* port);
extern void *PassthroughTask(void *argument);
extern int UART0_Init(int fd);
extern int UART0_Send(int fd, char *send_buf,int data_len);
extern int32_t rpcTransportOpen(char *_devicePath, uint32_t port);

extern char sync_bytes[2];

const char * server = "[::1]";
const char * serverPort = "5683";


void send_addr(int addr, int fd)
{

	char buffer[100];
	memcpy(buffer, sync_bytes, sizeof(sync_bytes));
	packet_t * out = (packet_t*) (buffer+2);

	memset(out, 0, sizeof(*out));
	out->type = 200;
	out->src = 0;
	out->dest = 0xFF;
	sprintf((char*)out->payload,"%d", addr);
	out->payload_len = strlen((char*)out->payload) + 1;
	WriteUART(fd, (uint8_t *)buffer, sizeof(packet_t)+ out->payload_len+sizeof(sync_bytes));
}


void print_usage(void)
{
    fprintf(stdout, "Options:\r\n");
    fprintf(stdout, "  -u UART\tSet the UART port name of radio module connection. Default: /dev/ttyUSB0\r\n");
    //fprintf(stdout, "  -l PORT\tSet the local UDP port of the Client. Default: 56830\r\n");
    fprintf(stdout, "  -h HOST\tSet the hostname of the LWM2M Server to connect to. Default: localhost\r\n");
    fprintf(stdout, "  -p PORT\tSet the port of the LWM2M Server to connect to. Default: 5683\r\n");
    //fprintf(stdout, "  -c\t\tChange battery level over time.\r\n");
    fprintf(stdout, "\r\n");
}


int main(int argc, char *argv[])
{
    int opt;
    char * uart ="/dev/ttyUSB0";


	cout << "Start LWM2M on Radio Agent. Packet header size is " << sizeof(packet_t) << endl; // prints !!!Hello World!!!
    while ((opt = getopt(argc, argv, "bu:p:s:")) != -1)
    {
        switch (opt)
        {
        case 'b':
            //bootstrapRequested = true;
            break;
        case 'u':
        	uart = optarg;
            break;
        case 's':
            server = optarg;
            break;
        case 'p':
            serverPort = optarg;
            break;
        default:
            print_usage();
            return 0;
        }
    }


	int fd = OpenUART(uart);
	if(fd == -1)
		return -1;


	CRadioUartPort * port = new CRadioUartPort();
	port->m_fd = fd;
	port->m_name = "first";

	g_ports.push_front(port);

	pthread_t thread;

	pthread_create(&thread, NULL, PassthroughTask, (void *) port);

	while(1)
	{
		sleep(1000);
	}

	return 0;
}
