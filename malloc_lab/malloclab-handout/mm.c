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
#define MIN_BLK     24      /* Header, footer, prev ptr, next ptr */
//#define CHUNKSIZE  ((1<<12)-DSIZE) /* Extend heap by this amount (bytes) */
#define CHUNKSIZE   308

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

/* Given block ptr bp, put address of next and previous blocks */
#define GET_NEXT(bp) (*(void **)(bp + DSIZE))
#define GET_PREV(bp) (*(void **)(bp))

/* Given block ptr bp, get address of next and previous blocks */
#define PUT_NEXT(bp, next) (* (char *)(GET_NEXT(bp)) = (intptr_t)next)
#define PUT_PREV(bp, prev) (* (char *)(GET_PREV(bp)) = (intptr_t)prev)

/* Global variables-- Base of the heap(heap_listp) */
static void *heap_listp;
static void *free_listp;

/* Function definition */
static void *coalesce(void *bp);
static void *extend_heap(size_t words);
static void *find_fit(size_t size);
static void place(void* bp, size_t size);
static void join_prev_next(void* bp);
static void move_to_root(void* bp);

/*
 * Initialize: return -1 on error, 0 on success.
 */
int mm_init(void) {

	if ((heap_listp = mem_sbrk(2 * MIN_BLK)) == NULL)
		return -1;

	/* Prologue and Epilogue */
	PUT(heap_listp, 0); /* Alignment padding */
	PUT(heap_listp + (1 * WSIZE), PACK(MIN_BLK, 1)); /* Prologue header */
	PUT(heap_listp + (2 * WSIZE), 0); /* Prev free block = NULL */
	PUT(heap_listp + (3 * WSIZE), 0); /* Next free block = NULL */
	PUT(heap_listp + MIN_BLK, PACK(MIN_BLK, 1)); /* Prologue footer */

	PUT(heap_listp + MIN_BLK + WSIZE, PACK(0, 1)); /*Epiologue */

	free_listp = heap_listp + DSIZE;
//	printf("Finish initialization.\n");
	/* Extend the empty heap with a free block of CHUNKSIZE bytes */
//	if ((heap_listp = extend_heap(CHUNKSIZE / WSIZE)) == NULL)
	if (extend_heap(CHUNKSIZE / WSIZE) == NULL)
	{
//		printf("Extend heap first time Error.\n");

		return -1;
	}
//	printf("Finish mm_init. New exiting.\n");
	return 0;
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
	asize = MAX(ALIGN(size) + DSIZE, MIN_BLK);

//	if (size <= DSIZE)
//		asize = 2 * DSIZE;
//	else
//		asize = DSIZE * ((size + (DSIZE) + (DSIZE - 1)) / DSIZE);

//	printf("In malloc, before find fit.\n");
	/* Search the free list for a fit */
	if ((bp = find_fit(asize)) != NULL) {
		place(bp, asize);
		return bp;
	}

//	printf("In malloc, before extend heap.\n");
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
	if (!bp)
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
	size_t asize = MAX(ALIGN(size) + DSIZE, MIN_BLK);

	/* If oldptr is NULL, then this is just malloc. */
	if (oldptr == NULL) {
		return malloc(size);
	}

	/* If size == 0 then this is just free, and we return NULL. */
	if (size == 0) {
		free(oldptr);
		return NULL;
	}

	/* Copy the old data. */
	oldsize = GET_SIZE(HDRP(oldptr));
	if (asize == oldsize)
		return oldptr;
	else if (asize < oldsize) {
		if (oldsize - asize <= MIN_BLK)
			return oldptr;

		PUT(HDRP(oldptr), PACK(asize, 1));
		PUT(FTRP(oldptr), PACK(asize, 1));
		PUT(HDRP(NEXT_BLKP(oldptr)), PACK(oldsize-asize, 1));
		free(NEXT_BLKP(oldptr));
		return oldptr;
	}

	newptr = malloc(size);

	/* If realloc() fails the original block is left untouched  */
	if (!newptr) {
		return NULL;
	}

	/* Copy the old data */
	if(size < oldsize)
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
	if (size < MIN_BLK)
		size = MIN_BLK;
	if ((long) (bp = mem_sbrk(size)) == -1)
		return NULL;

	/* Initialize free block header/footer and the epilogue header */
	PUT(HDRP(bp), PACK(size, 0)); /* Free block header */
	PUT(FTRP(bp), PACK(size, 0)); /* Free block footer */
	PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); /* New epilogue header */

//	printf("Extend heap. Right before coalesce.\n");
	/* Coalesce if the previous block was free */
	return coalesce(bp);
}

/* Coalesce
 * Four cases:
 * 1. bp has no free previous blocks or next blocks.
 * 2. bp has a free previous block.
 * 3. bp has a free next block.
 * 4. bp has free previous blocks and next blocks.
 */
static void *coalesce(void *bp) {
	size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp))) || PREV_BLKP(bp) == bp;
	size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
	size_t size = GET_SIZE(HDRP(bp));

	/* Case 1 */
	if (prev_alloc && next_alloc) {
		move_to_root(bp);
		return bp;
	}
	/* Case 2 */
	else if (prev_alloc && !next_alloc) {
		size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
		join_prev_next(NEXT_BLKP(bp));
		PUT(HDRP(bp), PACK(size, 0));
		PUT(FTRP(bp), PACK(size,0));
	}

	/* Case 3 */
	else if (!prev_alloc && next_alloc) {
		size += GET_SIZE(HDRP(PREV_BLKP(bp)));
		join_prev_next(PREV_BLKP(bp));
		PUT(FTRP(bp), PACK(size, 0));
		PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
		bp = PREV_BLKP(bp);
	}
	/* Case 4 */
	else {
		size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp)));
		join_prev_next(NEXT_BLKP(bp));
		join_prev_next(PREV_BLKP(bp));
		bp = PREV_BLKP(bp);
		PUT(HDRP(bp), PACK(size, 0));
		PUT(FTRP(bp), PACK(size, 0));
	}
	move_to_root(bp);
	return bp;
}

/* Place
 * PUT size at the header and the footer of a block.
 *
 */
static void place(void* bp, size_t size) {
	size_t newsize = size;
	size_t oldsize = GET_SIZE(HDRP(bp));

	if ((oldsize - newsize) >= MIN_BLK) {
		PUT(HDRP(bp), PACK(newsize, 1));
		PUT(FTRP(bp), PACK(newsize, 1));
		join_prev_next(bp);

		bp = NEXT_BLKP(bp);
		PUT(HDRP(bp), PACK(oldsize - newsize, 0));
		PUT(FTRP(bp), PACK(oldsize - newsize, 0));
		coalesce(bp);
	} else {
		PUT(HDRP(bp), PACK(oldsize, 1));
		PUT(FTRP(bp), PACK(oldsize, 1));
		join_prev_next(bp);
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
  /* First fit */
	void *bp;

	for (bp = free_listp; GET_ALLOC(HDRP(bp)) == 0; bp = GET_NEXT(bp)) {
		if (GET_SIZE(HDRP(bp)) >= size)
			return bp;
	}
	return NULL;

//	/* Next fit. */
//	void *first;
//	void *second;
//
//	for (first = free_listp; GET_ALLOC(HDRP(first)) == 0; first = GET_NEXT(first)) {
//		if (GET_SIZE(HDRP(first)) >= size)
//			break;
//	}
//
//	for (second = first; GET_ALLOC(HDRP(second)) == 0; second = GET_NEXT(second)) {
//			if (GET_SIZE(HDRP(second)) >= size)
//				return second;
//		}
//
//	if (GET_ALLOC(HDRP(first)) == 0)
//		return first;
//
//	return NULL;

//	/* Best fit */
//	void *best = free_listp;
//	void *iter;
//	for (iter = free_listp; GET_ALLOC(HDRP(iter)) == 0; iter = GET_NEXT(iter)) {
//			if (GET_SIZE(HDRP(iter)) >= size)
//				if (GET_SIZE(HDRP(iter)) < GET_SIZE(HDRP(best)))
//					best = iter;
//		}
//
//	if (best != free_listp)
//		return best;
//	return NULL;
}

static void join_prev_next(void* bp) {
	if (GET_PREV(bp) != NULL)
		GET_NEXT(GET_PREV(bp)) = GET_NEXT(bp);
	else
		free_listp = GET_NEXT(bp);

	GET_PREV(GET_NEXT(bp)) = GET_PREV(bp);
}

static void move_to_root(void* bp) {
	GET_NEXT(bp) = free_listp;
	GET_PREV(free_listp) = bp;
	GET_PREV(bp) = NULL;
	free_listp = bp;
}
