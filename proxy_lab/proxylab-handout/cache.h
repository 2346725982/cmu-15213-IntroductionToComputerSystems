#ifndef CACHE_H
#define CACHE_H

#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

typedef struct cacheblock {
	char* request;
	char* content;
	unsigned int size;
	struct cacheblock* prev;
	struct cacheblock* next;

} cache_block;

typedef struct cachelist {
	unsigned int total_size;
	cache_block* head;
	cache_block* tail;
} cache_list;

void init_list(cache_list* cl);
void free_list(cache_list* cl);
cache_block* find(cache_list* cl, char* request);
void write_cache(cache_list* cl, char* request, char* content, unsigned int size);

#endif
