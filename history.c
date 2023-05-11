#include <stdlib.h>
#include "history.h"
#include "strtools.h"

struct history_input{	// history of input
	// all history input
	char *buffer[HISTORY_MAXNUM+1];
	int cur;	// current index of browsing
	int front;	// earliest input
	int rear;	// latest input 
};

st_hisP new_his(void) {
	
	int i;
	
	st_hisP res = (st_hisP)malloc(sizeof(st_his));
	for(i = 0; i < HISTORY_MAXNUM+1; i ++) {
		res->buffer[i] = NULL;
	}
	res->cur = 0;
	res->front = 0;
	res->rear = 0;
	
	return res;
}

const char *get_last(const st_hisP his) {
	
	if(his->front == his->rear)
		return NULL;
	
	if(his->cur != his->front){
		his->cur = (his->cur-1+HISTORY_MAXNUM)%HISTORY_MAXNUM;
	}
	
	return (his->buffer[his->cur]);
}

const char *get_next(const st_hisP his) {
	
	if(his->front == his->rear)
		return NULL;
	if(his->cur != his->rear){
		his->cur = (his->cur+1)%HISTORY_MAXNUM;
	}
	
	return (his->buffer[his->cur]);
}

int clear_his(st_hisP his) {
	
	int i;
	int count = 0;
	
	for(i = his->front; i != his->rear; i = (i+1)%HISTORY_MAXNUM) {
		
		if(his->buffer[i] != NULL) {
			
			free(his->buffer[i]);
			his->buffer[i] = NULL;
			count++;
		}
	}
	his->front = 0;
	his->rear = 0;
	his->cur = 0;
	return count;
}

int destroy_his(st_hisP his) {
	
	int i;
	int count = 0;
	
	for(i = his->front; i != his->rear; i = (i+1)%HISTORY_MAXNUM) {
		
		if(his->buffer[i] != NULL) {
			
			free(his->buffer[i]);
			count++;
		}
	}
	free(his);
	return count;
}

int store_input(st_hisP his, const char *input) {
	
	if(his->buffer[his->rear] != NULL) {
		free(his->buffer[his->rear]);
	}
	get_copy(input, &(his->buffer[his->rear]));
	his->rear = (his->rear+1)%HISTORY_MAXNUM;
	his->cur = his->rear;
	if(his->front == his->rear) {
		his->front = (his->front+1)%HISTORY_MAXNUM;
		return 0;
	}
	return 1;
}


