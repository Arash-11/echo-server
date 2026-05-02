#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <sys/_types/_fsfilcnt_t.h>
#include <sys/_types/_gid_t.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define PORT "7"

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
		// TODO: add conditional compilation flag?
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

	socklen_t client_len = sizeof(client_addr);

	int sockfd = setup_listener_socket(PORT);
	printf("socket_fd = %d\n", sockfd);

	printf("server: waiting for connections...\n");

	while(1) {
		int connfd = accept(sockfd, (struct sockaddr*)&client_addr, &client_len);
		if (connfd == -1) {
			close(sockfd);
			perror("server: accept error\n");
			exit(EXIT_FAILURE);
		}
	}

	return 0;
}
