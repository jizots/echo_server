#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <errno.h>
#include <chrono>
#include <thread>

int	prepaireListeningSocket(int *server_fd, struct sockaddr_in *address)
{
	int opt = 1;

	// サーバのリスニングソケットの作成
	*server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (*server_fd == 0)
	{
		perror("socket failed");
		return -1;
	}
	// リスニングソケットのアドレスの設定
	(*address).sin_family = AF_INET;
	(*address).sin_addr.s_addr = INADDR_ANY;
	(*address).sin_port = htons(8080);
	// リスニングソケットをポート8080にバインド
	if (bind(*server_fd, (struct sockaddr *)&(*address), sizeof(*address))<0)
	{
		perror("bind failed");
		return -1;
	}
	return 0;
}

void	waitClientConnection(int new_socket)
{
	char buffer[1024] = {0};

	// ループの終了条件
	bool waitingForData = true;
	auto startTime = std::chrono::system_clock::now();
	// クライアントからの接続を受け入れる
	while(waitingForData)
	{
		// データの非同期受信を試みる。オプションを0にするとifに入らない。
		ssize_t bytes_received = recv(new_socket, buffer, 1024, MSG_DONTWAIT);
		if (bytes_received < 0)
		{
			/*
			 ソケットが非停止に設定されていて受信操作が停止するような状況になったか、
			 受信に時間切れ (timeout) が設定されていてデータを受信する前に時間切れになった。
			 ちなみに、webservの場合はここでerrnoをチェックするとサブジェクト違反になる。
			*/
			if (errno == EAGAIN || errno == EWOULDBLOCK) 
			{
				// データがまだ利用可能でない場合の処理
				auto currentTime = std::chrono::system_clock::now();
				std::chrono::duration<double> elapsedSeconds = currentTime - startTime;
				if (elapsedSeconds.count() >= 10.0)
				{ // 5秒経過したらループを抜ける
					waitingForData = false;
					std::cout << "No data received for 10 seconds, exiting." << std::endl;
				}
				else
				{
					// 他のタスク（ここでは現在時刻の表示）を実行
					std::this_thread::sleep_for(std::chrono::seconds(1)); // 1秒ごとに時刻を表示
					std::time_t now = std::chrono::system_clock::to_time_t(currentTime);
					std::cout << "Waiting for data... " << std::ctime(&now);
				}
			}
			else
			{
				perror("recv");
				waitingForData = false;
			}
		}
		else if (bytes_received == 0) /*ページをリロードするとここに入る。ブラウザが接続終了(EOF, FIN_packet）を送信しているのであろう*/
		{
			std::cout << "Connection closed by client." << std::endl;
			waitingForData = false;
		}
		else //localhost:8080にアクセスするとここに入る
		{
			std::cout << "Received data: " << std::string(buffer, bytes_received) << std::endl;
		}
	}
	// ソケットのクローズ
	close(new_socket);
}

int main() 
{
	int server_fd, new_socket;
	struct sockaddr_in address;
	unsigned int addrlen = sizeof(address);

	// サーバのリスニングソケットの作成
	if (prepaireListeningSocket( &server_fd, &address) < 0)
		return -1;

	// リスン開始
	if (listen(server_fd, 3) < 0) {
		perror("listen");
		return -1;
	}

	std::cout << "Server is listening on port 8080..." << std::endl;

	// クライアントからの接続を受け入れる
    new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
    if (new_socket < 0) {
        perror("accept");
        return -1;
    }

	// クライアントからの接続を待つ
	waitClientConnection(new_socket);

	// ソケットのクローズ
	close(server_fd);
	return 0;
}
