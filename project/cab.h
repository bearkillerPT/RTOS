#include<stdint.h>
#include<stddef.h>

typedef struct cab cab;

cab * open_cab(char * name, size_t num, uint8_t dim, uint8_t ** first);

uint8_t ** reserve(cab * cab_id);

void put_mes (uint8_t ** buf_pointer, cab * cab_id); 

uint8_t ** get_mes (cab * cab_id);

void unget (uint8_t ** mes_pointer, cab * cab_id); 


