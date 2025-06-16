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

#define BACKLOG 50
#define ADDRSTRLEN (NI_MAXHOST + NI_MAXSERV + 10)

int main()
{
    if (signal(SIGPIPE, SIG_IGN) == SIG_ERR)
    {
        perror("signal");
        exit(EXIT_FAILURE);
    }
    
    struct addrinfo hints;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_family = AF_UNSPEC;
    hints.ai_flags = AI_PASSIVE | AI_NUMERICSERV;
    struct addrinfo *result;
    if (getaddrinfo(NULL, PORT_NUM, &hints, &result) != 0)
    {
        sysErrExit("getaddrinfo");
    }

    int optval = 1;
    int lfd;
    struct addrinfo *rp;

    char addrStr[ADDRSTRLEN];
    char host[NI_MAXHOST];
    char service[NI_MAXSERV];

    for (rp = result; rp != NULL; rp = rp->ai_next)
    {
        lfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (lfd == -1)
        {
            DEBUG_MSG("socket created failed, try next candidate\n");
            continue;
        }
        else
        {
            DEBUG_MSG("socket created, fd: %d, ai_family: %d, ai_socktype: %d, ai_protocol: %d\n", \
                lfd, rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        }
        

        if (setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) == -1)
        {
            sysErrExit("setsockopt");
        }

        
        if (bind(lfd, rp->ai_addr, rp->ai_addrlen) == 0)
        {
            if (getnameinfo(rp->ai_addr, rp->ai_addrlen, host, NI_MAXHOST, service, NI_MAXSERV, 0) == 0)
            {
                DEBUG_MSG("socket fd %d binded successfully with %s:%s\n", lfd, host, service);
            }
            break;
        }

        close(lfd);
    }

    if (rp == NULL)
    {
        usrErrExit("Could not bind socket to any address\n");
    }

    if (listen(lfd, BACKLOG) == -1)
    {
        sysErrExit("listen");
    }
    else
    {
        DEBUG_MSG("listening socket fd %d\n", lfd);
    }

    freeaddrinfo(result);

    struct sockaddr_storage claddr;
    socklen_t addrlen;
    int cfd;

    for (;;)
    {
        addrlen = sizeof(struct sockaddr_storage);
        DEBUG_MSG("waiting for client at socket fd %d...\n", lfd);
        cfd = accept(lfd, (struct sockaddr *)&claddr, &addrlen);
        if (cfd == -1)
        {
            perror("accept");
            continue;
        }        

        if (getnameinfo((struct sockaddr *)&claddr, addrlen, host, NI_MAXHOST, service, NI_MAXSERV, 0) == 0)
        {
            snprintf(addrStr, ADDRSTRLEN, "(%s, %s)", host, service);
        }
        else
        {
            snprintf(addrStr, ADDRSTRLEN, "(?UNKNOWN?)");
        }
        printf("Connection from %s\n", addrStr);

        for (;;)
        {
            char buf[MAX_LEN];
            ssize_t readLen = read(cfd, buf, MAX_LEN);
            if (readLen != MAX_LEN)
            {
                DEBUG_MSG("client %s:%s has terminated the connection or crashed.\n", host, service);
                break;
            }
            printf("Message from %s:%s: %s\n", host, service, buf);
        }
    }

    close(cfd);
    return 0;
}
