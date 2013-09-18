#ifndef _COMMON_H_
#define _COMMON_H_

#include<stdlib.h>

void *aligned_malloc( size_t size, int align );
void aligned_free( void *mem );
int is_addr_aligned(unsigned int addr, int align);


#endif
