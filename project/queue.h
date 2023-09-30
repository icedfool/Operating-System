#ifndef __QUEUE_H_
#define __QUEUE_H_

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

typedef struct queue
{

    // the char array that holds items:
    // ' ' as empty slot
    char *queue;
    int size;
    int current_size;

} queue;

void queue_init(queue *q, int size)
{
    q->size = size;

    q->queue = calloc(size, sizeof(char));
    for (int i = 0; i < size; i++)
    {
        q->queue[i] = ' ';
    }

    q->current_size = 0;
}

void deleting_first(queue *q)
{
    int cur_size = q->current_size;
    if (cur_size == 1)
    {
        q->queue[0] = ' ';
        q->current_size--;
    }
    else if (cur_size > 1)
    {
        for (int i = 1; i < cur_size; i++)
        {
            q->queue[i - 1] = q->queue[i];
        }
        q->queue[cur_size - 1] = ' ';
        q->current_size--;
    }
}

void adding_one(queue *q, char input)
{
    int cur_size = q->current_size;
    // int already = 0; 

    // for (int i = 0; i < q->current_size; i++) {
    //     if ()
    // }

    if (cur_size < q->size)
    {
        q->queue[cur_size] = input;
        q->current_size++;
    }
}

void printing(queue *q)
{
    printf("[Q:");
    if (q->current_size != 0)
    {
        for (int i = 0; i < q->current_size; i++)
        {
            printf(" %c", q->queue[i]);
        }
    }
    else
    {
        printf(" empty");
    }
    printf("]\n");
}

#endif