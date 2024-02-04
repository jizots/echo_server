#include <iostream>
#include <string>
#include <sys/socket.h> /*socket*/
#include <netinet/in.h> /*sockaddr_in*/
#include <arpa/inet.h> /*inet_ntoa*/
#include <unistd.h> /*close*/

# define BUFFER_SIZE 1000

unsigned int	serverPort = 8000;

void	errorExit(const std::string& msg)
{
	perror(msg.c_str());
	std::exit(EXIT_FAILURE);
}

void	HandleTCP(int clientSocket)
{
	char	recivedMsg[BUFFER_SIZE] = {0};
	size_t	recivedMsgSize;

	recivedMsgSize = recv(clientSocket, recivedMsg, BUFFER_SIZE, 0);
	if (recivedMsgSize == -1)
		errorExit("recv() error");

	while (recivedMsgSize > 0)
	{
		if (send(clientSocket, recivedMsg, recivedMsgSize, 0) != recivedMsgSize)
			errorExit("send() error");
		if ((recivedMsgSize = recv(clientSocket, recivedMsg, BUFFER_SIZE, 0)) < 0)
			errorExit("recv() error2");
	}
	close(clientSocket);
}

int main()
{
	int					serverSocket;
	struct sockaddr_in	echoServerAddr;
	socklen_t			clientLen;
	struct sockaddr_in	echoClientaddr;
	int					clientSocket;

	if ((serverSocket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
		errorExit("socket() error: ");

	memset(&echoServerAddr, 0 , sizeof(struct sockaddr_in));
	echoServerAddr.sin_family = AF_INET;
	echoServerAddr.sin_addr.s_addr = INADDR_ANY;
	echoServerAddr.sin_port = htons(serverPort);

	if (bind(serverSocket, (struct sockaddr*) &echoServerAddr, sizeof(struct sockaddr_in)) == -1)
		errorExit("bind() error: ");

	if (listen(serverSocket, SOMAXCONN) == -1)
		errorExit("listen() error: ");

	std::cout << "Server listening" << std::endl;

	while (true)
	{
		clientLen = sizeof(struct sockaddr_in);
		clientSocket = accept(serverSocket, (struct sockaddr*) &echoClientaddr, &clientLen);
		if (clientSocket == -1)
			errorExit("accept errro: ");
		std::cout << "Handling client: " << inet_ntoa(echoClientaddr.sin_addr) << std::endl;
		HandleTCP(clientSocket);
	}
}