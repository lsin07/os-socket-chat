#ifndef _CLIENT_LIST_H
#define _CLIENT_LIST_H

#include <netdb.h>

typedef struct _client_info_t
{
    int fd;
    char host[NI_MAXHOST];
    char service[NI_MAXSERV];
} client_info_t;

typedef struct _client_t
{
    client_info_t *client_info;
    struct _client_t* next;
} client_t;

/*  add a client information to the linked list.
    WARNING: `add_client`, `remove_client`, and `get_cllist_head` functions are NOT THREAD SAFE. */
int add_client(client_info_t *clinfo);

/*  remove a client from the linked list.
    WARNING: `add_client`, `remove_client`, and `get_cllist_head` functions are NOT THREAD SAFE. */
int remove_client(const client_info_t *clinfo);

/*  get the first item of the linked list.
    WARNING: `add_client`, `remove_client`, and `get_cllist_head` functions are NOT THREAD SAFE. */
client_t *get_cllist_head(void);

#endif