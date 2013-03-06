#include <stdio.h>
void* base = NULL; // beginning of the heap

typedef struct header *header_p;
// Struct with metainformation for the heap
struct header
{
    size_t    size;  // 1 bit shows if this block free
    header_p  next;  // pointer to next block
    header_p  prev;  // pointer to prev block
    int       magic; // must be equal to 0xdeadbeef
};

#define HEADER_SIZE (sizeof(struct header)) 
#define align4(s) (((((s) - 1)>> 2) << 2) + 4)
#define align4096(s) (((((s) - 1) >> 12) << 12) + 4096)
#define MAGIC 0xdeadbeef

inline size_t size(header_p h)
{
    return h->size & (~1);
}

inline int is_free(header_p h)
{
    return h->size & 1;
}

inline void set_free(header_p h)
{
    h->size |= 1;
}

inline void set_used(header_p h)
{
    h->size &= (~1);
}

header_p extend_heap(size_t size, header_p last)
{
    void* heap_br = sbrk(0);
    size_t s = align4096(size + HEADER_SIZE);
    printf("extending heap by %d byte\n", s);
    if (sbrk(s) == (void *)(-1))
        return NULL;
    header_p new_header = heap_br;
    new_header->size = (s - HEADER_SIZE) | 1;
    new_header->next = NULL;
    new_header->prev = last;
    if (last != NULL)
        last->next = new_header;
    new_header->magic = MAGIC;
    
    return new_header;
}

header_p find_fit_block(size_t size)
{
    header_p last = base;
    while(last != NULL)
    {
        if (is_free(last) && last->size >= size + HEADER_SIZE)
            return last;
        if (last->next == NULL) {
            last->next = extend_heap(size + HEADER_SIZE, last);
        }
        last = last->next;
    }
    return NULL;
}

void split_blocks(header_p b, size_t s)
{
    printf("spliting block %p(%d) by %d and %d\n", b, b->size, s, b->size - s - HEADER_SIZE);
    void *ptr = ((void*)b) + s + HEADER_SIZE;
    header_p new_block = ptr;

    new_block->next = b->next;
    new_block->size = (b->size - s - HEADER_SIZE) | 1;
    new_block->magic = MAGIC;
    new_block->prev = b;
    b->next = new_block;
    b->size = s;
}

void* Emalloc(size_t size)
{
    printf("allocate %d bytes\n", size);
    int s = align4(size);
    header_p block;
    if (base == NULL)
    {
        block = extend_heap(s, NULL);
        if (block == NULL)
            return NULL;
        base = block;
    } else {
        block = find_fit_block(s);
        if (block == NULL)
            return NULL;
    }

    set_used(block);
    if (block->size > s + HEADER_SIZE)
        split_blocks(block, s);
    
    printf("return block at %p with size %d\n", block, block->size);
    return ((void *)block) + HEADER_SIZE;
}

header_p merge(header_p a, header_p b)
{
    printf("merging blocks %p %p\n", a, b);
    a->size += size(b) + HEADER_SIZE;
    a->next = b->next;
    return a;
}

void Efree(void* ptr)
{
    printf("free %p\n", ptr);
    if (ptr < base + HEADER_SIZE || ptr >= sbrk(0))
        return;
    header_p block = ptr - HEADER_SIZE;
    if (block->magic != MAGIC)
        return;
    printf("%p is correct pointer\n", ptr);
    block->size |= 1;
    if (block->next && is_free(block->next)) 
    {
        printf("merging %p %p\n", block, block->next);
        merge(block, block->next);
        block->size += size(block->next);
        block->next = block->next->next;
    }
    if (block->prev && is_free(block->prev))
    {
        printf("merging %p %p\n", block, block->prev);
        block->prev->size += size(block);
        block->prev->next = block->next;
    }
//        block = merge(block, block->next);
}

int main()
{
    printf("base - %p\n", sbrk(0));
    void * ptr = Emalloc(30);
    void * p = Emalloc(10);
    //void * n = Emalloc(6);
    //Efree(ptr);
    //Efree(n);
    //Efree(p);
    printf("heap break - %p\n", sbrk(0));
    return 0;
}

