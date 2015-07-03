#include <getopt.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/queue.h>
#include "cachelab.h"

int hits;
int miss;
int evict;

struct Line {
	unsigned int tag;
	LIST_ENTRY(Line)
	list_entry;
};

struct Set {
	unsigned int size;
	LIST_HEAD(line_head, Line)
	head;
};

struct Cache {
	struct Set* sets;
};

char** tokenize(char* line, char* ins, unsigned int addr, unsigned int block) {
	char **array = malloc(3 * sizeof(char *));
	char *token;
	int i;

	i = 0;
	token = strtok(line, " ,");
	while (token != NULL) {
		array[i] = token;
		token = strtok(NULL, " ,");
		i += 1;
	}

	return array;
}

unsigned int getTagBit(unsigned int address, int s, int b) {
	return address >> (s + b);
}

unsigned int getSetIndex(unsigned int address, int s, int b) {
	int mask;

	mask = (1 << (s + b)) - 1;

	return (address & mask) >> b;
}

bool addressInCache(struct Set* set, int s, int E, unsigned int tag) {
	struct Line* tmp;
	LIST_FOREACH(tmp, &set->head, list_entry)
	{
		if (tmp->tag == tag)
			return true;
	}
	return false;
}

void evictFromCache(struct Set* set) {
	set->size -= 1;
	struct Line* tmp;

	tmp = LIST_FIRST(&set->head);
	while (LIST_NEXT(tmp, list_entry) != NULL) {
		tmp = LIST_NEXT(tmp, list_entry);
	}

	//printf("list tail tag=%d\n", tmp->tag);
	LIST_REMOVE(tmp, list_entry);
}

void addToCache(struct Set* set, unsigned int tag) {
	set->size += 1;
	struct Line* line = malloc(sizeof(struct Line));
	line->tag = tag;
	LIST_INSERT_HEAD(&set->head, line, list_entry);
}

void updateCache(struct Set* set, unsigned int tag) {
	struct Line* result;
	struct Line* tmp;
	LIST_FOREACH(tmp, &set->head, list_entry)
	{
		if (tmp->tag == tag)
			result = tmp;
	}

	LIST_REMOVE(result, list_entry);

	struct Line* new = malloc(sizeof(struct Line));
	new->tag = tag;
	LIST_INSERT_HEAD(&set->head, new, list_entry);
}

void modifyMemoryOnce(struct Cache* cache, unsigned int address, int s, int E,
		int b) {

	unsigned int tag;
	unsigned int setIndex;


	tag = getTagBit(address, s, b);
	setIndex = getSetIndex(address, s, b);

	struct Set* set = cache->sets + setIndex;

	//printf("start searching\n");
	if (addressInCache(set, s, E, tag)) {
		printf("hit ");
		hits += 1;

		updateCache(set, tag);
	} else {
		printf("miss ");
		miss += 1;

		if (set->size == E) {
			printf("evcition ");
			evict += 1;
			evictFromCache(set);
		}

		addToCache(set, tag);
		//printf("add completed\n");
	}
}

void initCache(struct Cache* cache, int s, int E) {
	int i;
	struct Set* set;

	cache->sets = malloc(s * sizeof(struct Set));

	for (i = 0; i < s; i++) {
		set = cache->sets + i;
		set->size = 0;

		LIST_INIT(&set->head);
	}
}

void freeCache(struct Cache* cache, int s, int E) {
	free(cache);
}

int main(int argc, char *argv[]) {
	/* First step:
	 Parse Arguments.
	 */
	int opt;
	int s, E, b;
	char *t;
	FILE *trace;
	struct Cache* cache;
	cache = malloc(sizeof(struct Cache));

	while ((opt = getopt(argc, argv, "hvb:s:E:t:")) != -1) {
		switch (opt) {
		case 'h':
			break;
		case 'v':
			break;
		case 's':
			s = atoi(optarg);
			break;
		case 'E':
			E = atoi(optarg);
			break;
		case 'b':
			b = atoi(optarg);
			break;
		case 't':
			t = optarg;
			break;
		default:
			exit(-1);
		}
	}

	initCache(cache, (1 << s), E);
	trace = fopen(t, "r");

	/* Second step:
	 Process.
	 */
	char line[80];
	char **array = malloc(3 * sizeof(char *));
	char *instruction;
	unsigned int address, block;
	while (fgets(line, 80, trace) != NULL) {
		if (line[0] != ' ') {
			continue;
		}

		array = tokenize(line, instruction, address, block);
		printf("%s %s,", array[0], array[1]);
		instruction = array[0];
		address = strtol(array[1], NULL, 16);
		block = atoi(array[2]);

		if (instruction[0] == 'L' || instruction[0] == 'S') {
			modifyMemoryOnce(cache, address, s, E, b);
		} else if (instruction[0] == 'M') {
			modifyMemoryOnce(cache, address, s, E, b);
			modifyMemoryOnce(cache, address, s, E, b);
		}
		printf("\n");
	}

	fclose(trace);
	//freeCache(cache, (1 << s), E);

	printSummary(hits, miss, evict);
}
