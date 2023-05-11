#ifndef __STRTOOLS_H__
#define __STRTOOLS_H__

/* 
 * module function: realization of some string-manipulating functions 
 */ 

#include <stdio.h>

#define TOKEN_MAXNUM 100

// compare suffixe of filename with given suffix, e.g: 1.txt's suffix is .txt
int suffix_cmp(const char *file_name, const char *suffix);

// remove suffix of file name, store in buf
int remove_suffix(const char *str, char buf[], int maxlen);

// cut string (from-to) and store in res
int cut_str(const char *from, const char *to, char **res);

// split string by divides and store in tokens, return number of tokens
int split_str_by(const char *str, char *tokens[], const char *divides);

// get one line of input(accept '\n' but would not store it in buf)
int fgetline(FILE* fp, char *buf, int maxcount);

// copy str to res
int get_copy(const char *str, char **res);

// return upper case of ch
char upper_case(char ch);

// compare two strings omiting case
int strcmp_omit_case(const char *str1, const char *str2);

// get index of target in strs
int get_index(const char *strs[], int n, const char *target);

// get match length of two strinas
int match_len(const char *str1, const char *str2);

// insert target into str at index pos, return 1 if succeed
int insert_str(char *str, int maxlen, const char *target, int pos);

// return 1 if ch is in [a-zA-Z0-9_]
int is_leagal_id_ch(char ch);

#endif // __STRTOOLS_H__
