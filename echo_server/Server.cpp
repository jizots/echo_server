#include <iostream>
#include <string>
#include <sys/socket.h> /*socket*/
#include <netinet/in.h> /*sockaddr_in*/
#include <arpa/inet.h> /*inet_ntoa*/
#include <unistd.h> /*close*/

// クライアントから受信できるリクエスト文字数
# define BUFFER_SIZE 10

// 外部からの通信を受け付けるポートを8000に指定
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
	static int	count = 1;

	/* 
	 * まずクライアントからのメッセージを1回うけとる。
	 * ただし送信されたメッセージは、バッファーサイズの１０を超える可能性がある。
	 * その場合は次のループで解説する。
	*/
	recivedMsgSize = recv(clientSocket, recivedMsg, BUFFER_SIZE, 0);
	if (recivedMsgSize == -1)
		errorExit("recv() error");

	/*
	 * メッセージを読み込み、１文字以上あれば以下のループに入る。
	 * sendでは、クライアントに、先のrecvで読み取ったメッセージを送り返している。
	*/
	while (recivedMsgSize > 0)
	{
		if (send(clientSocket, recivedMsg, recivedMsgSize, 0) != recivedMsgSize)
			errorExit("send() error");
		/*
		 * 次のrecvは、ソケットの受信バッファー内に読み取るべきメッセージが残っているかを確認している。
		 * もし受信バッファーにデータが横っていた場合は、whileループが継続する。
		*/
		if ((recivedMsgSize = recv(clientSocket, recivedMsg, BUFFER_SIZE, 0)) < 0)
			errorExit("recv() error2");
	}
	close(clientSocket);
}

int main()
{
	int					serverSocket;
	struct sockaddr_in	echoServerAddr; /*bind()のために必要*/
	socklen_t			clientLen;
	struct sockaddr_in	echoClientaddr;
	int					clientSocket;

	/*
	 * 外部からの通信を受け入れるための、ソケット(ファイルディスクリプタの一種)を作成する。
	 * socketの第１引数はネットワークの設定で、AF-INETはIPv4の通信規格を指す。
	 * 第２引数は通信ストリームの規格で、SOCK-STREAMはTCP/IPでの通信規格を指す。
	*/
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