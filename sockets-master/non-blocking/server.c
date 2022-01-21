#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>
#include <sys/types.h>
#include <time.h>
#include <sys/time.h>

#define PORT 8080
#define MAX_CONNECTIONS 1024
#define block_size 1048576

#define SERVER_UPTIME_SEC 60
#define TIMEOUT_SEC 2

struct sockaddr_in address;
int addrlen = sizeof(address);
long bytes_received = 0;
char ack;

long curr_time_s()
{
        return time(NULL);
}

long long curr_time_ms() {
	struct timeval tv;
	gettimeofday(&tv,NULL);
	return (((long long)tv.tv_sec)*1000)+(tv.tv_usec/1000);
}

int read_client(int client)
{
	char data[block_size];
	long bytes_received_client = 0;
	printf("receiving some data from client %d...\n", client);

	int read_result = read(client, data, block_size);
	assert(read_result >= 0);

	if (data[0] == 1)
		return -1;
	
	bytes_received_client += block_size;

	bytes_received += bytes_received_client;
	printf("received %lu bytes from the client %d (%lu overall).\n", bytes_received_client, client, bytes_received);
	return 0;
}

void handle_client_disconnect(int clients[], int index, int *n_clients)
{
	(*n_clients)--;

	for (int i = index; i < *n_clients - 1; ++i)
		clients[i] = clients[i + 1];
}

void perform_select(int listening_socket, int clients[], int *n_clients)
{
	fd_set sock_set;
	struct timeval select_timeout;
	int i;
	int select_result;

	FD_ZERO(&sock_set);
	FD_SET(listening_socket, &sock_set);
	for (i = 0; i < *n_clients; ++i)
		FD_SET(clients[i], &sock_set);

	select_timeout.tv_sec = TIMEOUT_SEC;
	select_timeout.tv_usec = 0;

	printf("checking for socket events...\n");
	select_result = select(FD_SETSIZE, &sock_set, NULL, NULL, &select_timeout);
	assert(select_result >= 0);
	
	if (select_result == 0)
	{
		printf("no external events occured, proceeding...\n");
	}

	if (FD_ISSET(listening_socket, &sock_set) && (*n_clients) < MAX_CONNECTIONS)
	{	
                int new_client = accept(listening_socket, (struct sockaddr *)&address, (socklen_t*)&addrlen);
		assert(new_client >= 0);
		printf("client connected.\n");
		
		clients[*n_clients] = new_client;
		(*n_clients)++;
	}

	for (i = 0; i < *n_clients; ++i)
	{
		if (FD_ISSET(clients[i], &sock_set) && read_client(clients[i]) < 0)		
		{
			handle_client_disconnect(clients, i, n_clients);
			send(clients[i], &ack, 1, 0);
			printf("client %d disconnected.\n", i);
		}
	}
}

int main(int argc, char const *argv[])
{
	int server_fd, invoke_result, opt = 1;
	
	int clients[MAX_CONNECTIONS] = {0};
	int n_clients = 0;

	server_fd = socket(AF_INET, SOCK_STREAM, 0); 
	assert(server_fd != 0);

        invoke_result = setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));
        assert(invoke_result == 0);
	
	address.sin_family = AF_INET; 
	address.sin_port = htons(PORT);
	address.sin_addr.s_addr = INADDR_ANY;

	invoke_result = bind(server_fd, (struct sockaddr *)&address, sizeof(address));
	assert(invoke_result >= 0);

	invoke_result = listen(server_fd, 3);
	assert(invoke_result >= 0);
	
	long up_until = curr_time_s() + SERVER_UPTIME_SEC;
	while (curr_time_s() < up_until)
	{
		perform_select(server_fd, clients, &n_clients);
	}

	printf("disconnecting all clients...\n");
	for (int j = 0; j < n_clients; ++j)
	{
		send(clients[j], &ack, 1, 0);
		close(clients[j]);
	}

	printf("clients disconnected.\n");
	printf("server exits\n");
	close(server_fd);
	
	return 0;
}
