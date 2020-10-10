#pragma once
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
#include <pthread.h>
#include <execinfo.h>
#include <semaphore.h> 
#include "libcli.h"
#include <limits.h>
#include <signal.h>
#include <strings.h>

#define TELNET_PORT 9090
#define BACKTRACE_LENGTH 100

pthread_t thread_telnet;
pthread_t thread_inotify;

#ifdef __GNUC__
#define UNUSED(d) d __attribute__((unused))
#else
#define UNUSED(d) d
#endif

//Global

unsigned int regular_count = 0;
unsigned int debug_regular = 0;

void *backtrace_buffer[BACKTRACE_LENGTH];

struct backtrace {
    char **trace;
    int nptrs;
    char is_active;
};

struct backtrace bt_s;

struct backtrace* bt = &bt_s;

sem_t sem;

int backtrace_exce(struct cli_def* cli, UNUSED(const char* command), UNUSED(char* argv[]), UNUSED(int argc))
{

    bt->is_active = 1;

    cli_print(cli, "backtrace() returned %d addresses\n", bt->nptrs);


    for (int j = 0; j < bt->nptrs; j++)
        cli_print(cli, "%s\n", bt->trace[j]);


    sem_post(&sem);

    return CLI_OK;
}

int check_auth(const char *username, const char *pass) {
    if (strcmp(username, "UNIX") != 0 && strcmp(pass, "123")) 
        return CLI_ERROR;
    return CLI_OK;
}

int regular_callback(struct cli_def *cli) {
    regular_count++;
    if (debug_regular) {
        cli_print(cli, "Regular callback - %u times so far", regular_count);
        cli_reprompt(cli);
    }
    return CLI_OK;
}

int check_enable(const char *password) {
    return !strcmp(password, "123");
}

int idle_timeout(struct cli_def *cli) {
    cli_print(cli, "Custom idle timeout");
    return CLI_QUIT;
}

void pc(UNUSED(struct cli_def *cli), const char *string) {
    printf("%s\n", string);
}

void run_child(int x) {
    struct cli_def *cli;

    cli = cli_init();
    cli_set_banner(cli, "Commands : 'backtrace'. ");
    cli_set_hostname(cli, "UNIX FINAL");
    cli_allow_user(cli, "UNIX", "123");
    cli_telnet_protocol(cli, 1);
    cli_regular(cli, regular_callback);

    cli_regular_interval(cli, 5);

    cli_set_idle_timeout_callback(cli, 200, idle_timeout);
    

    cli_register_command(cli, NULL, "backtrace", backtrace_exce, PRIVILEGE_UNPRIVILEGED, MODE_EXEC,
                                          "Show backtrace"); 
    cli_set_auth_callback(cli, check_auth);

    cli_loop(cli, x);

    cli_done(cli);
}


void  __attribute__ ((no_instrument_function))  __cyg_profile_func_enter (void *this_fn,
                                                                          void *call_site)
{
        if(bt->is_active == 1) {

            sem_wait(&sem);


            bt->is_active = 0;
            bt->nptrs = 0;
            bt->trace = (char**)malloc(0*sizeof(char*));
            free(bt->trace);
        }


        if (!pthread_equal(thread_telnet, pthread_self())) {
            int nptrs = backtrace(backtrace_buffer, BACKTRACE_LENGTH);
            char** response = backtrace_symbols(backtrace_buffer, nptrs);

            bt->trace = (char**)realloc(bt->trace, (nptrs + bt->nptrs) * sizeof(char*));
            for (int i=0; i < nptrs; i++)
            {
                bt->trace[bt->nptrs + i] = (char*)malloc(BACKTRACE_LENGTH*sizeof(char));
                strcpy(bt->trace[bt->nptrs + i], response[i]);
            }
            bt->nptrs += nptrs;
           }

}

void* telnet_thread (void *args){
    int s,x;
    struct sockaddr_in addr;
    int on = 1;

    thread_telnet = pthread_self();

    signal(SIGCHLD, SIG_IGN);

    if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        exit(0);
    }

    if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on))) {
        perror("setsockopt");
        exit(0);
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(TELNET_PORT);
    if (bind(s, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        exit(0);
    }

    if (listen(s, 50) < 0) {
        perror("listen");
        exit(0);
    }

    printf("Listening on port %d\n", TELNET_PORT);
    while ((x = accept(s, NULL, 0))) {
        run_child(x);
        exit(0);
    }
}
