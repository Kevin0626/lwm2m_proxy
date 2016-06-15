//============================================================================
// Name        : lwm2m_on_radio.cpp
// Author      : Jerry Liu
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

int main() {
	cout << "!!!Hello World!!!. packet header size is " << sizeof(packet_t) << endl; // prints !!!Hello World!!!

	int fd = OpenUART("/dev/ttyUSB1");
	if(fd == -1)
		return -1;

	//UART0_Init(fd);

	uint8_t addr = 0;
	while(addr < 5)
	{
		addr++;

		//send_addr(addr, fd);

		//sleep(1);
	}

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
