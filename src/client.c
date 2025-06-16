#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include "log.h"
#include "main.h"

static void sigpipe_handler([[maybe_unused]] int signo)
{
    printf("Connection closed from server.\n");
    exit(EXIT_SUCCESS);
}

int main()
{
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

    if (getaddrinfo("localhost", PORT_NUM, &hints, &result) != 0)
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
    
    char msg[MAX_LEN];
    for (;;)
    {
        printf(">>> ");
        fflush(stdout);
        char *res = fgets(msg, MAX_LEN, stdin);
        if (res != NULL)
        {
            if (msg[0] == '!')
            {
                break;
            }
            msg[strlen(msg) - 1] = '\0';
            ssize_t len = write(cfd, msg, MAX_LEN);
            if (len != MAX_LEN)
            {
                usrErrExit("could not write to socket fd %d\n", cfd);
            }
        }
    }

    close(cfd);
    return 0;
}
