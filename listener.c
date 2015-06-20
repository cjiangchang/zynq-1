/*
** listener.c -- a datagram sockets "server" demo
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define MYPORT "32000"	// the port users will be connecting to

#define MAXBUFLEN 1500

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(void)
{
	int sockfd;
	struct addrinfo hints, *servinfo, *p;
	int rv;
	int numbytes;
	struct sockaddr_storage their_addr;
	char buf[MAXBUFLEN];
	socklen_t addr_len;
	char s[INET6_ADDRSTRLEN];

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC; // set to AF_INET to force IPv4
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE; // use my IP

	if ((rv = getaddrinfo(NULL, MYPORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and bind to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("listener: socket");
			continue;
		}

		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("listener: bind");
			continue;
		}

		break;
	}

	if (p == NULL) {
		fprintf(stderr, "listener: failed to bind socket\n");
		return 2;
	}

	freeaddrinfo(servinfo);

	printf("listener: waiting to recvfrom...\n");

	addr_len = sizeof their_addr;
	
	unsigned short seq;
	unsigned int timestamp;
	int basicb[55] = {0};
	int offset;
	char num;
	uint32_t diffCount;

	while(1){
		offset = 0;
		if ((numbytes = recvfrom(sockfd, buf, MAXBUFLEN-1 , 0,
			(struct sockaddr *)&their_addr, &addr_len)) == -1) {
			perror("recvfrom");
			exit(1);
		}


		while(1){
			if (buf[offset] == 'D'){
				break;
			}
			//get the snapshot number
			memcpy(&seq, buf, sizeof(unsigned short));
			offset += sizeof(unsigned short);
			printf("Seq num: %hu\n", seq);
		
			//get the timestamp
			memcpy(&timestamp, buf+offset, sizeof(unsigned int));
			offset += sizeof(unsigned int);
			printf("Timestamp: %u\n", timestamp);
	
			//get the basic block counts
			while(1){
				if (buf[offset] == '#'){
					offset += 1;
					break;
				}
				//get the basic block number and the difference
				memcpy(&num, buf+offset, sizeof(char));
				offset += sizeof(char);
				printf("counter num: %d\n", (unsigned int)num);
				memcpy(&diffCount, buf+offset, sizeof(uint32_t));
				offset += sizeof(uint32_t);
				printf("BB diff: %u\n", diffCount);
				//update the local basic block counts
				basicb[num] += diffCount;
			}		
		}
	}

	close(sockfd);

	return 0;
}
