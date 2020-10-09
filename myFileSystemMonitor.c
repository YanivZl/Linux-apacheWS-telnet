
#include <errno.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/inotify.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <string.h>
#include <sys/socket.h> 
#include <sys/types.h> 
#include <netinet/in.h> 
#include <arpa/inet.h>
#include <libcli.h>
#include <getopt.h>
#include <execinfo.h>

       /* Read all available inotify events from the file descriptor 'fd'.
          wd is the table of watch descriptors for the directories in argv.
          argc is the length of wd and argv.
          argv is the list of watched directories.
          Entry 0 of wd and argv is unused. */

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
				//printf("Event watched.");
				printf("IN_CLOSE_NONWRITE: ");
				sprintf(str , "ACCESSED FILE: %s/%s ACCESS: %s TIME ACCESS: %d-%02d-%02d %02d:%02d:%02d%c", argv ,event ->name , "NO_WRITE" , tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec , '\0');	
			}
			else if (event->mask & IN_CLOSE_WRITE)
			{
				//printf("Event watched.");
				printf("IN_CLOSE_WRITE: ");
				sprintf(str ,"ACCESSED FILE: %s/%s ACCESS: %s TIME ACCESS: %d-%02d-%02d %02d:%02d:%02d%c", argv , event ->name , "WRITE" , tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec , '\0');
			}

			/* Print the name of the watched directory */
			
			printf("%s" , argv);
			// for (i = 1; i < argc; ++i) {
			// 	if (wd == event->wd) {
			// 		printf("%s/", argv);
			// 		break;
			// 	}
			// }

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

#define PORT 8080

int callback(const char* username, const char* pass)
{
	if(username == "user" && pass == "123")
		return CLI_OK;
	return CLI_ERROR;
}

int count = 0;
void __attribute__((no_instrument_function)) __cyg_profile_func_enter(void *this_fn, void *call_site)
{
	printf("Enter %d\n", ++count);
}

void __attribute__((no_instrument_function)) __cyg_profile_func_exit(void *this_fn, void *call_site)
{
	printf("Exit cnt=%d\n", --count);
}

int main(int argc, char *argv[])
{
	// getopt 
	int option;
	char *dic_input, *ip_input;
	while((option = getopt(argc, argv, "d:i:")) != -1){
    	switch (option) {
			case 'd':
				dic_input = optarg;
				break;
			case 'i':
				ip_input = optarg;
				break;
			default:
				return EXIT_FAILURE;
		}
	}

	// libcli
	struct cli_def* cli = cli_init();
	cli_allow_user(cli, "user", "123");
	cli_set_auth_callback(cli, callback);
	//struct cli_command *c = cli_register_command(cli, NULL, "backtrace", backtrace, PRIVILEGE_UNPRIVILEGED, MODE_EXEC, NULL);

	char buf;
	int fd, i, poll_num;
	int wd;
	nfds_t nfds;
	struct pollfd fds[2];
	
	const char* index_dir = "/var/www/html/index.html";

	if (argc != 3) 
    {
		printf("The program works with 3 arguments only!\n");
		exit(EXIT_FAILURE);
	}

	// UDP client
	int sockfd;
	struct sockaddr_in  server_addr;
	if((sockfd = socket(AF_INET , SOCK_DGRAM, 0)) < 0)
	{
		perror("socket");
		exit(EXIT_FAILURE);
	}
	server_addr.sin_family = AF_INET;
	if(!inet_aton(ip_input , &server_addr.sin_addr))
	{
		perror("inet_aton");
		exit(EXIT_FAILURE);
	}

    
	printf("Press ENTER key to terminate.\n");

	/* Create the file descriptor for accessing the inotify API */

	fd = inotify_init1(IN_NONBLOCK);
	if (fd == -1) 
	{
		perror("inotify_init1");
		exit(EXIT_FAILURE); 
	}

	/* Allocate memory for watch descriptors */

	// wd = malloc(sizeof(int));
	// if (wd == NULL) 
	// {
	// 	perror("malloc");
	// 	exit(EXIT_FAILURE);
	// }

	/* Mark directories for events
	   - file was opened
	   - file was closed */
	wd = inotify_add_watch(fd , dic_input , IN_OPEN | IN_CLOSE_WRITE | IN_CLOSE_NOWRITE);
	if(wd == -1)
	{
		fprintf(stderr, "Cannot watch '%s'\n", dic_input);
		perror("inotify_add_watch");
		exit(EXIT_FAILURE);
	}
	// for (i = 1; i < argc; i++) {
	// 	wd[i] = inotify_add_watch(fd, argv[i], IN_OPEN | IN_CLOSE_NOWRITE | IN_CLOSE_WRITE);
	// 	if (wd[i] == -1) 
	// 	{
	// 		fprintf(stderr, "Cannot watch '%s'\n", argv[i]);
	// 		perror("inotify_add_watch");
	// 		exit(EXIT_FAILURE);
	// 	}
	// }

	/* Prepare for polling */

	nfds = 2;

	/* Console input */

	fds[0].fd = STDIN_FILENO;
	fds[0].events = POLLIN;

	/* Inotify input */

	fds[1].fd = fd;
	fds[1].events = POLLIN;

	/* Wait for events and/or terminal input */

	printf("Listening for events.\n");
	while (1) 
	{
		poll_num = poll(fds, nfds, -1);
		if (poll_num == -1) 
		{
			if (errno == EINTR)
				continue;
			perror("poll");
			exit(EXIT_FAILURE);
		}

		if (poll_num > 0) 
		{

			if (fds[0].revents & POLLIN) 
			{

				/* Console input is available. Empty stdin and quit */

				while (read(STDIN_FILENO, &buf, 1) > 0 && buf != '\n')
					continue;
				break;
			}

			if (fds[1].revents & POLLIN) 
			{

				/* Inotify events are available */
				char* str = handle_events(fd, wd, 1, dic_input);
				// char buf2[36];
				// char* temp = malloc(7);
				// if(ed ->writeOrRead)
				// 	sprintf(temp , "Write\n");
				// else
				// 	sprintf(temp , "Read\n");
				// sprintf(buf2 , "<br>time: %d-%02d-%02d %02d:%02d:%02d %s", ed -> tm.tm_year + 1900, ed -> tm.tm_mon + 1, ed -> tm.tm_mday, ed -> tm.tm_hour, ed -> tm.tm_min, ed -> tm.tm_sec , temp);
				// free(temp);

				/* Open 'index.html' to append events */

				int fd2 = open(index_dir , O_RDWR);
				if(fd2 == -1)
				{
					perror("open");
					exit(EXIT_FAILURE);
				}
				off_t off_t = lseek(fd2 , SEEK_SET , SEEK_END);
				lseek(fd2 , off_t , SEEK_SET);
				if(str[0] == '/')
					write(fd2 , "<br>" , 4);
				char* temp = strchr(str , '\0');
				if((sendto(sockfd , str ,temp - str + 1 , MSG_CONFIRM ,(const struct sockaddr *) &server_addr , sizeof(server_addr))) == -1)
				{
					perror("sendto");
				}
				else
				{
					printf("Sent to Server");
				}
				
				if(write(fd2 , str , temp - str + 1) == -1)
				{
					perror("write");
				}
				else
					printf("Printed to index.html\n");
				//fprintf(fd2 , "<br>time: %d-%02d-%02d %02d:%02d:%02d\t%s", ed -> tm.tm_year + 1900, ed -> tm.tm_mon + 1, ed -> tm.tm_mday, ed -> tm.tm_hour, ed -> tm.tm_min, ed -> tm.tm_sec , ed ->writeOrRead ? "Write" : "Read");
				close(fd2);
				free(str);

			}
		}
	}

	printf("Listening for events stopped.\n");

	/* Close inotify file descriptor */

	close(fd);
	exit(EXIT_SUCCESS);
}