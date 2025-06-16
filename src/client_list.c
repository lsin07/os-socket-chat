#include <stdlib.h>
#include "client_list.h"
#include "log.h"

static client_t *head = NULL;
static client_t *tail = NULL;

int add_client(client_info_t *clinfo)
{
    client_t *new_cl = (client_t *)malloc(sizeof(client_t));
    new_cl->client_info = clinfo;

    if (head == NULL)
    {
        head = new_cl;
    }
    else
    {
        tail->next = new_cl;
    }
    new_cl->next = NULL;
    tail = new_cl;

    DEBUG_MSG("client %d added to list.\n", new_cl->client_info->fd);

    return 0;
}

int remove_client(const client_info_t *clinfo)
{
    int cfd = clinfo->fd;
    client_t *cur = head;
    client_t *prev = NULL;
    while (cur != NULL)
    {
        if (cur->client_info->fd == cfd)
        {
            if (prev == NULL)
            {
                /* it's an head. */
                head = cur->next;
            }
            else if (cur->next == NULL)
            {
                /* it's a tail. */
                tail = prev;
                tail->next = NULL;
            }
            else
            {
                prev->next = cur->next;
            }
            DEBUG_MSG("client %d removed from list.\n", cur->client_info->fd);
            free(cur);
            return 0;
        }
        else
        {
            prev = cur;
            cur = cur->next;
        }
    }

    DEBUG_WARN("fd %d not found in client list.\n", cfd);
    return -1;
}

client_t *get_cllist_head(void)
{
    return head;
}