#include "inotify.c"
#include "backtrace_telnet.c"

#define PORT 8000
#define BACKTRACE_LENGTH 100

//Global vars
sem_t telnet_sem;

pthread_t thread_inotify;
pthread_t thread_telnet;

int main(int argc, char* argv[])
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

	
	pthread_t inotify;
	pthread_t telnet;
	
	struct params_to_inotify params;
	params.dic_to_watch = dic_input;
	params.ip = ip_input;
	params.port = PORT;

	if (sem_init(&sem, 0, 0) == -1){
        perror("sem_init");
        exit(EXIT_FAILURE);
  	}
	
	if (pthread_create(&inotify, NULL, (void*)inotify_thread, (void*)&params))
    	exit(EXIT_FAILURE);
 	if (pthread_create(&telnet, NULL, telnet_thread, NULL))
    	exit(EXIT_FAILURE);


  	pthread_join(inotify, NULL);
 	pthread_join(telnet, NULL);
  	sem_close (&sem);
  	exit(EXIT_SUCCESS);

  
}