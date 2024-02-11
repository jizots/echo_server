#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>

int PORT[3] = {8001, 8002, 8003};

#define MAX_CLIENTS 10

void	createMasterSocket(int *server_fd, struct sockaddr_in *address)
{
	// Create a Multiple master socket
	for (int j = 0; j < 3; ++j)
	{
		if ((server_fd[j] = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
			perror("socket failed");
			exit(EXIT_FAILURE);
		}

		// Type of socket created
		address[j].sin_family = AF_INET;
		address[j].sin_addr.s_addr = INADDR_ANY;
		address[j].sin_port = htons(PORT[j]);

		// Bind the socket to localhost port
		if (bind(server_fd[j], (struct sockaddr *)&address[j], sizeof(address[j])) < 0) {
			perror("bind failed");
			exit(EXIT_FAILURE);
		}
		printf("Listener on port %d(%d)\n", PORT[j], ntohs(address[j].sin_port));

		// Try to specify maximum of 10 pending connections for the master socket
		if (listen(server_fd[j], 10) < 0) {
			perror("listen");
			exit(EXIT_FAILURE);
		}
	}
}

bool isNewConnection(int *server_fd, fd_set* readfds, struct sockaddr_in *address, int *client_socket) {
    int new_socket;
    struct sockaddr_in temp_address; // 新しい接続のアドレス情報を格納するための一時的な変数
    int addrlen = sizeof(temp_address);

    for (int j = 0; j < 3; ++j) {
        if (FD_ISSET(server_fd[j], readfds)) {
            if ((new_socket = accept(server_fd[j], (struct sockaddr *)&temp_address, (socklen_t*)&addrlen)) < 0) {
                perror("accept");
                exit(EXIT_FAILURE);
            }

            // Inform user of socket number - used in send and receive commands
            std::cout 
                << "New connection to serverFd " << server_fd[j] << ", socket fd is " << new_socket 
                << ", ip is : " << inet_ntoa(temp_address.sin_addr)
                << ", port : " << ntohs(temp_address.sin_port)
                << std::endl;

            // Add new socket to array of sockets
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (client_socket[i] == 0) {
                    client_socket[i] = new_socket;
                    printf("Adding to list of sockets as index:%d, socketFd:%d\n", i, client_socket[i]);
                    return true;
                }
            }
        }
    }
    return false;
}

bool	isReadable(int *client_socket, fd_set *readfds)
{
	int	valread;
	char buffer[1025];  // 1KB buffer for incoming messages

	for (int i = 0; i < MAX_CLIENTS; i++) {
		if (FD_ISSET(client_socket[i], readfds)) {
			// Check if it was for closing, and also read the incoming message
			if ((valread = read(client_socket[i], buffer, 1024)) == 0) {
				// Somebody disconnected, get his details and print
				// getpeername(client_socket[i], (struct sockaddr*)&address, (socklen_t*)&addrlen);
				printf("Host disconnected, fd %d, port can't detect \n", client_socket[i]); // port can't detect

				// Close the socket and mark as 0 in list for reuse
				close(client_socket[i]);
				client_socket[i] = 0;
			} else {
				// Echo back the message that came in
				buffer[valread] = 0;
				std::cout << "buffer: " << buffer << std::endl;
				const char* response = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<html><body><h1>Hello, WebBrouser!</h1></body></html>\r";
				write(client_socket[i], response, strlen(response));
				printf("sent message\n");

				// // ↓もし接続を維持するならこの二行はいらない
				// close(client_socket[i]);
				// client_socket[i] = 0;
			}
			std::cout << "Break becase request" << std::endl;
			return (true);
		}
	}
	return (false);
}

int main(int ac, char*av[]) {
	int server_fd[3], client_socket[MAX_CLIENTS], activity;
	int max_sd;
	struct sockaddr_in address[3];
	int opt = 1;
	int addrlen = sizeof(address[0]);

	// Set of socket descriptors
	fd_set readfds;

	// Create a Multiple master socket
	createMasterSocket(server_fd, address);

	// Accept incoming connections
	puts("Waiting for connections ...");

	// Initialize all client_socket[] to 0 so not checked
	for (int i = 0; i < MAX_CLIENTS; i++) {
		client_socket[i] = 0;
	}

	while (true) {
		std::cout << "Go select loop" << std::endl;
		// Clear the socket set
		FD_ZERO(&readfds);

		// Add master socket to set
		for (int j = 0; j < 3; ++j)
		{
			FD_SET(server_fd[j], &readfds);
			max_sd = server_fd[j];
		}

		// Add child sockets to set
		for (int i = 0; i < MAX_CLIENTS; i++) {
			// If valid socket descriptor then add to read list
			if (client_socket[i] > 0)
				FD_SET(client_socket[i], &readfds);

			// Highest file descriptor number, need it for the select function
			if (client_socket[i] > max_sd)
				max_sd = client_socket[i];
		}

		// Wait for an activity on one of the sockets, timeout is NULL, so wait indefinitely
		activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);

		if ((activity < 0) && (errno != EINTR)) {
			printf("select error");
		}
		std::cout << "selected" << std::endl; 

		// If something happened on the master socket, then it's an incoming connection
		if (isNewConnection(server_fd, &readfds, address, client_socket))
		{
			std::cout << "Continue becase accept" << std::endl;
			continue;
		}

		// Else it's some IO operation on some other socket
		isReadable(client_socket, &readfds);
	}
	return 0;
}