#include <zephyr.h>
#include <stdint.h>
#include <stddef.h>
#include "cab.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/printk.h>
#include <string.h> 

#define IMGWIDTH 128

struct cab
{
    struct k_sem *op_Sem;
    char *name;
    int num;
    size_t dim;
    void **buffers;
    uint8_t *buffersTaken;
};

// creates a new cab
cab *open_cab(char *name, int num, size_t dim, void *first)
{
    cab *new_cab = calloc(1, sizeof(cab));
    new_cab->name = name;
    new_cab->num = num;
    new_cab->dim = dim;
    new_cab->op_Sem = (struct k_sem*)calloc(1, sizeof(struct k_sem));
    k_sem_init(new_cab->op_Sem, 1, 1);
    // allocate the buffersTaken array
    new_cab->buffersTaken = (uint8_t *)calloc(num, sizeof(uint8_t));
    for (size_t i = 0; i < num; i++)
        new_cab->buffersTaken[i] = 0;

    // allocate all buffers
    new_cab->buffers = (void **)calloc(num, sizeof(void *));
    for (size_t i = 0; i < num; i++)
    {
        new_cab->buffers[i] = (void *)calloc(1, dim);
    }

    memcpy(new_cab->buffers[0], first, dim);
    new_cab->buffersTaken[0] = 1; // The first will always be taken
    return new_cab;
}

// returns a new buffer
void *reserve(cab *cab_id)
{
    k_sem_take(cab_id->op_Sem, K_FOREVER);
    // find a free buffer
    for (size_t i = 0; i < cab_id->num; i++)
    {
        if (cab_id->buffersTaken[i] == 0)
        {
            cab_id->buffersTaken[i] = 1;
            k_sem_give(cab_id->op_Sem);
            return cab_id->buffers[i];
        }
    }
    k_sem_give(cab_id->op_Sem);
    return NULL;
}

// puts a filled buffer inside the CAB
void put_mes(void *buf_pointer, cab *cab_id)
{
    k_sem_take(cab_id->op_Sem, K_FOREVER);

    for (size_t i = 0; i < cab_id->num; i++)
    {
        if (cab_id->buffers[i] == buf_pointer)
        {
            memcpy(cab_id->buffers[0], cab_id->buffers[i], cab_id->dim);
            cab_id->buffersTaken[i] = 0;
        }
    }
    k_sem_give(cab_id->op_Sem);
}

// get latest message
void *get_mes(cab *cab_id)
{
    k_sem_take(cab_id->op_Sem, K_FOREVER);
    // find a free buffer
    for (size_t i = 0; i < cab_id->num; i++)
    {
        if (cab_id->buffersTaken[i] == 0)
        {
            cab_id->buffersTaken[i] = 1;
            memcpy(cab_id->buffers[i], cab_id->buffers[0], cab_id->dim);
            k_sem_give(cab_id->op_Sem);
            return cab_id->buffers[i];
        }
    }
    k_sem_give(cab_id->op_Sem);
    return NULL;
}

// release message to the CAB
void unget(void* mes_pointer, cab *cab_id)
{
    k_sem_take(cab_id->op_Sem, K_FOREVER);
    for (size_t i = 0; i < cab_id->num; i++)
    {
        if (cab_id->buffers[i] == mes_pointer)
        {
            cab_id->buffersTaken[i] = 0;
        }
    }
    k_sem_give(cab_id->op_Sem);
}

int test(int argc, char const *argv[])
{

    uint8_t **img1 = (uint8_t **)malloc(IMGWIDTH * sizeof(uint8_t *));
    for (uint8_t j = 0; j < IMGWIDTH; j++)
        img1[j] = (uint8_t *)malloc(IMGWIDTH * sizeof(uint8_t));

    uint8_t vertical[5][5] = {{0, 0, 255, 0, 0},
                              {0, 0, 255, 0, 0},
                              {0, 0, 255, 0, 0},
                              {0, 0, 255, 0, 0},
                              {0, 0, 255, 0, 0}};

    for (size_t i = 0; i < IMGWIDTH; i++)
        for (size_t j = 0; j < IMGWIDTH; j++)
            img1[i][j] = vertical[i][j];

    // testing
    cab *cab1;
    cab1 = open_cab("images", 3, IMGWIDTH * IMGWIDTH, (void*)img1);
    printf("cab %s -> num=%ld, dim=%d\n", cab1->name, cab1->num, cab1->dim);

    uint8_t **buffer1 = (uint8_t **)get_mes(cab1);

    for (size_t i = 0; i < IMGWIDTH; i++)
    {
        for (size_t j = 0; j < IMGWIDTH; j++)
        {
            printf("%d, ", buffer1[i][j]);
        }
        printf("\n");
    }

    uint8_t **free_buffer = (uint8_t **)reserve(cab1);

    uint8_t diagonal[5][5] = {{255, 0, 0, 0, 0},
                              {0, 255, 0, 0, 0},
                              {0, 0, 255, 0, 0},
                              {0, 0, 0, 255, 0},
                              {0, 0, 0, 0, 255}};

    for (size_t i = 0; i < IMGWIDTH; i++)
        for (size_t j = 0; j < IMGWIDTH; j++)
            free_buffer[i][j] = diagonal[i][j];

    put_mes((void *)free_buffer, cab1);

    uint8_t **buffer2 = get_mes(cab1);

    for (size_t i = 0; i < IMGWIDTH; i++)
    {
        for (size_t j = 0; j < IMGWIDTH; j++)
        {
            printf("%d, ", buffer2[i][j]);
        }
        printf("\n");
    }

    unget((void *)buffer1, cab1);
    unget((void *)buffer2, cab1);

    return 0;
}
