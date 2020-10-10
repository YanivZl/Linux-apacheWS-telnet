#include <sys/socket.h> 
#include <sys/types.h> 
#include <netinet/in.h> 
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "inotify.c"

#define INDEX_DIR "/var/www/html/index.html"

struct udp_client_info{
	int sockfd;
	struct sockaddr_in servaddr;
};

void udp_client_init(struct udp_client_info* udpInfo,char* ip, int port)
{
    udpInfo->servaddr.sin_family = AF_INET;
	inet_aton(ip, &udpInfo->servaddr.sin_addr);
    udpInfo->servaddr.sin_port = htons(port);
	
    if ((udpInfo->sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) { 
        perror("socket"); 
        exit(EXIT_FAILURE); 
    } 
}

