#include <stdint.h>
#include <stddef.h>
#include "cab.h"
#include <stdio.h>
#include <stdlib.h>

#define IMGWIDTH 128

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
    cab *new_cab = calloc(1, sizeof(cab));
    new_cab->name = name;
    new_cab->num = num;
    new_cab->dim = dim;

    // allocate the buffersTaken array
    new_cab->buffersTaken = (uint8_t *)calloc(num, sizeof(uint8_t));
    for(size_t i = 0; i < num; i++)
        new_cab->buffersTaken[i] = 0;

    // allocate all buffers
    new_cab->buffers = (uint8_t ***)calloc(num, sizeof(uint8_t **));
    for (size_t i = 0; i < num; i++)
    {
        new_cab->buffers[i] = (uint8_t **)calloc(IMGWIDTH, sizeof(uint8_t *));
        for (uint8_t j = 0; j < IMGWIDTH; j++)
            new_cab->buffers[i][j] = (uint8_t *)calloc(IMGWIDTH, sizeof(uint8_t));
    }
    for(size_t i = 0; i < num; i++)
        for(size_t j = 0; j < IMGWIDTH; j++)
            for(size_t k = 0; k < IMGWIDTH; k++)
                new_cab->buffers[i][j][k] = 0;
    for(size_t i = 0; i < IMGWIDTH; i++)
        for(size_t j = 0; j < IMGWIDTH; j++)
            new_cab->buffers[0][i][j] = first[i][j];
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
            for(size_t j = 0; j < IMGWIDTH; j++)
                for(size_t k = 0; k < IMGWIDTH; k++)
                    cab_id->buffers[0][j][k] = cab_id->buffers[i][j][k];            
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
            for(size_t j = 0; j < IMGWIDTH; j++)
                for(size_t k = 0; k < IMGWIDTH; k++)
                    cab_id->buffers[i][j][k] = cab_id->buffers[0][j][k];
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

int test_cab(int argc, char const *argv[])
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
    printf("cab %s -> num=%ld, dim=%d\n", cab1->name, cab1->num, cab1->dim);

     

    uint8_t **buffer1 = get_mes(cab1);

    for (size_t i = 0; i < IMGWIDTH; i++)
    {
        for (size_t j = 0; j < IMGWIDTH; j++)
        {
            printf("%d, ", buffer1[i][j]);
        }
        printf("\n");
    }

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

    uint8_t **buffer2 = get_mes(cab1);

    for (size_t i = 0; i < IMGWIDTH; i++)
    {
        for (size_t j = 0; j < IMGWIDTH; j++)
        {
            printf("%d, ", buffer2[i][j]);
        }
        printf("\n");
    }

    unget(buffer1, cab1);
    unget(buffer2, cab1);

    

    return 0;
}
