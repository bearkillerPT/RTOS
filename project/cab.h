#include<stdint.h>
#include<stddef.h>

typedef struct cab cab;

cab * open_cab(char * name, int num, size_t dim, void* first);

uint8_t ** reserve(cab * cab_id);

void put_mes (void* buf_pointer, cab * cab_id); 

uint8_t ** get_mes (cab * cab_id);

void unget (* mes_pointer, cab * cab_id); 


