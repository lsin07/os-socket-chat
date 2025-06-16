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

#define BACKLOG 50
#define ADDRSTRLEN (NI_MAXHOST + NI_MAXSERV + 10)

typedef struct _client_info_t
{
    int cfd;
    char host[NI_MAXHOST];
    char service[NI_MAXSERV];
} client_info_t;

static void *client_routine(void *arg)
{
    client_info_t *clinfo = (client_info_t *)arg;
    char buf[MAX_LEN];
    int cfd = clinfo->cfd;
    char *host = clinfo->host;
    char *service = clinfo->service;
    
    for (;;)
    {
        ssize_t readLen = read(cfd, buf, MAX_LEN);
        if (readLen != MAX_LEN)
        {
            printf("Connection terminated from (%s, %s)\n", host, service);
            break;
        }
        printf("Message from %s:%s: %s\n", host, service, buf);
    }
    
    free(clinfo);
    return NULL;
}

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
    pthread_t tid;

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

        client_info_t *info = malloc(sizeof(client_info_t));
        if (info == NULL)
        {
            sysErrExit("malloc");
        }
        info->cfd = cfd;
        strncpy(info->host, host, sizeof(info->host));
        strncpy(info->service, service, sizeof(info->service));

        if (pthread_create(&tid, NULL, client_routine, (void *)info) != 0)
        {
            perror("pthread_create");
            close(cfd);
            free(info);
            continue;
        }
        else
        {
            DEBUG_MSG("created new thread (tid: %lu) for client %s:%s\n", tid, host, service);
        }
        
        pthread_detach(tid);
    }

    close(cfd);
    return 0;
}
