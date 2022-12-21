#include <stdint.h>
#include <stddef.h>
#include "cab.h"
#include <stdio.h>
#include <stdlib.h>

#define IMGWIDTH 5

struct cab
{
    char *name;
    size_t num;
    uint8_t dim;
    uint8_t ***buffers;
    uint8_t *buffersTaken;
};

// creates a new cab
cab *open_cab(char *name, size_t num, uint8_t dim, uint8_t **first)
{

    cab *new_cab = malloc(sizeof(cab));
    new_cab->name = name;
    new_cab->num = num;
    new_cab->dim = dim;

    // allocate the buffersTaken array
    new_cab->buffersTaken = (uint8_t *)calloc(num, sizeof(uint8_t));
    memset(new_cab->buffersTaken, 0, num);

    // allocate all buffers
    new_cab->buffers = (uint8_t ***)calloc(num, sizeof(uint8_t **));
    for (size_t i = 0; i < num; i++)
    {
        new_cab->buffers[i] = (uint8_t **)malloc(IMGWIDTH * sizeof(uint8_t *));
        for (uint8_t j = 0; j < IMGWIDTH; j++)
            new_cab->buffers[i][j] = (uint8_t *)malloc(IMGWIDTH * sizeof(uint8_t));
    }

    memset(new_cab->buffers, 0, num * IMGWIDTH * IMGWIDTH);
    memcpy(new_cab->buffers[0], first, IMGWIDTH * IMGWIDTH);
    new_cab->buffersTaken[0] = 1; // The first will always be taken
    return new_cab;
}

// returns a new buffer
uint8_t **reserve(cab *cab_id)
{
    // find a free buffer
    for (size_t i = 0; i < cab_id->num; i++)
    {
        if (cab_id->buffersTaken[i] == 0)
        {
            cab_id->buffersTaken[i] = 1;
            return cab_id->buffers[i];
        }
    }
    return NULL;
}

// puts a filled buffer inside the CAB
void put_mes(uint8_t **buf_pointer, cab *cab_id)
{

    for (size_t i = 0; i < cab_id->num; i++)
    {
        if (cab_id->buffers[i] == buf_pointer)
        {
            memcpy(cab_id->buffers[0], cab_id->buffers[i], IMGWIDTH * IMGWIDTH);
            cab_id->buffersTaken[i] = 0;
        }
    }
}

// get latest message
uint8_t **get_mes(cab *cab_id)
{
    // find a free buffer
    for (size_t i = 0; i < cab_id->num; i++)
    {
        if (cab_id->buffersTaken[i] == 0)
        {
            cab_id->buffersTaken[i] = 1;
            memcpy(cab_id->buffers[i], cab_id->buffers[0], IMGWIDTH * IMGWIDTH);
            return cab_id->buffers[i];
        }
    }
    return NULL;
}

// release message to the CAB
void unget(uint8_t **mes_pointer, cab *cab_id)
{
    for (size_t i = 0; i < cab_id->num; i++)
    {
        if (cab_id->buffers[i] == mes_pointer)
        {
            cab_id->buffersTaken[i] = 0;
        }
    }
}

int main(int argc, char const *argv[])
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
    cab1 = open_cab("images", 3, IMGWIDTH * IMGWIDTH, img1);
    printf("cab %s -> num=%ld, dim=%d,  first[0][0][2]=%d\n", cab1->name, cab1->num, cab1->dim, cab1->buffers[0][0][2]);

    uint8_t **free_buffer = reserve(cab1);

    uint8_t diagonal[5][5] = {{255, 0, 0, 0, 0},
                              {0, 255, 0, 0, 0},
                              {0, 0, 255, 0, 0},
                              {0, 0, 0, 255, 0},
                              {0, 0, 0, 0, 255}};

    for (size_t i = 0; i < IMGWIDTH; i++)
        for (size_t j = 0; j < IMGWIDTH; j++)
            free_buffer[i][j] = diagonal[i][j];

    put_mes(free_buffer, cab1);

    free_buffer = reserve(cab1);

    uint8_t diagonal2[5][5] = {{0, 0, 0, 0, 255},
                               {0, 0, 0, 255, 0},
                               {0, 0, 255, 0, 0},
                               {0, 255, 0, 0, 0},
                               {255, 0, 0, 0, 0}};

    for (size_t i = 0; i < IMGWIDTH; i++)
        for (size_t j = 0; j < IMGWIDTH; j++)
            free_buffer[i][j] = diagonal2[i][j];

    put_mes(free_buffer, cab1);

    for (size_t i = 0; i < IMGWIDTH; i++)
    {
        for (size_t j = 0; j < IMGWIDTH; j++)
        {
            printf("%d, ", cab1->buffers[0][i][j]);
        }
        printf("\n");
    }

    for (size_t i = 0; i < IMGWIDTH; i++)
    {
        for (size_t j = 0; j < IMGWIDTH; j++)
        {
            printf("%d, ", cab1->buffers[1][i][j]);
        }
        printf("\n");
    }

    for (size_t i = 0; i < IMGWIDTH; i++)
    {
        for (size_t j = 0; j < IMGWIDTH; j++)
        {
            printf("%d, ", cab1->buffers[2][i][j]);
        }
        printf("\n");
    }

    return 0;
}
