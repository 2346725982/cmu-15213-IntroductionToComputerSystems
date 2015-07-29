#include "csapp.h"
#include "cache.h"

void init_list(cache_list* cl);
void free_list(cache_list* cl);
cache_block* find(cache_list* cl, char* request);
void write_cache(cache_list* cl, char* request, char* content,
		unsigned int size);

cache_block* new_cache(char* request, char* content, unsigned int size);
void delete_cache(cache_block* cb);
void insert_to_list(cache_list* cl, cache_block* cb);
void remove_from_list(cache_block* cb);

/* Gloabl Variables, lock*/
sem_t sem;

/* API call */
cache_block* find(cache_list *cl, char* request) {
	cache_block* cb;
	P(&sem);
	for (cb = cl->head->next; cb != cl->tail; cb = cb->next) {
		if (!strcmp(cb->request, request)) {
			V(&sem);
			return cb;
		}
	}
	V(&sem);
	return NULL;
}

void write_cache(cache_list* cl, char* request, char* content,
		unsigned int size) {

	P(&sem);
	cache_block* cb = new_cache(request, content, size);

	if (cl->total_size + cb->size < MAX_CACHE_SIZE) {
		insert_to_list(cl, cb);
	} else {
		remove_from_list(cl->tail->prev);
		insert_to_list(cl, cb);
	}
	V(&sem);
}

/* */

void init_list(cache_list* cl) {
	cl->total_size = 0;

	cl->head = new_cache(NULL, NULL, 0);
	cl->tail = new_cache(NULL, NULL, 0);

	cl->head->next = cl->tail;
	cl->tail->prev = cl->head;

	sem_init(&sem, 0, 1);
}

void free_list(cache_list* cl) {
	cache_block* cb;
	cache_block* tmp;

	for (cb = cl->head->next; cb != cl->tail;) {
		tmp =  cb->next;
		delete_cache(cb);
		cb = tmp;
	}
}

cache_block* new_cache(char* request, char* content, unsigned int size) {
	cache_block* cb;

	cb = malloc(sizeof(cache_block));

	cb->size = size;
	cb->prev = NULL;
	cb->next = NULL;

	if (request != NULL) {
		cb->request = (char *) malloc(sizeof(char) * (strlen(request) + 1));
		strcpy(cb->request, request);
	}

	if (content != NULL) {
		cb->content = (char *) malloc(sizeof(char) * size);
		memcpy(cb->content, content, sizeof(char) * size);
	}

	return cb;
}

void delete_cache(cache_block* cb) {
	free(cb->content);
	free(cb->request);
	free(cb);
}

void insert_to_list(cache_list* cl, cache_block* cb) {
	cb->next = cl->head->next;
	cb->prev = cl->head;

	cb->next->prev = cb;
	cl->head->next = cb;

	cl->total_size += cb->size;
}

void remove_from_list(cache_block* cb) {
	cb->prev->next = cb->next;
	cb->next->prev = cb->prev;

	delete_cache(cb);
}
