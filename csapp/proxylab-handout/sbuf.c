#include "csapp.h"
#include "sbuf.h"

void sbuf_init(sbuf_t *sp, int n)
{
    sp->buf = Calloc(n, sizeof(int));
    sp->front = 0;
    sp->rear = 0;
    Sem_init(&sp->mutex, 0,1);
    Sem_init(&sp->slots,0,n);
    Sem_init(&sp->items,0,0);
}


/*Clean up buffer sp*/
void sbuf_deinit(sbuf_t *sp)
{
    Free(sp->buf);
}

/*Insert item onto the rear of shared buffer sp*/
void sbuf_insert(sbuf_t *sp, int item)
{
    P(&sp->slots);
    P(&sp->mutex);

    sp->buf[(++sp->rear)%(sp->n)] = item;

    V(&sp->mutex);
    V(&sp->items);
}

int sbuf_remove(sbuf_t *sp)
{
    int item;
    P(&sp->items);      //等待可以访问item
    P(&sp->mutex);      //锁上当前缓冲区
    item = sp->buf[(++sp->front)%(sp->n)];
    V(&sp->mutex);
    V(&sp->slots);      //Announce available slot

    return item;
}


