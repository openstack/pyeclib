#include<common.h>

int is_addr_aligned(unsigned int addr, int align)
{
  return (addr & (align-1)) == 0;
}

void *aligned_malloc( size_t size, int align )
{
    void *mem = malloc( size + (align-1) + sizeof(void*) );
    char *amem;
    if (!mem) {
      return NULL;
    }

    amem = ((char*)mem) + sizeof(void*);
    amem += align - ((unsigned long)amem & (align - 1));

    ((void**)amem)[-1] = mem;
    return amem;
}

void aligned_free( void *mem )
{
    free( ((void**)mem)[-1] );
}

