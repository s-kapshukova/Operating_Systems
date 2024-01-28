#include <iostream>
#include <signal.h>
#include <sys/types.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#define PORT 8888
#define MAX_CLIENTS 2
volatile sig_atomic_t wasSigHup = 0;

typedef struct {
    int connfd;
    struct sockaddr_in addr;
} client_t;

void sigHupHandler(int r)
{
	wasSigHup = 1;
}
void setupSignalHandling(sigset_t* origMask) 
{
	struct sigaction sa;
	sigaction(SIGHUP, NULL, &sa);
	sa.sa_handler = sigHupHandler;
	sa.sa_flags |= SA_RESTART;
	sigaction(SIGHUP, &sa, NULL);
	
	sigset_t blockedMask;
	sigemptyset(&blockedMask);
	sigaddset(&blockedMask, SIGHUP);
	sigprocmask(SIG_BLOCK, &blockedMask, origMask);
}
int main() 
{
    client_t clients[3];
    int active_clients = 0;
    //создаём сервер
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) 
    {
        std::cout << "Error creating socket" << std::endl;
        return 1;
    }
    
    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);
    
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) 
    {
        std::cout << "Bind failed" << std::endl;
        return 1;
    }
    
    if (listen(server_fd, 3) < 0) 
    {
        std::cout << "Listen failed" << std::endl;
        return 1;
    }
    std::cout << "Server started" << std::endl;
    sigset_t origMask;
    setupSignalHandling(&origMask);
	
    while (true) 
    {
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(server_fd, &read_fds);
        int maxFd = server_fd;

        for (int i = 0; i < active_clients; i++) {
            FD_SET(clients[i].connfd, &read_fds);
            if(clients[i].connfd > maxFd) {
                maxFd = clients[i].connfd;
            }
        }
        //обрабатываем сигнал
        if (pselect (maxFd + 1, &read_fds, NULL, NULL, NULL, &origMask) < 0 ) {
            if (errno == EINTR) {
                continue;
            }
            return -1;
        }
        
        if (FD_ISSET(server_fd, &read_fds) && active_clients < MAX_CLIENTS) 
        {
            client_t *client = &clients[active_clients];
			socklen_t client_fd_address_len = sizeof(client->addr);
			int client_fd = accept(server_fd, (struct sockaddr*)&client->addr, &client_fd_address_len);
            if (client_fd < 0)
            {
                std::cout << "Accept failed" << std::endl;
                return 1;
            }
            client->connfd = client_fd;
            active_clients++;
            std::cout << "New connection accepted" << std::endl;
		}
        //буфер
		
        for (int i = 0; i < active_clients; i++) {
            client_t *client = &clients[i];
            if (FD_ISSET(client->connfd, &read_fds)) {
                char buffer[1024] = {0};
                int read_len = read(client->connfd, &buffer, 1023);
                if (read_len > 0) {
                    buffer[read_len - 1] = 0;
                    printf(" %s\n", buffer);
                }
                else {
                    close(client->connfd);
					std::cout << "Connection closed for client: " << client->connfd<< std::endl;		
                    clients[i] = clients[active_clients - 1];
                    active_clients--;
                    i--;
                }
            }
        }
    }
}
