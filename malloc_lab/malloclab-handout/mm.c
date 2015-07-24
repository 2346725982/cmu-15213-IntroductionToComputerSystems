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
#define CHUNKSIZE   256

#define OVERHEAD    8

#define MAX(x, y) ((x) > (y)? (x) : (y))
#define MIN(x, y) ((x) < (y)? (x) : (y))

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
static void *pro_epi_louge;

/* Segregated lists define */
#define LIST_NUMBER 4

#define LIST0_HEAD 0 * MIN_BLK
#define LIST1_HEAD 1 * MIN_BLK
#define LIST2_HEAD 2 * MIN_BLK
#define LIST3_HEAD 3 * MIN_BLK

#define LIST0_SIZE 24
#define LIST1_SIZE 960
#define LIST2_SIZE 3840
#define LIST3_SIZE 30720 // To infinity

/* Function definition */
static void *coalesce(void *bp);
static void *extend_heap(size_t words);
static void *find_fit(size_t size);
static void *search_list(void* head, size_t size);
static void place(void* bp, size_t size);
static void add_to_list(void* bp, size_t size);
static void remove_from_list(void* bp);

/*
 * Initialize: return -1 on error, 0 on success.
 */
int mm_init(void) {
	heap_listp = NULL;
	pro_epi_louge = NULL;

	/* Segregated lists */
	if ((heap_listp = mem_sbrk(LIST_NUMBER * MIN_BLK)) == NULL)
		return -1;

	PUT(heap_listp + LIST0_HEAD, PACK(0, 0));
	GET_PREV(heap_listp + LIST0_HEAD + WSIZE) = NULL;
	GET_NEXT(heap_listp + LIST0_HEAD + WSIZE) = NULL;
	PUT(heap_listp + LIST0_HEAD + WSIZE + 2 * DSIZE, PACK(0, 0));

	PUT(heap_listp + LIST1_HEAD, PACK(0, 0));
	GET_PREV(heap_listp + LIST1_HEAD + WSIZE) = NULL;
	GET_NEXT(heap_listp + LIST1_HEAD + WSIZE) = NULL;
	PUT(heap_listp + LIST1_HEAD + WSIZE + 2 * DSIZE, PACK(0, 0));

	PUT(heap_listp + LIST2_HEAD, PACK(0, 0));
	GET_PREV(heap_listp + LIST2_HEAD + WSIZE) = NULL;
	GET_NEXT(heap_listp + LIST2_HEAD + WSIZE) = NULL;
	PUT(heap_listp + LIST2_HEAD + WSIZE + 2 * DSIZE, PACK(0, 0));

	PUT(heap_listp + LIST3_HEAD, PACK(0, 0));
	GET_PREV(heap_listp + LIST3_HEAD + WSIZE) = NULL;
	GET_NEXT(heap_listp + LIST3_HEAD + WSIZE) = NULL;
	PUT(heap_listp + LIST3_HEAD + WSIZE + 2 * DSIZE, PACK(0, 0));

	/* Prologue and Epilogue */
	if ((pro_epi_louge = mem_sbrk(4 * WSIZE)) == NULL)
		return -1;

	PUT(pro_epi_louge, 0); /* Alignment padding */
	PUT(pro_epi_louge + (1 * WSIZE), PACK(DSIZE, 1)); /* Prologue header */
	PUT(pro_epi_louge + (2 * WSIZE), PACK(DSIZE, 1)); /* Prologue footer */
	PUT(pro_epi_louge + (3 * WSIZE), PACK(0, 1)); /*Epiologue */

	/* Extend the empty heap with a free block of CHUNKSIZE bytes */
	if (extend_heap(CHUNKSIZE / WSIZE) == NULL) {
		return -1;
	}
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

	/* Search the free list for a fit */
	if ((bp = find_fit(asize)) != NULL) {
		place(bp, asize);
		return bp;
	}

	/* No fit found. Get more memory and place the block */
	extendsize = MAX(asize, CHUNKSIZE);
	if ((bp = extend_heap(extendsize / WSIZE)) == NULL) {
		return NULL;
	}
	place(bp, asize);

	return bp;

}

/*
 * free
 */
void free(void *bp) {
	//printf("free: bp = %p\n", bp);
	if (!bp)
		return;

	size_t size = GET_SIZE(HDRP(bp));

	PUT(HDRP(bp), PACK(size, 0));
	PUT(FTRP(bp), PACK(size, 0));

	//printf("free: add_to_list, bp = %p, size = %zd\n", bp, size);
	add_to_list(bp, size);

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
	GET_PREV(bp) = NULL;
	GET_NEXT(bp) = NULL;
	PUT(FTRP(bp), PACK(size, 0)); /* Free block footer */
	PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); /* New epilogue header */
	add_to_list(bp, size);

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
	size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));

	if (PREV_BLKP(bp) == heap_listp + LIST0_HEAD + WSIZE
			|| PREV_BLKP(bp) == heap_listp + LIST1_HEAD + WSIZE
			|| PREV_BLKP(bp) == heap_listp + LIST2_HEAD + WSIZE
			|| PREV_BLKP(bp) == heap_listp + LIST3_HEAD + WSIZE) {
		prev_alloc = 1;
	}

	size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
	size_t size = GET_SIZE(HDRP(bp));

	/* Case 1 */
	if (prev_alloc && next_alloc) {
		return bp;
	}
	/* Case 2 */
	else if (prev_alloc && !next_alloc) {
		size += GET_SIZE(HDRP(NEXT_BLKP(bp)));

		remove_from_list(bp);
		remove_from_list(NEXT_BLKP(bp));

		PUT(HDRP(bp), PACK(size, 0));
		PUT(FTRP(bp), PACK(size,0));
	}

	/* Case 3 */
	else if (!prev_alloc && next_alloc) {
		size += GET_SIZE(HDRP(PREV_BLKP(bp)));
		remove_from_list(bp);
		remove_from_list(PREV_BLKP(bp));
		PUT(FTRP(bp), PACK(size, 0));
		PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
		bp = PREV_BLKP(bp);
	}
	/* Case 4 */
	else {
		size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp)));
		remove_from_list(bp);
		remove_from_list(NEXT_BLKP(bp));
		remove_from_list(PREV_BLKP(bp));
		bp = PREV_BLKP(bp);
		PUT(HDRP(bp), PACK(size, 0));
		PUT(FTRP(bp), PACK(size, 0));
	}

	add_to_list(bp, size);
	return bp;
}

/* Place
 * PUT size at the header and the footer of a block.
 *
 */
static void place(void* bp, size_t size) {
	size_t newsize = size;
	size_t oldsize = GET_SIZE(HDRP(bp));

	remove_from_list(bp);

	if ((oldsize - newsize) >= MIN_BLK) {

		PUT(HDRP(bp), PACK(newsize, 1));
		PUT(FTRP(bp), PACK(newsize, 1));

		bp = NEXT_BLKP(bp);

		PUT(HDRP(bp), PACK(oldsize - newsize, 0));
		PUT(FTRP(bp), PACK(oldsize - newsize, 0));

		add_to_list(bp, oldsize - newsize);

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
	void *bp = NULL;

	if (size <= LIST0_SIZE) {
		bp = search_list(heap_listp + LIST0_HEAD, size);
	}
	if (bp != NULL) {
		return bp;
	}

	if (size <= LIST1_SIZE) {
		bp = search_list(heap_listp + LIST1_HEAD, size);
	}
	if (bp != NULL) {
		return bp;
	}

	if (size <= LIST2_SIZE) {
		bp = search_list(heap_listp + LIST2_HEAD, size);
	}
	if (bp != NULL) {
		return bp;
	}

	if (size <= LIST3_SIZE) {
		bp = search_list(heap_listp + LIST3_HEAD, size);
	}
	if (bp != NULL) {
		return bp;
	}

	return bp;
}

static void *search_list(void* head, size_t size) {
	void *bp;

	for (bp = head + WSIZE; bp != NULL; bp = GET_NEXT(bp)) {
		if (GET_SIZE(HDRP(bp)) >= size) {
			return bp;
		}
	}
	return NULL;

}

static void add_to_list(void* bp, size_t size) {
	void* head;

	if (size <= LIST0_SIZE) {
		head = heap_listp + LIST0_HEAD;
	} else if (size <= LIST1_SIZE) {
		head = heap_listp + LIST1_HEAD;
	} else if (size <= LIST2_SIZE) {
		head = heap_listp + LIST2_HEAD;
	} else {
		head = heap_listp + LIST3_HEAD;
	}
	head = head + WSIZE;
	GET_PREV(bp) = head;
	GET_NEXT(bp) = GET_NEXT(head);
	if (GET_NEXT(head) != NULL)
		GET_PREV(GET_NEXT(head)) = bp;
	GET_NEXT(head) = bp;
}

static void remove_from_list(void* bp) {
	GET_NEXT(GET_PREV(bp)) = GET_NEXT(bp);

	if (GET_NEXT(bp) != NULL)
		GET_PREV(GET_NEXT(bp)) = GET_PREV(bp);
}
