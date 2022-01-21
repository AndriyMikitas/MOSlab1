#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <sys/time.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "unix-socket.h"

#define block_size 1048576

long curr_time_s()
{
	return time(NULL);
}

long long curr_time_ms() {
	struct timeval tv;
	gettimeofday(&tv,NULL);
	return (((long long)tv.tv_sec)*1000)+(tv.tv_usec/1000);
}

void print_bandwidth(long bandwidth)
{
	long bandwidth_in_kb = bandwidth / 1024;
	long bandwidth_in_mb = bandwidth_in_kb / 1024;
	printf("bandwidth (send): %lu bytes per second (%lu KB/s, %lu MB/s)\n",
		       	bandwidth, bandwidth_in_kb, bandwidth_in_mb);
}

int main(int argc, char const *argv[])
{
	int send_period = 5;

	if (argc > 1)
		send_period = atoi(argv[1]);

	int sock = 0, invoke_result;
	long bytes_sent = 0, bandwidth;
	long long start_time, end_time;
	struct sockaddr_un serv_addr;
	char data[block_size] = {0}; 
	char finish_data[block_size] = {1}; 
	char ack;

	sock = socket(AF_UNIX, SOCK_STREAM, 0); 
	assert(sock >= 0);

 	memset(&serv_addr, 0, sizeof(struct sockaddr_un));
	serv_addr.sun_family = AF_UNIX; 
	strncpy(serv_addr.sun_path, SV_SOCK_PATH, sizeof(serv_addr.sun_path) - 1);

	start_time = curr_time_ms();
	invoke_result = connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)); 
	end_time = curr_time_ms();
	assert(invoke_result >= 0);
	printf("socket connected, took milliseconds to connect: %lld\n", end_time - start_time);

	long send_until = curr_time_s() + send_period; 
	while(curr_time_s() < send_until)
	{
		invoke_result = send(sock, data, block_size, 0);
		assert(invoke_result >= 0);
		bytes_sent += block_size;
	}

	invoke_result = send(sock, finish_data, block_size, 0);
	assert(invoke_result >= 0);

	recv(sock, &ack, 1, MSG_WAITALL); 

	bandwidth = (bytes_sent / send_period);
	print_bandwidth(bandwidth);

	printf("client exits.\n");
	close(sock);
	
	return 0;
}

