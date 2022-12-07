#include <stdint.h>
#include <stddef.h>
#include "cab.h"

struct cab {
   char * name;
   size_t num;
   uint8_t dim;
   uint8_t *** first;
};

cab * open_cab(char * name, size_t num, uint8_t dim, uint8_t ** first){

}

int main(int argc, char const *argv[]){
    
    return 0;
}
