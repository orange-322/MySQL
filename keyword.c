#include <stdlib.h>
#include <string.h>
#include "keyword.h"
#include "strtools.h"

struct key_word {
	char **words;
	int size;
	int capacity;
};

st_keywordP new_keyword(void) {
	
	st_keywordP res = (st_keywordP)malloc(sizeof(st_keyword));
	res->capacity = 50;
	res->size = 0;
	res->words = (char**)malloc(sizeof(char*)*50);
	return res;
}

int clear_keyword(st_keywordP kwd) {
	
	int i;
	int count = 0;
	
	for(i = 0; i < kwd->size; i ++) {
		if(kwd->words[i]){
			free(kwd->words[i]);
			count++;
		}
	}
	kwd->size = 0;
	return count;
}

int destroy_keyword(st_keywordP kwd) {
	
	int i;
	int count = 0;
	
	for(i = 0; i < kwd->size; i ++) {
		if(kwd->words[i]){
			free(kwd->words[i]);
			count++;
		}
	}
	free(kwd->words);
	free(kwd);
	return count;
}

int add_word(st_keywordP kwd, const char *word) {
	
	int high = kwd->size-1;
	int low = 0;
	int mid = 0;
	int bias = 0;
	int cmp_res;
	int i;
	
	while(low <= high) {
		
		mid = low+(high-low)/2;
		cmp_res = strcmp(word, kwd->words[mid]);
		if(cmp_res == 0) {
			return 0;
		} else if(cmp_res > 0) {
			low = mid+1;
			bias = 1;
		} else {
			high = mid-1;
			bias = 0;
		}
	}
	
	if(kwd->size >= kwd->capacity) {
		
		kwd->words = 
			(char**)realloc(kwd->words, sizeof(char*)*(kwd->capacity+20));
		kwd->capacity += 20;
	}
	for(i = kwd->size++; i-1 >= mid+bias; i --) {
		kwd->words[i] = kwd->words[i-1];
	}
	get_copy(word, &(kwd->words[mid+bias]));
	
	return 1;
}

int prefix_match_keyword(const st_keywordP kwd, const char *prefix, char **res) {
	
 	int high = kwd->size-1;
	int low  = 0;
	int mid;
	int cmp_res;
	int i, j;
	int pos = -1;
	int len_prefix;
	
	(*res) = NULL;
	if(!prefix) {
		return 0;
	}
	while(low <= high) {
		
		mid = low+(high-low)/2;
		cmp_res = prefix[0] - kwd->words[mid][0];
		if(cmp_res == 0) {
			break;
		} else if(cmp_res > 0) {
			low = mid+1;
		} else {
			high = mid-1;
		}
	}
	if(cmp_res != 0) {
		return 0;
	}
	len_prefix = strlen(prefix);
	for(i = mid, j = mid+1; 
		(i >= 0 && kwd->words[i][0] == prefix[0]) 
	 || (j < kwd->size && kwd->words[j][0] == prefix[0]); i--, j++) {
	 	
		if(i >= 0 && kwd->words[i][0] == prefix[0]) {
			
			if(match_len(kwd->words[i], prefix) == len_prefix) {
				if(pos != -1) {
					return 0;
				}
				pos = i;
			}
		}
		if(j < kwd->size && kwd->words[j][0] == prefix[0]) {
			if(match_len(kwd->words[j], prefix) == len_prefix) {
				if(pos != -1) {
					return 0;
				}
				pos = j;
			}
		}
	}
	if(pos == -1){
		return 0;
	}
	get_copy(kwd->words[pos]+len_prefix, res);
	return strlen(kwd->words[pos])-len_prefix;
}
