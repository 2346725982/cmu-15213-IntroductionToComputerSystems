/*
 * mm.c
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "mm.h"
#include "memlib.h"

/* If you want debugging output, use the following macro.  When you hand
 * in, remove the #define DEBUG line. */
#define DEBUG
#ifdef DEBUG
# define dbg_printf(...) printf(__VA_ARGS__)
#else
# define dbg_printf(...)
#endif

/* do not change the following! */
#ifdef DRIVER
/* create aliases for driver tests */
#define malloc mm_malloc
#define free mm_free
#define realloc mm_realloc
#define calloc mm_calloc
#endif /* def DRIVER */

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(p) (((size_t)(p) + (ALIGNMENT-1)) & ~0x7)

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

#define SIZE_PTR(p)  ((size_t*)(((char*)(p)) - SIZE_T_SIZE))

/* Basic constants and macros */
#define WSIZE       4       /* Word and header/footer size (bytes) */
#define DSIZE       8       /* Double word size (bytes) */

#define CHUNKSIZE  ((1<<12)-DSIZE) /* Extend heap by this amount (bytes) */

#define OVERHEAD    8

#define MAX(x, y) ((x) > (y)? (x) : (y))

/* Pack a size and allocated bit into a word */
#define PACK(size, alloc)  ((size) | (alloc))

/* Read and write a word at address p */
#define GET(p)       (*(unsigned int *)(p))
#define PUT(p, val)  (*(unsigned int *)(p) = (val))

/* Read the size and allocated fields from address p */
#define GET_SIZE(p)  (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)

/* Given block ptr bp, compute address of its header and footer */
#define HDRP(bp) ((char *)(bp) - WSIZE)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp))- DSIZE)

/* Given block ptr bp, compute address of next and previous blocks */
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))

/* Global variables-- Base of the heap(heap_listp) */
static void *heap_listp;

/* Coalesce
 * Four cases:
 * 1. bp has no free previous blocks or next blocks.
 * 2. bp has a free previous block.
 * 3. bp has a free next block.
 * 4. bp has free previous blocks and next blocks.
 */
static void *coalesce(void *bp) {
	size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
	size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
	size_t size = GET_SIZE(HDRP(bp));

	/* Case 1 */
	if (prev_alloc && next_alloc) {
		return bp;
	}
	/* Case 2 */
	else if (prev_alloc && !next_alloc) {
		size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
		PUT(HDRP(bp), PACK(size, 0));
		PUT(FTRP(bp), PACK(size,0));
	}

	/* Case 3 */
	else if (!prev_alloc && next_alloc) {
		size += GET_SIZE(HDRP(PREV_BLKP(bp)));
		PUT(FTRP(bp), PACK(size, 0));
		PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
		bp = PREV_BLKP(bp);
	}
	/* Case 4 */
	else {
		size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp)));
		PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
		PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
		bp = PREV_BLKP(bp);

	}
	return bp;
}

/* Extend Heap
 * The extend_heap function is invoked in two different circumstances:
 * (1) when the heap is initialized, and
 * (2) when mm_malloc is unable to find a suitable fit
 */
static void *extend_heap(size_t words) {
	char *bp;
	size_t size;

	/* Allocate an even number of words to maintain alignment */
	size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;
	if ((long) (bp = mem_sbrk(size)) == -1)
		return NULL;

	/* Initialize free block header/footer and the epilogue header */
	PUT(HDRP(bp), PACK(size, 0)); /* Free block header */
	PUT(FTRP(bp), PACK(size, 0)); /* Free block footer */
	PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); /* New epilogue header */

	/* Coalesce if the previous block was free */
	return coalesce(bp);
}

/*
 * Initialize: return -1 on error, 0 on success.
 */
int mm_init(void) {

	if ((heap_listp = mem_sbrk(4 * WSIZE)) == (void *) -1)
		return -1;

	PUT(heap_listp, 0); /* Alignment padding */
	PUT(heap_listp + (1*WSIZE), PACK(DSIZE, 1)); /* Prologue header */
	PUT(heap_listp + (2*WSIZE), PACK(DSIZE, 1)); /* Prologue footer */
	PUT(heap_listp + (3*WSIZE), PACK(0, 1)); /* Epilogue header */
	heap_listp += (2 * WSIZE);

	/* Extend the empty heap with a free block of CHUNKSIZE bytes */
	if ((heap_listp = extend_heap(CHUNKSIZE / WSIZE)) == NULL)
		return -1;
	return 0;
}

/* Place
 * PUT size at the header and the footer of a block.
 *
 */
static void place(void* bp, size_t newsize) {
//	size_t newsize = size; //((size + 1) >> 1) << 1;
	size_t oldsize = GET_SIZE(HDRP(bp));
//	int oldsize = *bp & -2;
//	*bp = newsize | 1;
//	if (newsize < oldsize)
//		*(bp + newsize) = oldsize - newsize;

	if ((oldsize - newsize) >= (DSIZE + OVERHEAD)) {
		PUT(HDRP(bp), PACK(newsize, 1));
		PUT(FTRP(bp), PACK(newsize, 1));
		bp = NEXT_BLKP(bp);
		PUT(HDRP(bp), PACK(oldsize - newsize, 0));
		PUT(FTRP(bp), PACK(oldsize - newsize, 0));
	} else {
		PUT(HDRP(bp), PACK(oldsize, 1));
		PUT(FTRP(bp), PACK(oldsize, 1));
	}

	return;
}

/* Find fit
 * Fint block fit for the size.
 * Different strategies are:
 * 1. First/next fit;
 * 2. Best fit;
 * 3. First fit in sorted address.
 */
static void *find_fit(size_t size) {
//	void *bp = heap_listp;
//	(bp < CHUNKSIZE)
//	while (GET_ALLOC(bp) || (GET_SIZE(bp) < size)) {
//		if (GET_SIZE(bp) == 0)
//			return NULL;
//		bp = NEXT_BLKP(bp);
//	}
//
//	return bp;

	void *bp;
	for (bp = heap_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)) {
		if (!GET_ALLOC(HDRP(bp)) && GET_SIZE(HDRP(bp)) >= size)
			return bp;
	}
	return NULL;
}

/*
 * malloc
 */
void *malloc(size_t size) {

	size_t asize; /* Adjusted block size */
	size_t extendsize;/* Amount to extend heap if no fit */
	char *bp;

	/* Ignore spurious requests */
	if (size <= 0)
		return NULL;

	/* Adjust block size to include overhead and alignment reqs. */
	if (size <= DSIZE)
		asize = 2 * DSIZE;
	else
		asize = DSIZE * ((size + (DSIZE) + (DSIZE - 1)) / DSIZE);
	/* Search the free list for a fit */
	if ((bp = find_fit(asize)) != NULL) {
		place(bp, asize);
		return bp;
	}
	/* No fit found. Get more memory and place the block */
	extendsize = MAX(asize, CHUNKSIZE);
	if ((bp = extend_heap(extendsize / WSIZE)) == NULL)
		return NULL;
	place(bp, asize);
	return bp;

}

/*
 * free
 */
void free(void *bp) {
	if(!bp)
		return;

	size_t size = GET_SIZE(HDRP(bp));

	PUT(HDRP(bp), PACK(size, 0));
	PUT(FTRP(bp), PACK(size, 0));

	coalesce(bp);
}

/*
 * realloc - you may want to look at mm-naive.c
 */
void *realloc(void *oldptr, size_t size) {
	/* Copy from mm-naive.c. Waiting to be modified */

	size_t oldsize;
	void *newptr;

	/* If oldptr is NULL, then this is just malloc. */
	if (oldptr == NULL) {
		return malloc(size);
	}

	/* If size == 0 then this is just free, and we return NULL. */
	if (size == 0) {
		free(oldptr);
		return NULL;
	}

	newptr = malloc(size);

	/* If realloc() fails the original block is left untouched  */
	if (!newptr) {
		return NULL;
	}

	/* Copy the old data. */
	oldsize = *SIZE_PTR(oldptr);
	if (size < oldsize)
		oldsize = size;
	memcpy(newptr, oldptr, oldsize);

	/* Free the old block. */
	free(oldptr);

	return newptr;
}

/*
 * calloc - you may want to look at mm-naive.c
 * This function is not tested by mdriver, but it is
 * needed to run the traces.
 */
void *calloc(size_t nmemb, size_t size) {

	/* Copy from mm-naive.c. Waiting to be modified */
	size_t bytes = nmemb * size;
	void *newptr;

	newptr = malloc(bytes);
	memset(newptr, 0, bytes);

	return newptr;
}

///*
// * Return whether the pointer is in the heap.
// * May be useful for debugging.
// */
//static int in_heap(const void *p) {
//	return p <= mem_heap_hi() && p >= mem_heap_lo();
//}
//
///*
// * Return whether the pointer is aligned.
// * May be useful for debugging.
// */
//static int aligned(const void *p) {
//	return (size_t) ALIGN(p) == (size_t) p;
//}

/*
 * mm_checkheap
 */
void mm_checkheap(int lineno) {
	/*Get gcc to be quiet. */
	lineno++;
	lineno--;
}
