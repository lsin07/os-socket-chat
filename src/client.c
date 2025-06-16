#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <pthread.h>

#include "log.h"
#include "main.h"

static pthread_mutex_t console_mutex = PTHREAD_MUTEX_INITIALIZER;

static void sigpipe_handler(int signo)
{
    (void)signo;
    printf("Connection closed by server.\n");
    exit(EXIT_SUCCESS);
}

static void *recv_routine(void* args)
{
    int cfd = *(int *)args;
    char buf[SERVER_MSG_BUF_LEN];

    for (;;)
    {
        ssize_t readLen = read(cfd, buf, sizeof(buf));
        if (readLen != SERVER_MSG_BUF_LEN)
        {
            usrErrExit("could not read from socket fd %d\n", cfd);
        }
        pthread_mutex_lock(&console_mutex);
        printf("\r\033[2K%s>>> ", buf);
        fflush(stdout);
        pthread_mutex_unlock(&console_mutex);
    }

    return NULL;
}

static void *send_routine(void* args)
{
    int cfd = *(int *)args;
    char buf[CLIENT_MSG_BUF_LEN];
    for (;;)
    {
        pthread_mutex_lock(&console_mutex);
        printf(">>> ");
        fflush(stdout);
        pthread_mutex_unlock(&console_mutex);
        char *res = fgets(buf, sizeof(buf), stdin);
        pthread_mutex_lock(&console_mutex);
        printf("\033[1A\r\033[2K");
        pthread_mutex_unlock(&console_mutex);
        if (res != NULL)
        {
            if (buf[0] == '!')
            {
                break;
            }
            buf[strcspn(buf, "\n")] = '\0';
            if (write(cfd, buf, sizeof(buf)) != CLIENT_MSG_BUF_LEN)
            {
                usrErrExit("could not write to socket fd %d\n", cfd);
            }
        }
    }

    return NULL;
}

int main(int argc, char *argv[])
{
    if (argc != 2 || strcmp(argv[1], "--help") == 0)
    {
        usageErr("%s server-host\n", argv[0]);
    }

    struct sigaction sa;
    sa.sa_handler = sigpipe_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGPIPE, &sa, NULL) == 1)
    {
        sysErrExit("sigaction");
    }

    struct addrinfo hints;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;
    hints.ai_family = AF_UNSPEC;                /* Allows IPv4 or IPv6 */
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_NUMERICSERV;
    struct addrinfo *result;
    char host[NI_MAXHOST];
    char service[NI_MAXSERV];

    if (getaddrinfo(argv[1], PORT_NUM, &hints, &result) != 0)
    {
        sysErrExit("getaddrinfo");
    }

    struct addrinfo *rp;
    int cfd;
    for (rp = result; rp != NULL; rp = rp->ai_next) {

        cfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (cfd == -1)
        {
            DEBUG_MSG("socket created failed, try next candidate\n");
            continue;
        }
        else
        {
            DEBUG_MSG("socket created, fd: %d, ai_family: %d, ai_socktype: %d, ai_protocol: %d\n", \
                cfd, rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        }

        if (connect(cfd, rp->ai_addr, rp->ai_addrlen) != -1)
        {
            if (getnameinfo(rp->ai_addr, rp->ai_addrlen, host, NI_MAXHOST, service, NI_MAXSERV, 0) == 0)
            {
                DEBUG_MSG("socket fd %d connected sucessfully with server %s:%s\n", cfd, host, service);
            }
            printf("Connected to server (%s, %s)\n", host, service);
            break;                              /* Success */
        }

        /* Connect failed: close this socket and try next address */

        close(cfd);
    }

    if (rp == NULL)
    {
        usrErrExit("Could not connect socket to any address\n");
    }

    freeaddrinfo(result);

    int *cfd_arg = (int *)malloc(sizeof(int));
    *cfd_arg = cfd;
    pthread_t sender_tid, reciever_tid;
    if (pthread_create(&sender_tid, NULL, send_routine, (void *)cfd_arg) != 0)
    {
        sysErrExit("pthread_create");
    }
    if (pthread_create(&reciever_tid, NULL, recv_routine, (void *)cfd_arg) != 0)
    {
        sysErrExit("pthread_create");
    }

    pthread_detach(reciever_tid);
    pthread_join(sender_tid, NULL);

    free(cfd_arg);
    close(cfd);
    return 0;
}
