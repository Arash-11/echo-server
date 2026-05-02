#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define PORT "7"
#define REQ_BUF_SIZE (1 << 14) // 16KiB

int setup_listener_socket(char *port) {
	int sockfd, error;
	struct addrinfo *ai, *p;
	int yes = 1;

	struct addrinfo hints = {
		.ai_family = PF_UNSPEC,
		.ai_socktype = SOCK_STREAM,
		.ai_flags = AI_NUMERICSERV | AI_PASSIVE,
	};

	if ((error = getaddrinfo(NULL, port, &hints, &ai)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(error));
		exit(EXIT_FAILURE);
	}

	// Loop through list of potential address candidates
	// and try setting up a socket on each.
	for (p = ai; p; p = p->ai_next) {
		sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
		if (sockfd == -1) {
			perror("server: socket error\n");
			continue;
		}

		// SO_REUSEADDR prevents "address already in use" errors by
		// enabling duplicate address and port bindings (needed during testing).
		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
			close(sockfd);
			perror("server: setsockopt error");
			continue;
		}

		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("server: bind error");
			continue;
		}
	
		break;
	}

	freeaddrinfo(ai);

	// If p is NULL, it means we didn't break out
	// of the above loop and we don't have a socket.
	if (p == NULL) {
		fprintf(stderr, "server: failed to bind\n");
		exit(EXIT_FAILURE);
	}

	if (listen(sockfd, SOMAXCONN) == -1) {
		close(sockfd);
		perror("server: listen error\n");
		exit(EXIT_FAILURE);
	}

	return sockfd;
}

int main(void) {
	struct sockaddr_storage client_addr;

	int sockfd = setup_listener_socket(PORT);

	char req[REQ_BUF_SIZE];

	printf("server: waiting for connections...\n");

	while(1) {
		socklen_t client_len = sizeof(client_addr);
		int connfd = accept(sockfd, (struct sockaddr*)&client_addr, &client_len);
		if (connfd == -1) {
			perror("server: accept error\n");
			continue;
		}

		ssize_t bytes_read;
		// reminder: `req` is a pointer to the first element (`&req[0]`)
		while ((bytes_read = recv(connfd, req, sizeof(req), 0)) > 0) {
			ssize_t total_sent = 0;
			while (total_sent < bytes_read) {
				// If the peer is slow to recv(), the TCP buffer in the kernel fills up.
				// This causes the peer's receive window to close, and send() returns
				// however many bytes it managed to queue rather than blocking forever.
				ssize_t n = send(connfd, req + total_sent, bytes_read - total_sent, 0);
				if (n == -1) {
					perror("server: send error\n");
					exit(EXIT_FAILURE);
				}
				total_sent += n;
			}
		}

		close(connfd);
	}

	return 0;
}
