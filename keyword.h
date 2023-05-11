#ifndef __KEYWORD_H__
#define __KEYWORD_H__

/* 
 * module function: realization of keyword and related functions
 */ 

struct key_word;
typedef struct key_word st_keyword;
typedef struct key_word *st_keywordP;

// create new keyword and return
st_keywordP new_keyword(void);

// remove all words
int clear_keyword(st_keywordP kwd);

// relaease space allocated
int destroy_keyword(st_keywordP kwd);

// add word into keyword
int add_word(st_keywordP kwd, const char *word);

// match keyword with given prefix
int prefix_match_keyword(
	const st_keywordP kwd, 	// keyword
	const char *prefix, 	// target prefix
	char **res				// to store result
);

#endif // __KEYWORD_H__
