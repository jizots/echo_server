#include <iostream>
#include <string>
#include <sys/socket.h> /*socket*/
#include <netinet/in.h> /*sockaddr_in*/
#include <arpa/inet.h> /*inet_ntoa*/
#include <unistd.h> /*close*/

# define BUFFER_SIZE 1000

unsigned int    serverPort = 8000;

void	errorExit(const std::string& msg)
{
	perror(msg.c_str());
	std::exit(EXIT_FAILURE);
}

int main(int ac, char **av)
{
	int					_socket;
	struct sockaddr_in	echoServerAddr;
	char				*serverIP;
	char				*echoString;
	char				echoBuffer[BUFFER_SIZE] = {0};
	unsigned int		echoStringLen;
	int					recvBytes;
	int					totalRecvBytes;

	serverIP = av[1];
	echoString = av[2];

	if ((_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		errorExit("client socket() error");
	memset(&echoServerAddr, 0, sizeof(struct sockaddr_in));
	echoServerAddr.sin_family = AF_INET;
	echoServerAddr.sin_addr.s_addr = inet_addr(serverIP);
	echoServerAddr.sin_port = htons(serverPort);

	// connect echo server
	if (connect(_socket, (struct sockaddr *) &echoServerAddr, sizeof(echoServerAddr)) < 0)
		errorExit("connect() failed");

	echoStringLen = strlen(echoString);

	// send message to echo server
	if (send(_socket, echoString, echoStringLen, 0) != echoStringLen)
		errorExit("send() failed");

	// recieve message from echo server
	totalRecvBytes = 0;
	std::cout << "Recieved: ";
	while (totalRecvBytes < echoStringLen)
	{
		if ((recvBytes = recv(_socket, echoBuffer, BUFFER_SIZE - 1, 0)) <= 0)
			errorExit("recv() failed");
		totalRecvBytes += recvBytes;
		echoBuffer[recvBytes] = '\0';
		std::cout << echoBuffer;
	}
	std::cout << std::endl;

	// close socket
	close(_socket);
	exit(0);
}