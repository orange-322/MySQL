#ifndef __TABLE_H__
#define __TABLE_H__

/* 
 * module function: realization of table and related functions
 */ 

#include "mybool.h"

// filename extension of table file
extern const char *table_file_suffix;
// dilimiter of filepath
extern const char *PATH_DIVIDE;
// initial capacity of table and database 
extern const int INIT_SIZE;
// supplement of capacity
extern const int SIZE_INCREMENT;

// single line of data 
struct record;
typedef struct record st_rec;
typedef struct record *st_recP;

// table
struct table;
typedef struct table st_table;
typedef struct table *st_tableP;


// create new line of data and return
st_recP new_rec(
	const char *vals[], // values
	int count			// number of values
);

// copy rec and return
st_recP copy_rec(
	const st_recP rec, 	// source record
	int select[], 		// selected columns
	int n_select		// number of selected columns
);

// get data of rec by column index
const char *get_val(
	const st_recP rec, 	// source record
	int index			// index of value
);

// set data of rec by column index
_bool set_val(
	st_recP rec, 			// source record
	const char* new_val, 	// target value
	int index				// index of value
);

// return true if two records resemble
_bool rec_eq(
	const st_recP r1, // two records to be compared
	const st_recP r2
);

// release space allocated to record, return 1 if succeed
int destroy_rec(st_recP rec);

// get number of column of table
int get_ncol(const st_tableP table);

// get number of line of table
int get_nrow(const st_tableP table);

// get index of primary key of table
int get_prim_key(const st_tableP table);

// set primary_key
int set_prim_key(st_tableP table, int prim);

// get titles of table
const char **get_titles(const st_tableP table);

// get name of table
const char *get_table_name(const st_tableP table);

// get a new table
st_tableP new_table(
	const char *name, 		// table name
	const char *titles[], 	// titles
	int prim_key, 			// primary key
	int title_count			// number of title
);

// renew width of each column, return number of columns renewed
int renew_witdh(
	st_tableP table, 	// target table
	const st_recP rec	// new-added record
);

// insert a record into table, return 1 if succeed
int add_rec(
	st_tableP table, 	// target table
	const st_recP rec, 	// new reacord
	_bool distinct		// =true if to add record distinctly
);

// remove column of index in table
int remove_column(st_tableP table, int index);

// add column of name to table
int add_column(st_tableP table, const char *name);

// remove record from table by index, return 1 if succeed
int remove_rec(st_tableP table, int index);

// test if a record satisfies constrains
// satisfy if :
// 		for all elem e in filter_index, s in filters and f in filter_funcs
// 		1. if and_or == AND
// 			f( get_val(rec, e), s ) == true
// 		2. if and_or == OR
// 			at least one f == true
_bool test_match(
	const st_recP rec, // target record
	// filter functions
	_bool (*filter_funcs[])(const char*, const char*), 
	// AND or OR
	_bool (*and_or)(_bool, _bool),
	// indeces of target values
	int *filter_index,
	// constrains
	const char *filters[],
	// number of constrains
	int n_filter
);

// query records in table and print all that satisfy constrains;
// return number of lines of result 
int query_rec(
	const st_tableP table,	// target table
	int *select_index, 		// indeces of selected values
	int n_select,			// number of selected, =0 means all-selected
	// filters
	_bool (*filter_funcs[])(const char*, const char*), 
	// AND or OR
	_bool (*and_or)(_bool, _bool),
	// indeces of target values
	int *filter_index,
	// constrains
	const char *filters[],
	// number of constrains
	int n_filter,
	// =true to return distinct records
	_bool distinct, 
	// to store query result
	st_tableP *res
);

// set records which satisfy constrains in table with new values
// reutrn number of records set successfully
int set_multi(
	st_tableP table,	// target table
	int *target_index,	// indeces of target values
	int n_target,		// number of target values
	const char *new_vals[],	// new values
	// filter functions
	_bool (*filter_funcs[])(const char*, const char*),
	// AND or OR
	_bool (*and_or)(_bool, _bool),
	// indeces of filters
	int *filter_index,
	// constrains
	const char *filters[],
	// number of constrains
	int n_filter
);

// remove records which satisfy constrains in table 
// return number of records removed
int remove_multi(
	st_tableP table,	// target table
	// filter functions
	_bool (*filter_funcs[])(const char*, const char*),
	// AND or OR
	_bool (*and_or)(_bool, _bool),
	// indeces of filter target
	int *filter_index,
	// constrains
	const char *filters[],
	// number of constrains
	int n_filter
);

// load table from file, return number of records read
int load_table(
	const char *folder_path, 	// folder the file is in
	const char* name, 			// file name
	st_tableP *table			// result
);

// save table as file, return number of records written
int save_table(
	const char *folder_path, 
	const st_tableP table
);

// print boundary of table
void print_line(
	int *max_width, // width of each column
	int title_count	// number of column
);

// as its name
void print_table(
	st_tableP table, 	// target table
	_bool delete_after_print	// =true destroy the table in the end
);

// remove all records in table
int clear_table(st_tableP table);

// release space allocated
int destroy_table(st_tableP table);

#endif	// __TABLE_H__
