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
    ""
};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)

#define MAX(x,y) ((x) > (y) ? (x) : (y))

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

/* my define */
#define WSIZE 4 /* Word and header(footer) sizes */
#define DSIZE 8 /* double size*/
#define CHUNKSIZE (1<<12) /* Extend heap by this*/

/* Packe a size and allocated bit into a word*/
#define PACK(size,alloc) ((size) | alloc)

/* Read and write a word at address p*/
#define GET(p) (*(unsigned int *)(p))
#define PUT(p,val) ((*(unsigned int *)(p)) = (val))

/* Read the size and allocated fields from address p */
#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)

/* Given block ptr bp , compute address of its header and footer*/
#define HDRP(bp) ((char *)(bp) - WSIZE)
#define FTRP(bp) ((char*)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

/* Given block ptr bp , compute address of next and previous blocks */
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE (((char * )(bp)-WSIZE)))
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE (((char * )(bp)-DSIZE)))

/* A private global vari , points prolog*/
static void *heap_listp; 

/* Create a new list pointer */
#define GET_HEAD(class_block) ((unsigned int *)(long)(GET(heap_listp + WSIZE * class_block)))
#define GET_PRE(bp) ((unsigned int *)(long)(GET(bp)))
#define GET_SUC(bp) ((unsigned int *)(long)(GET((unsigned int *)bp + 1)))
#define CLASS_SIZE 20
 
/*
 * find_classloc -search with bytes return for class
*/
int search (size_t size ){
    int i ;
    for ( i = 4 ; i <= 22 ;i++){
        if (size <= (1 << i ))
            return i - 4;
    }
    
    return i - 4;
}


void insert(void *bp){
    /* block size */
    size_t size = GET_SIZE(HDRP(bp));
    int class_block = search(size);
    if ( GET_HEAD(class_block) == NULL){
        PUT(heap_listp + class_block * WSIZE , bp);
        PUT(bp,NULL);
        PUT((unsigned int * )bp + 1 , NULL );
    }
    else {
        PUT((unsigned int *)(bp) + 1 , GET_HEAD(class_block));
        PUT(GET_HEAD(class_block) , bp);
        PUT(bp,NULL);
        PUT(heap_listp + WSIZE * class_block , bp);
    }
}

void delete (void *bp){
    size_t size = GET_SIZE(HDRP(bp));
    int class_block = search(size);
    /* the four casses of pointer node */
    if (GET_PRE(bp) == NULL && GET_SUC(bp) == NULL) { 
        PUT(heap_listp + WSIZE * class_block, NULL);
    } else if (GET_PRE(bp) != NULL && GET_SUC(bp) == NULL) {
        PUT(GET_PRE(bp) + 1, NULL);
    } else if (GET_SUC(bp) != NULL && GET_PRE(bp) == NULL){
        PUT(heap_listp + WSIZE * class_block, GET_SUC(bp));
        PUT(GET_SUC(bp), NULL);
    } else if (GET_SUC(bp) != NULL && GET_PRE(bp) != NULL) {
        PUT(GET_PRE(bp) + 1, GET_SUC(bp));
        PUT(GET_SUC(bp), GET_PRE(bp));
    }
}


/*
 * Coalesce - 
 */
static void *coalesce(void *bp){
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));
    /* case 1 */
    if ( prev_alloc && next_alloc ){
        insert(bp);
        return bp;
    }
    else if ( prev_alloc && !next_alloc ){
        delete(NEXT_BLKP(bp));
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp),PACK(size , 0));
        PUT(FTRP(bp),PACK(size , 0));
    }
    else if ( !prev_alloc && next_alloc){
        delete(PREV_BLKP(bp));
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(FTRP(bp),PACK(size,0));
        PUT(HDRP(PREV_BLKP(bp)),PACK(size,0));
        bp = PREV_BLKP(bp);
    }
    else if ( !prev_alloc && !next_alloc){
        delete(PREV_BLKP(bp));
        delete(NEXT_BLKP(bp));
        size += GET_SIZE(HDRP(NEXT_BLKP(bp))) + GET_SIZE(FTRP(PREV_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size,0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size,0));
        bp = PREV_BLKP(bp);
    }
    insert(bp);
    return bp;
}



/*
 * extend_heap - Extend the empty heap
 */
static void *extend_heap(size_t words){
    char *bp ;
    size_t size;

    /* Allocate an even number of words to maintain alignment*/
    size = (words % 2 ) ? (words + 1) * WSIZE : words * WSIZE;
    if ((long)(bp = mem_sbrk(size)) == -1)
        return NULL;
    
    /* Initialize free block header/footer and the epilogue header */
    PUT(HDRP(bp), PACK(size,0)); /* Free block header */
    PUT(FTRP(bp), PACK(size,0)); /* Free block footer */
    PUT(HDRP(NEXT_BLKP(bp)),PACK(0,1)); /* New epilogue header*/

    /* Coalesce if the previous block was free */
    return coalesce(bp);
}

/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    /* Creat the initial empty heap*/
    if ((heap_listp = mem_sbrk((4+CLASS_SIZE) * WSIZE)) == (void *)-1){
        return -1;
    }

    for (int i = 0 ; i < CLASS_SIZE ; i++){
        PUT(heap_listp + i*WSIZE,NULL);
    }

    PUT(heap_listp + CLASS_SIZE * WSIZE, 0 );
    PUT(heap_listp + ((1 + CLASS_SIZE)*WSIZE),PACK(DSIZE,1));
    PUT(heap_listp + ((2 + CLASS_SIZE)*WSIZE),PACK(DSIZE,1));
    PUT(heap_listp + ((3 + CLASS_SIZE)*WSIZE),PACK(0,1));

    /* Extend the empty heap with a free block of CHUNKSIZE bytes */
    if (extend_heap (CHUNKSIZE/WSIZE) == NULL)
        return -1;
    return 0;
}

/*
 * place - place the block into heap
*/
void place(void *bp , size_t asize ){
    size_t csize = GET_SIZE(HDRP(bp));

    delete(bp);
    if ( csize - asize >= 2 * DSIZE ){
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));
        /* bp points to unalloc block */
        bp = NEXT_BLKP(bp);
        PUT(HDRP(bp), PACK(csize - asize, 0));
        PUT(FTRP(bp), PACK(csize - asize, 0));
        /* insert unalloc block */
        insert(bp);
    }
    else{
        PUT(HDRP(bp),PACK(csize , 1));
        PUT(FTRP(bp),PACK(csize , 1 ));
    }
}

/*
 * find_fit - 
*/
void *find_fit(size_t size ){
    int class_block = search(size);
    unsigned int* bp;
    while (class_block < CLASS_SIZE){
        bp = GET_HEAD(class_block);
        while (bp){
            if (GET_SIZE(HDRP(bp)) >= size){
                return (void *)bp;
            }
            bp = GET_SUC(bp);
        }
        class_block++;
    }
    return NULL;
}
/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    size_t asize ;
    size_t extendsize ;
    char *bp ;

    /* Ignore spurious requests */
    if (size == 0 ){
        return NULL;
    }

    /* Adjust block size to includ overhead and aligment */
    if ( size <= DSIZE){
        asize = 2 * DSIZE ;
    }
    else {
        asize = DSIZE * ((size + (DSIZE ) + (DSIZE - 1 )) / DSIZE);
    }

    /* Search the free list for a fit */
    if ((bp = find_fit(asize)) != NULL){
        place(bp , asize );
        return bp ;
    }
    /* No fit found . Get more memory */
    extendsize = MAX(asize , CHUNKSIZE );
    if ((bp =  extend_heap(extendsize/WSIZE)) == NULL)
        return NULL;
    place(bp , asize );
    return bp ;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
    if (ptr == 0)
        return ;
    size_t size = GET_SIZE(HDRP(ptr));

    PUT(HDRP(ptr),PACK(size,0));
    PUT(FTRP(ptr),PACK(size,0));

    coalesce(ptr);
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    void *newptr;
    size_t copySize;
    
    if ((newptr = mm_malloc(size)) == NULL)
        return 0;
    copySize = GET_SIZE(HDRP(ptr));
    if (size < copySize)
        copySize = size;
    memcpy(newptr, ptr, copySize);
    mm_free(ptr);
    return newptr;
}














