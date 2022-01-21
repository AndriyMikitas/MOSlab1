#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <assert.h>
#include <sys/time.h>
#include <stdbool.h>

#define PORT 8080
#define block_size 1048576


long long curr_time_ms() {
	struct timeval tv;
	gettimeofday(&tv,NULL);
	return (((long long)tv.tv_sec)*1000)+(tv.tv_usec/1000);
}

int main(int argc, char const *argv[])
{
	int server_fd, new_socket, invoke_result, opt = 1;
	struct sockaddr_in address;
	long long start_time, end_time;
	int addrlen = sizeof(address);
	char data[block_size]; 
	char ack;

	server_fd = socket(AF_INET, SOCK_STREAM, 0); 
	assert(server_fd != 0);

	invoke_result = setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));
	assert(invoke_result == 0);

	address.sin_family = AF_INET; 
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(PORT);

	start_time = curr_time_ms();
	invoke_result = bind(server_fd, (struct sockaddr *)&address, sizeof(address));
	end_time = curr_time_ms();
	assert(invoke_result >= 0);
	printf("socket binded, took milliseconds to bind: %lld\n", end_time - start_time);

	start_time = curr_time_ms();
	invoke_result = listen(server_fd, 3);
	end_time = curr_time_ms();
	assert(invoke_result >= 0);
	printf("server started listening, took milliseconds to start: %lld\n", end_time - start_time);
	
	bool accept_new = 1;
	while (accept_new)
	{
		printf("waiting for new clients to connect...\n");
	
		new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
		assert(new_socket >= 0);
		printf("client connected. Creating a new thread...\n");
			
		int pid = fork();
		if (pid <= 0) // child
		{
			printf("receiving some data from client...\n");
			long bytes_received_client = 0;
			while (1) 
			{
				invoke_result = read(new_socket, data, block_size);
				assert(invoke_result >= 0);
	
				if (data[0] == 1)
					break;
	
				bytes_received_client += block_size;
			}
		
			printf("received %lu bytes from the client.\n", bytes_received_client);

			send(new_socket, &ack, 1, 0);
			close(new_socket);
			printf("communication with the client ended.\n");

			return 0;
		}
		else // parent
		{
			char c_flag;
			printf("continue listening? (y/n): ");
			scanf(" %c",&c_flag);

			if (c_flag != 'y')
				accept_new = 0;
		}
	}

	printf("server exits\n");
	close(server_fd);
	
	return 0;
}
