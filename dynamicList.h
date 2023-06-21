#include <stdio.h>
#include <stdlib.h>

typedef struct ArrayList{
    int size;
    size_t data_size;
    union value* value;
}ArrayList;
union value{
        int i_val;
        double d_val;
        char* p_val;
        char c_val;
        float f_val;
}value;

void* initList();
void* addElement(ArrayList, void*, size_t);
void* removeElement(ArrayList, int);