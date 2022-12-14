#include <stdint.h>
#include <stddef.h>
#include "cab.h"
#include <stdio.h>
#include <stdlib.h>

#define IMGWIDTH 5

struct cab {
   char * name;
   size_t num;
   uint8_t dim;
   uint8_t *** buffers;
   size_t buff_size;
   size_t size;
   size_t nBuffers;
};

//creates a new cab
cab * open_cab(char * name, size_t num, uint8_t dim, uint8_t ** first){

    cab * new_cab = malloc((sizeof(name) / sizeof(name[0]))*sizeof(size_t)*sizeof(uint8_t)*sizeof(uint8_t)*dim);
    new_cab->size = (sizeof(name) / sizeof(name[0]))*sizeof(size_t)*sizeof(uint8_t)*sizeof(uint8_t)*dim;
    new_cab-> num = num;
    new_cab-> dim = dim;

    // allocate a buffer 
    new_cab->buffers = (uint8_t***) malloc(sizeof(uint8_t**));
    new_cab->buffers[0] = (uint8_t**)malloc(IMGWIDTH * sizeof(uint8_t*));
    for (uint8_t j = 0; j < IMGWIDTH; j++)
        new_cab->buffers[0][j] = (uint8_t*)malloc(IMGWIDTH * sizeof(uint8_t));
    new_cab->buff_size = sizeof(uint8_t**) + IMGWIDTH * sizeof(uint8_t*) + IMGWIDTH *IMGWIDTH *sizeof(uint8_t);
    new_cab->buffers[0] = first;
    new_cab->nBuffers = 1;
    return new_cab;
}

//returns a new buffer
uint8_t ** reserve(cab * cab_id){
    // allocate another buffer 
    cab_id->size += cab_id->buff_size;
    cab_id->buff_size *= 2;
    cab_id->buffers = (uint8_t***) realloc(cab_id->buffers, cab_id->buff_size);
    uint8_t ** new_buffer = (uint8_t**)malloc(IMGWIDTH * sizeof(uint8_t*));
    
    for (uint8_t j = 0; j < IMGWIDTH; j++)
        new_buffer[j] = (uint8_t*)malloc(IMGWIDTH * sizeof(uint8_t));
    return new_buffer;
}

//puts a filled buffer inside the CAB
void put_mes (uint8_t ** buf_pointer, cab * cab_id){
    // allocate new buffer in CAB
    cab_id->buffers[cab_id->nBuffers] = (uint8_t**)malloc(IMGWIDTH * sizeof(uint8_t*));
    for (uint8_t j = 0; j < IMGWIDTH; j++)
        cab_id->buffers[cab_id->nBuffers][j] = (uint8_t*)malloc(IMGWIDTH * sizeof(uint8_t));
    
    // push back existing buffers
    int n = cab_id->nBuffers;
    while(n > 0){
        for (size_t i = 0; i < IMGWIDTH; i++)
            for (size_t j = 0; j < IMGWIDTH; j++)
                cab_id->buffers[n][i][j] = cab_id->buffers[n-1][i][j];    
        n--;
    }

    cab_id->nBuffers++;

    //fill buffer 0 with new data
    for (size_t i = 0; i < IMGWIDTH; i++)
        for (size_t j = 0; j < IMGWIDTH; j++)
            cab_id->buffers[0][i][j] = buf_pointer[i][j]; 

    free(buf_pointer);
}

int main(int argc, char const *argv[]){

    uint8_t ** img1 = (uint8_t**) malloc(IMGWIDTH*sizeof(uint8_t*));
    for (uint8_t j = 0; j < IMGWIDTH; j++)
        img1[j] = (uint8_t*)malloc(IMGWIDTH * sizeof(uint8_t));

    uint8_t vertical[5][5] = {{0,0,255,0,0}, 
                             {0,0,255,0,0},
                             {0,0,255,0,0},
                             {0,0,255,0,0},
                             {0,0,255,0,0}};

    for (size_t i = 0; i < IMGWIDTH; i++)
        for (size_t j = 0; j < IMGWIDTH; j++)
            img1[i][j] = vertical[i][j];
    
   
    // testing
    cab * cab1;
    cab1 = open_cab("images", 3, IMGWIDTH*IMGWIDTH, img1);
    printf("cab %s -> num=%ld, dim=%d, first[0][0][0]=%d\n", cab1->name, cab1->num, cab1->dim, cab1->buffers[0][0][2]);
    
    uint8_t ** free_buffer = reserve(cab1);


    uint8_t diagonal[5][5] = {{255,0,0,0,0}, 
                             {0,255,0,0,0},
                             {0,0,255,0,0},
                             {0,0,0,255,0},
                             {0,0,0,0,255}};

    for (size_t i = 0; i < IMGWIDTH; i++)
        for (size_t j = 0; j < IMGWIDTH; j++)
            free_buffer[i][j] = diagonal[i][j];

    for (size_t i = 0; i < IMGWIDTH; i++){
        for (size_t j = 0; j < IMGWIDTH; j++){
            printf("%d, ", cab1->buffers[0][i][j]);
        }
        printf("\n");
    }

    put_mes(free_buffer, cab1);

    for (size_t i = 0; i < IMGWIDTH; i++){
        for (size_t j = 0; j < IMGWIDTH; j++){
            printf("%d, ", cab1->buffers[0][i][j]);
        }
        printf("\n");
    }

    for (size_t i = 0; i < IMGWIDTH; i++){
        for (size_t j = 0; j < IMGWIDTH; j++){
            printf("%d, ", cab1->buffers[1][i][j]);
        }
        printf("\n");
    }


    free_buffer = reserve(cab1);

    uint8_t diagonal2[5][5] = {{0,0,0,0,255}, 
                             {0,0,0,255,0},
                             {0,0,255,0,0},
                             {0,255,0,0,0},
                             {255,0,0,0,0}};

    for (size_t i = 0; i < IMGWIDTH; i++)
        for (size_t j = 0; j < IMGWIDTH; j++)
            free_buffer[i][j] = diagonal2[i][j];
    
    put_mes(free_buffer, cab1);

    for (size_t i = 0; i < IMGWIDTH; i++){
        for (size_t j = 0; j < IMGWIDTH; j++){
            printf("%d, ", cab1->buffers[0][i][j]);
        }
        printf("\n");
    }

    for (size_t i = 0; i < IMGWIDTH; i++){
        for (size_t j = 0; j < IMGWIDTH; j++){
            printf("%d, ", cab1->buffers[1][i][j]);
        }
        printf("\n");
    }

    for (size_t i = 0; i < IMGWIDTH; i++){
        for (size_t j = 0; j < IMGWIDTH; j++){
            printf("%d, ", cab1->buffers[2][i][j]);
        }
        printf("\n");
    }
    
   


    
    return 0;
}
