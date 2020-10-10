#pragma once
#include <errno.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/inotify.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include "appacheWebServer.c"
#include "udp_client.c"


struct inotify_struct{

    struct pollfd fds[2];
    nfds_t nfds;
    int wd;
};

struct params_to_inotify{
	
	char* dic_to_watch;
	char* ip;
	int port;

};

char* handle_events(int fd, int wd, int argc, char argv[])
{
	/* Some systems cannot read integer variables if they are not
	   properly aligned. On other systems, incorrect alignment may
	   decrease performance. Hence, the buffer used for reading from
	   the inotify file descriptor should have the same alignment as
	   struct inotify_event. */

	char buf[4096]
	    __attribute__ ((aligned(__alignof__(struct inotify_event))));
	const struct inotify_event *event;
	int i;
	ssize_t len;
	char *ptr;
	/* Loop while events can be read from inotify file descriptor. */

	for (;;) {

		/* Read some events. */

		len = read(fd, buf, sizeof buf);
		if (len == -1 && errno != EAGAIN) {
			perror("read");
			exit(EXIT_FAILURE);
		}

		/* If the nonblocking read() found no events to read, then
		   it returns -1 with errno set to EAGAIN. In that case,
		   we exit the loop. */

		if (len <= 0)
			break;

		/* Loop over all events in the buffer */

		char* str = malloc(1024);
		for (ptr = buf; ptr < buf + len; ptr += sizeof(struct inotify_event) + event->len) 
		{

			event = (const struct inotify_event *) ptr;

			/* Print event type */
			time_t t = time(NULL);
			struct tm tm = *localtime(&t);
			if (event->mask & IN_OPEN)
			{
				printf("IN_OPEN: ");
			}
			if (event->mask & IN_CLOSE_NOWRITE)
			{
				printf("IN_CLOSE_NONWRITE: ");
				sprintf(str , "ACCESSED FILE: %s/%s ACCESS: %s TIME ACCESS: %d-%02d-%02d %02d:%02d:%02d%c", argv ,event ->name , "NO_WRITE" , tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec , '\0');	
			}
			else if (event->mask & IN_CLOSE_WRITE)
			{
				printf("IN_CLOSE_WRITE: ");
				sprintf(str ,"ACCESSED FILE: %s/%s ACCESS: %s TIME ACCESS: %d-%02d-%02d %02d:%02d:%02d%c", argv , event ->name , "WRITE" , tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec , '\0');
			}

			/* Print the name of the watched directory */
			
			printf("%s" , argv);

			/* Print the name of the file */

			if (event->len)
				printf("%s", event->name);

			/* Print type of filesystem object */

			if (event->mask & IN_ISDIR)
				printf(" [directory]\n");
			else
				printf(" [file]\n");
		}
		return str;
	}
}

void inotify_initialize(struct inotify_struct* inot_st ,char* dic_to_watch)
{
    int fd = inotify_init1(IN_NONBLOCK);
	if (fd == -1) 
	{
		perror("inotify_init1");
		exit(EXIT_FAILURE); 
	}

    int wd = inotify_add_watch(fd , dic_to_watch , IN_OPEN | IN_CLOSE_WRITE | IN_CLOSE_NOWRITE);
	if(wd == -1)
	{
		fprintf(stderr, "Cannot watch '%s'\n", dic_to_watch);
		perror("inotify_add_watch");
		exit(EXIT_FAILURE);
	}

    /* Prepare for polling */

	struct pollfd fds[2];
    nfds_t nfds = 2;

    /* Console input */

    fds[0].fd = STDIN_FILENO;
	fds[0].events = POLLIN;

    /* Inotify input */

	fds[1].fd = fd;
	fds[1].events = POLLIN;

    /* Wait for events and/or terminal input */


    inot_st->fds[0] = fds[0];
    inot_st->fds[1] = fds[1];
    inot_st->nfds = nfds;
    inot_st->wd = wd;
}

void inotify_loop(char* dic_to_watch, char* ip, int port)
{
    char buf;
    struct inotify_struct inot_st;
	inotify_initialize(&inot_st, dic_to_watch);
	struct udp_client_info udpInfo = {0};
	udp_client_init(&udpInfo, ip, port);
    printf("Listening for events.\n");
	while (1) 
	{
		int poll_num = poll(inot_st.fds, inot_st.nfds, -1);
		if (poll_num == -1) 
		{
			if (errno == EINTR)
				continue;
			perror("poll");
			exit(EXIT_FAILURE);
		}

		if (poll_num > 0) 
		{

			if (inot_st.fds[0].revents & POLLIN) 
			{

				/* Console input is available. Empty stdin and quit */

				while (read(STDIN_FILENO, &buf, 1) > 0 && buf != '\n')
					continue;
				break;
			}

			if (inot_st.fds[1].revents & POLLIN) 
			{

				/* Inotify events are available */
				char* str = handle_events(inot_st.fds[1].fd, inot_st.wd, 1, dic_to_watch);
				

				print_to_index(str);
				printf("Sent to index.html. ");

				if(connect(udpInfo.sockfd, (const struct sockaddr *)&udpInfo.servaddr, sizeof(struct sockaddr)) == -1)
					perror("connect");

				str = strcat(str , "\n");

				if((sendto(udpInfo.sockfd , str ,strlen(str), MSG_CONFIRM ,(const struct sockaddr *) &udpInfo.servaddr , sizeof(struct sockaddr))) == -1)
				{
					perror("sendto");
				}
				else
				{
					printf("Sent to Server. %s\n", str);
				}

				free(str);
			}
		}
	}

	printf("Listening for events stopped.\n");

	/* Close inotify file descriptor */
	close(inot_st.fds[1].fd);

	/* Close udp socket file descriptor */ 
	close(udpInfo.sockfd);

	exit(EXIT_SUCCESS);
}

void* inotify_thread(void* params)
{
	struct params_to_inotify* p = (struct params_to_inotify*)params;

	// reset index file
	reset_index(p ->dic_to_watch);

	// inotify_loop (appache + udp_client)
    inotify_loop(p->dic_to_watch, p->ip, p->port);

}