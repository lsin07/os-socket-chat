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
#include "client_list.h"
#include "main.h"

#define BACKLOG 50
#define ADDRSTRLEN (NI_MAXHOST + NI_MAXSERV + 10)

/* client linked list functions are not thread safe. protect them. */
static pthread_mutex_t cllist_mutex = PTHREAD_MUTEX_INITIALIZER;

/* make sure that the console logs are not being messed up. */
static pthread_mutex_t console_mutex = PTHREAD_MUTEX_INITIALIZER;

static void *service_routine(void *arg)
{
    client_info_t *clinfo = (client_info_t *)arg;
    pthread_mutex_lock(&cllist_mutex);
    add_client(clinfo);
    pthread_mutex_unlock(&cllist_mutex);

    char strRecv[CLIENT_MSG_BUF_LEN];
    char strSend[SERVER_MSG_BUF_LEN];
    int cfd = clinfo->fd;
    char *host = clinfo->host;
    char *service = clinfo->service;

    while (read(cfd, strRecv, CLIENT_MSG_BUF_LEN) == CLIENT_MSG_BUF_LEN)
    {
        snprintf(strSend, sizeof(strSend), "Message from %s:%s: %s\n", host, service, strRecv);
        pthread_mutex_lock(&console_mutex);
        printf("%s", strSend);
        pthread_mutex_unlock(&console_mutex);

        int rfd;
        pthread_mutex_lock(&cllist_mutex);
        client_t *client = get_cllist_head();
        while (client != NULL)
        {
            /* broadcast message to all clients: traverse client linked list and send message to each clients */
            rfd = client->client_info->fd;
            if (rfd != cfd)
            {
                DEBUG_MSG("broadcasting message -- sending message to client %d\n", rfd);
                ssize_t w = write(rfd, strSend, sizeof(strSend));
                if (w != (ssize_t)(SERVER_MSG_BUF_LEN))
                {
                    pthread_mutex_unlock(&cllist_mutex);
                    goto TERMINATE;
                }
            }
            client = client->next;
        }
        pthread_mutex_unlock(&cllist_mutex);
    }

TERMINATE:
    pthread_mutex_lock(&console_mutex);
    printf("Client (fd %d) connection terminated from (%s, %s)\n", cfd, host, service);
    pthread_mutex_unlock(&console_mutex);
    pthread_mutex_lock(&cllist_mutex);
    remove_client(clinfo);
    pthread_mutex_unlock(&cllist_mutex);
    free(clinfo);
    close(cfd);
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
            DEBUG_MSG("socket created, ai_family: %d, ai_socktype: %d, ai_protocol: %d\n", \
                rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        }

        if (setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) == -1)
        {
            sysErrExit("setsockopt");
        }

        if (bind(lfd, rp->ai_addr, rp->ai_addrlen) == 0)
        {
            if (getnameinfo(rp->ai_addr, rp->ai_addrlen, host, NI_MAXHOST, service, NI_MAXSERV, 0) == 0)
            {
                DEBUG_MSG("listening socket fd %d binded successfully with %s:%s\n", lfd, host, service);
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
        printf("Server up (listening %s:%s)\n", host, service);
    }

    freeaddrinfo(result);

    struct sockaddr_storage claddr;
    socklen_t addrlen;
    int cfd;
    pthread_t tid;

    for (;;)
    {
        addrlen = sizeof(struct sockaddr_storage);
        DEBUG_MSG("waiting for client...\n");
        cfd = accept(lfd, (struct sockaddr *)&claddr, &addrlen);
        if (cfd == -1)
        {
            perror("accept");
            continue;
        }
        else
        {
            DEBUG_MSG("client accepted: fd %d\n", cfd);
        }

        if (getnameinfo((struct sockaddr *)&claddr, addrlen, host, NI_MAXHOST, service, NI_MAXSERV, 0) == 0)
        {
            snprintf(addrStr, ADDRSTRLEN, "(%s, %s)", host, service);
        }
        else
        {
            snprintf(addrStr, ADDRSTRLEN, "(?UNKNOWN?)");
        }
        printf("Client (fd %d) connected from %s\n", cfd, addrStr);

        client_info_t *info = (client_info_t *)malloc(sizeof(client_info_t));
        if (info == NULL)
        {
            sysErrExit("malloc");
        }
        info->fd = cfd;
        strncpy(info->host, host, sizeof(info->host));
        strncpy(info->service, service, sizeof(info->service));

        if (pthread_create(&tid, NULL, service_routine, (void *)info) != 0)
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

    return 0;
}
