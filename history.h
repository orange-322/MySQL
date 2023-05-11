#ifndef __HISTORY_H__
#define __HISTORY_H__

/* 
 * module function: realization of history and related functions
 */ 

#define HISTORY_MAXNUM 100

struct history_input;
typedef struct history_input st_his;
typedef struct history_input *st_hisP;

// create new history_input and return
st_hisP new_his(void);

// get last input;
const char *get_last(const st_hisP his);

// get next input
const char *get_next(const st_hisP his);

// clear all history
int clear_his(st_hisP his);

// release space allocated
int destroy_his(st_hisP his);

// store input into history_input
int store_input(st_hisP his, const char *input);

#endif // __HISTORY_H__
