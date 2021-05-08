/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 * 
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "ateam",
    /* First member's full name */
    "Harry Bovik",
    /* First member's email address */
    "bovik@cs.cmu.edu",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~0x7)

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

#define WSIZE 4
#define DSIZE 8
#define CHUNKSIZE (1 << 12)

#define MAX(x, y) ((x) > (y) ? (x) : (y))
#define PACK(size, alloc) ((size) | (alloc))

#define GET(p) (*(unsigned int *)(p))
#define PUT(p, val) (*(unsigned int *)(p) = (val))

#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)
#define GET_PREALLOC(p) (GET(p) & 0x2)

#define HDRP(bp) ((char *)(bp)-WSIZE)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp)-WSIZE)))
#define PREV_BLKP(bp) ((char *)(bp)-GET_SIZE(((char *)(bp)-DSIZE)))

static void *heap_listp;
static void *coalesce(void *);
static void *extend_heap(size_t);
static void place(void *, size_t);
static void *find_fit(size_t);

/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    if ((heap_listp = mem_sbrk(4 * WSIZE)) == (void *)-1)
        return -1;
    PUT(heap_listp, 0);
    PUT(heap_listp + (1 * WSIZE), PACK(DSIZE, 1));
    PUT(heap_listp + (2 * WSIZE), PACK(DSIZE, 1));
    PUT(heap_listp + (3 * WSIZE), PACK(0, 1));
    heap_listp += (2 * WSIZE);

    if (extend_heap(CHUNKSIZE / WSIZE) == NULL)
        return -1;
    return 0;
}

static void *extend_heap(size_t words)
{
    char *bp;
    size_t size = 0;
    size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;
    if ((long)(bp = mem_sbrk(size)) == -1)
        return NULL;
    PUT(HDRP(bp), PACK(size, 0)); //free block header
    PUT(FTRP(bp), PACK(size, 0)); //free block footer
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));

    return coalesce(bp);
    // return bp;
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    if (size == 0)
        return NULL;
    size_t newsize = ALIGN(size + 2 * WSIZE);
    char *bp = NULL;
    size_t extendsize = 0;

    if ((bp = find_fit(newsize)) != NULL)
    {
        place(bp, newsize);
        return bp;
    }

    extendsize = MAX(newsize, CHUNKSIZE);
    if ((bp = extend_heap(extendsize / WSIZE)) == NULL)
        return NULL;
    place(bp, newsize);
    return bp;
}

static void *find_fit(size_t size)
{
    char *now_ptr = heap_listp;
    size_t asize = 0;
    while ((asize = GET_SIZE(HDRP(now_ptr))) != 0) //不等于末尾块
    {
        if (!GET_ALLOC(HDRP(now_ptr)) && asize >= size)
        {
            return now_ptr;
        }
        now_ptr = NEXT_BLKP(now_ptr);
    }
    return NULL;
}

static void place(void *ptr, size_t size)
{
    size_t capacity = GET_SIZE(HDRP(ptr));
    if (capacity - size < 2 * DSIZE) //剩下的边角料小于最小块的话, 分割下来也没用,索性都用了
    {
        PUT(HDRP(ptr), PACK(capacity, 1));
        PUT(FTRP(ptr), PACK(capacity, 1));
    }
    else
    {
        PUT(HDRP(ptr), PACK(size, 1));
        PUT(FTRP(ptr), PACK(size, 1));

        PUT(HDRP(NEXT_BLKP(ptr)), PACK(capacity - size, 0));
        PUT(FTRP(NEXT_BLKP(ptr)), PACK(capacity - size, 0));
    }
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
    // static int cnt = 0;
    // cnt++;
    // printf("%d\n", cnt);
    size_t size = GET_SIZE(HDRP(ptr));
    PUT(HDRP(ptr), PACK(size, 0));
    PUT(FTRP(ptr), PACK(size, 0));

    coalesce(ptr);
}

static void *coalesce(void *ptr)
{
    // if (GET_ALLOC(ptr) == 1) //allocated block
    //     return NULL;

    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(ptr)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(ptr)));
    size_t size = GET_SIZE(HDRP(ptr));

    if (prev_alloc && next_alloc)
    {
        return ptr;
    }
    else if (prev_alloc && !next_alloc)
    {
        size_t next_size = GET_SIZE(HDRP(NEXT_BLKP(ptr)));
        size_t new_size = next_size + size;
        PUT(HDRP(ptr), PACK(new_size, 0));
        PUT(FTRP(ptr), PACK(new_size, 0));
    }
    else if (!prev_alloc && next_alloc)
    {
        size_t prev_size = GET_SIZE(HDRP(PREV_BLKP(ptr)));
        size_t new_size = prev_size + size;
        PUT(FTRP(ptr), PACK(new_size, 0));
        PUT(HDRP(PREV_BLKP(ptr)), PACK(new_size, 0));
        ptr = PREV_BLKP(ptr);
    }
    else
    {
        size_t prev_size = GET_SIZE(HDRP(PREV_BLKP(ptr)));
        size_t next_size = GET_SIZE(FTRP(NEXT_BLKP(ptr)));
        size_t new_size = prev_size + size + next_size;
        PUT(HDRP(PREV_BLKP(ptr)), PACK(new_size, 0));
        PUT(FTRP(NEXT_BLKP(ptr)), PACK(new_size, 0));
        ptr = PREV_BLKP(ptr);
    }
    return ptr;
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    size = ALIGN(size);
    if (ptr == NULL)
    {
        return mm_malloc(size);
    }
    if (size == 0)
    {
        mm_free(ptr);
        return NULL;
    }
    void *newptr;

    size_t copySize;
    copySize = GET_SIZE(HDRP(ptr)) - 2 * WSIZE;
    if (size <= copySize) //如果还没原来的大, 不需要新块
    {
        size_t full_size = size + 2 * WSIZE;
        place(ptr, full_size);
        newptr = ptr;
    }
    else
    {
        size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(ptr)));
        size_t prev_alloc = GET_ALLOC(HDRP(PREV_BLKP(ptr)));
        if (!next_alloc && GET_SIZE(HDRP(NEXT_BLKP(ptr))) + copySize >= size)
        {
            size_t total_size = GET_SIZE(HDRP(NEXT_BLKP(ptr))) + copySize + 2 * WSIZE;
            size_t remain_size = total_size - size - 2 * WSIZE;
            if (remain_size >= 2 * DSIZE)
            {
                PUT(HDRP(ptr), PACK(size + 2 * WSIZE, 1));
                PUT(FTRP(ptr), PACK(size + 2 * WSIZE, 1));

                PUT(HDRP(NEXT_BLKP(ptr)), PACK(remain_size, 0));
                PUT(FTRP(NEXT_BLKP(ptr)), PACK(remain_size, 0));
            }
            else
            {
                PUT(HDRP(ptr), PACK(total_size, 1));
                PUT(FTRP(ptr), PACK(total_size, 1));
            }
            newptr = ptr;
        }
        // else if ((!prev_alloc) * GET_SIZE(HDRP(PREV_BLKP(ptr))) +
        //              (!next_alloc) * GET_SIZE(HDRP(NEXT_BLKP(ptr))) + copySize >=
        //          size)
        // {
        //     newptr = coalesce(ptr);
        //     size_t total_size = GET_SIZE(HDRP(newptr));
        //     size_t remain_size = total_size - size - 2 * WSIZE;
        //     memcpy(newptr, ptr, copySize);

        //     if (remain_size >= 2 * DSIZE)
        //     {
        //         PUT(HDRP(newptr), PACK(size + 2 * WSIZE, 1));
        //         PUT(FTRP(newptr), PACK(size + 2 * WSIZE, 1));

        //         PUT(HDRP(NEXT_BLKP(newptr)), PACK(remain_size, 0));
        //         PUT(FTRP(NEXT_BLKP(newptr)), PACK(remain_size, 0));
        //     }
        //     else
        //     {
        //         PUT(HDRP(newptr), PACK(total_size, 1));
        //         PUT(FTRP(newptr), PACK(total_size, 1));
        //     }
        // }
        else
        {
            newptr = mm_malloc(size);
            if (newptr == NULL)
                return NULL;
            memcpy(newptr, ptr, copySize);
            mm_free(ptr);
        }
    }

    return newptr;
}
