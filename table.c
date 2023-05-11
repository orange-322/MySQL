#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "table.h"
#include "strtools.h"

#ifdef WIN32
	const char *PATH_DIVIDE = "\\";
#endif
#ifdef linux
	const char *PATH_DIVIDE = "/";
#endif

struct record{	// record 
	char **vals;		// values 
	int val_count;		// number of values
};

struct table{			// table
	char *name;			// name of table
	char **titles;		// titles
	st_recP *records;	// lines of data 
	int *max_width;		// maximum width of each column 
	int prim_key;		// primary key
	int title_count;	// number of columns
	int rec_capacity;	// capacity
	int rec_count;		// number of lines 
};

const int INIT_SIZE = 50;
const int SIZE_INCREMENT = 10;
const char *table_file_suffix = ".txt";

st_recP new_rec(const char *vals[], int count) {
	
	int i;
	st_recP rec = (st_recP)malloc(sizeof(st_rec));
	
	rec->vals = (char**)malloc(sizeof(char*)*count);
	for(i = 0; i < count; i ++){
		rec->vals[i] = (char*)malloc(sizeof(char)*(strlen(vals[i])+1));
		if(strcmp(vals[i], "$") == 0) {
			rec->vals[i][0] = '\0';
		} else {
			strcpy(rec->vals[i], vals[i]);
		}
	}
	rec->val_count = count;
	return rec;
}

st_recP copy_rec(const st_recP rec, int *select, int n_select){
	
	int i;
	st_recP res = (st_recP)malloc(sizeof(st_rec));
	
	if(select == NULL || n_select <= 0) {
		res->val_count = rec->val_count;
		res->vals = (char**)malloc(sizeof(char*)*(res->val_count));
		for(i = 0; i < res->val_count; i ++) {
			res->vals[i] = (char*)malloc(sizeof(char)*(strlen(rec->vals[i])+1));
			strcpy(res->vals[i], rec->vals[i]);
		}
		return res;
	}
	res->val_count = n_select;
	res->vals = (char**)malloc(sizeof(char*)*(res->val_count));
	for(i = 0; i < n_select; i ++) {
		res->vals[i] = (char*)malloc(sizeof(char)*(strlen(rec->vals[select[i]])+1));
		strcpy(res->vals[i], rec->vals[select[i]]);
	}
	return res;
}

const char *get_val(const st_recP rec, int index) {
	
	if(index < 0 || index > rec->val_count)
		return NULL;
	return rec->vals[index];
}

_bool set_val(st_recP rec, const char* new_val, int index) {
	
	if(index < 0 || index > rec->val_count)
		return false;
	free(rec->vals[index]);
	rec->vals[index] = (char*)malloc(sizeof(char)*(strlen(new_val)+1));
	strcpy(rec->vals[index], new_val);
	return true;
}

_bool rec_eq(const st_recP r1, const st_recP r2) {
	
	int i;
	int cmp_res;
	
	if(r1->val_count != r2->val_count){
		return false;
	}
	for(i = 0; i < r1->val_count; i ++) {	
		if(strcmp(r1->vals[i], r2->vals[i]) != 0){
			return false;
		}
	}
	return true;
}

int destroy_rec(st_recP rec) {
	
	int i;
	int res = rec->val_count;
	
	for(i = 0; i < rec->val_count; i ++) {
		free(rec->vals[i]);
	}
	free(rec->vals);
	free(rec);
	return res;
}



st_tableP new_table(const char *name, const char *titles[], int prim_key, int title_count) {
	
	int i;
	int len;
	st_tableP table = (st_tableP)malloc(sizeof(st_table));
	
	table->name = (char*)malloc(sizeof(char)*(strlen(name)+1));
	strcpy(table->name, name);
	table->title_count = title_count;
	table->titles = (char**)malloc(sizeof(char*)*title_count);
	table->max_width = (int*)malloc(sizeof(int)*title_count);
	for(i = 0; i < title_count; i ++) {
		len = strlen(titles[i]);
		table->max_width[i] = len;
		table->titles[i] = (char*)malloc(sizeof(char)*(len+1));
		strcpy(table->titles[i], titles[i]);
	}
	table->prim_key = prim_key;
	table->records = (st_recP*)malloc(sizeof(st_recP)*(INIT_SIZE));
	table->rec_capacity = INIT_SIZE;
	table->rec_count = 0;
	return table;
}

const char *get_table_name(const st_tableP table) {
	
	return table->name;
}

int get_ncol(const st_tableP table) {
	
	return table->title_count;
}

int get_nrow(const st_tableP table) {
	
	return table->title_count;
}

int get_prim_key(const st_tableP table) {
	
	return table->prim_key;
}

int set_prim_key(st_tableP table, int prim) {
	
	if(prim == -1) {
		table->prim_key = -1;
		return 1;
	} else if(prim >= 0 && prim < table->title_count) {
		table->prim_key = prim;
		return 1;
	} else {
		return 0;
	}
}

const char **get_titles(const st_tableP table) {
	
	return (const char **)table->titles;
}

int strwidth(const char *str) {
	
	int res = 0;
	#ifdef WIN32
		const int chi_len = 2;
	#endif
	#ifdef linux
		const int chi_len = 3;
	#endif
	while(*str) {
		
		if(*(str+1) && (*str)&0x80) {
			res += 2;
			str += (chi_len-1);
		} else {
			res ++;
		}
		str++;
	}
	return res;
}

int renew_witdh(st_tableP table, const st_recP rec) {
	
	int count = 0;
	int len;
	int i;
	
	for(i = 0; i < table->title_count; i ++) {		
		len = strwidth(rec->vals[i]);
		if(len > table->max_width[i]){
			table->max_width[i] = len;
			count++;
		}
	}
	return count;
}

int add_rec(st_tableP table, const st_recP rec, _bool distinct) {
	
	int high = table->rec_count-1, low = 0;
	int cmp_res;
	int bias = 0;
	int mid = 0;
	int i;
	
	if(table->rec_count >= table->rec_capacity) {
		table->records 
			= (st_recP*)realloc(table->records, 
				sizeof(st_recP)*(table->rec_capacity+SIZE_INCREMENT));
		table->rec_capacity += SIZE_INCREMENT;
	}
	
	while(low <= high) {
		
		mid = low+(high-low)/2;
		if(table->prim_key != -1) {
			
			cmp_res = strcmp(	get_val(table->records[mid], table->prim_key), 
								get_val(rec, table->prim_key) );
			if(cmp_res == 0) {
				if(!distinct){
					return 0;
				} else {
					low = mid+1;
					bias = 1;
				}
			} else if(cmp_res < 0) {
				high = mid-1;
				bias = 0;
			} else {
				low = mid+1;
				bias = 1;
			}
		} else if(distinct && rec_eq(table->records[mid], rec)) {
			return 0;
		}
		else {
			low=mid+1;
		}
	}
	for(i = table->rec_count++; i-1 >= mid+bias; i--) {
		table->records[i] = table->records[i-1];
	}
	table->records[mid+bias] = rec;
	renew_witdh(table, rec);
	return 1;
}

int remove_column(st_tableP table, int index) {
	
	int i, j;
	char *title;
	
	if(index < 0 || index >= table->title_count || table->title_count == 1) {
		return 0;
	}
	if(table->prim_key != -1) {
		
		if(index == table->prim_key) {
			table->prim_key = -1;
		} else if(index < table->prim_key) {
			table->prim_key = -1;
		}
	}
	free(table->titles[index]);
	for(i = 0; i < table->rec_count; i ++) {
		free(table->records[i]->vals[index]);
		table->records[i]->val_count --;
	} 
	for(i = index; i+1 < table->title_count; i ++) {
		
		table->max_width[i] = table->max_width[i+1];
		table->titles[i] = table->titles[i-1];
		for(j = 0; j < table->rec_count; j ++) {
			table->records[j]->vals[i] = 
				table->records[j]->vals[i+1];
		}
	}
	table->title_count --;
	return 1;
}

int add_column(st_tableP table, const char *name) {
	
	int i;
	int name_len = 0;
	
	for(i = 0; i < table->title_count; i ++) {
		
		if(strcmp(table->titles[i], name) == 0) {
			return 0;
		}
	}
	
	name_len = strlen(name);
	table->titles = 
		(char**)realloc(table->titles, sizeof(char*)*(++table->title_count));
		
	table->titles[table->title_count-1] = 
		(char*)malloc(sizeof(char)*(name_len+1));
	strcpy(table->titles[table->title_count-1], name);
	
	table->max_width = 
		(int*)realloc(table->max_width, sizeof(int)*(table->title_count));
	table->max_width[table->title_count-1] = name_len;
	
	for(i = 0; i < table->rec_count; i ++) {
		
		table->records[i]->vals = 
			(char**)realloc(table->records[i]->vals, 
							sizeof(char*)*(table->title_count));
		table->records[i]->val_count ++;
		table->records[i]->vals[table->title_count-1] = 
			(char*)malloc(sizeof(char));
		table->records[i]->vals[table->title_count-1][0] = '\0';
	}
	return 1;
}

int remove_rec(st_tableP table, int index) {
	
	int i;
	st_recP res;
	
	if(index < 0 || index >= table->rec_count)
		return 0;
	res = table->records[index];
	for(i = index; i+1 < table->rec_count; i ++){
		table->records[i] = table->records[i+1];
	}
	table->rec_count--;
	destroy_rec(res);
	return 1;
}

_bool test_match(const st_recP rec,
				_bool (*filter_funcs[])(const char*, const char*), 
				_bool (*and_or)(_bool, _bool), int *filter_index, 
				const char *filters[], int n_filter) {
	
	_bool res = true;
	int j;
	
	if(filter_funcs == NULL || n_filter <= 0) {
		return true;
	}
	if(filter_funcs && n_filter > 0) {
		res = filter_funcs[0](get_val(rec, filter_index[0]), filters[0]);
	}
	for(j = 1; j < n_filter; j ++) {
		if(and_or != NULL){
			res = and_or(res, 
					filter_funcs[j]( get_val(rec, filter_index[j]), 
									 filters[j]) );
		} else {
			res = res && filter_funcs[j](get_val(rec, filter_index[j]), 
										filters[j]);
		}
	}
	return res;
}

int query_rec(	const st_tableP table, 
				int *select_index,  int n_select,
				_bool (*filter_funcs[])(const char*, const char*), 
				_bool (*and_or)(_bool, _bool), int *filter_index,
				const char **filters, int n_filter, 
				_bool distinct, st_tableP *res) {
	
	const char **titles;
	_bool match;
	int prim;
	st_recP rec;
	int count = 0;
	int i, j;
	
	titles = (const char**)malloc(sizeof(char*)*(n_select));
	if(select_index != NULL && n_select > 0){
		titles = (const char**)malloc(sizeof(char*)*(n_select));
		for(i = 0; i < n_select; i ++) {
			titles[i] = table->titles[select_index[i]];
		}
		*res = new_table("query_res", titles, -1, n_select);
		free(titles);
	} else {
		*res = new_table("query_res", (const char**)table->titles, -1, table->title_count);
	}
	for(i = 0; i < table->rec_count; i ++) {	
		match = 
			test_match(table->records[i], filter_funcs, and_or, 
						filter_index, filters, n_filter);
		if(match) {
			rec = copy_rec(table->records[i], select_index, n_select);
			count += add_rec(*res, rec, distinct);
		}
	}
	return count;
}

int set_multi(	st_tableP table, 
				int *target_index, int n_target, const char *new_vals[],
				_bool (*filter_funcs[])(const char*, const char*),
				_bool (*and_or)(_bool, _bool), int *filter_index, 
				const char *filters[], int n_filter ) {
	int count = 0;
	int i, j;
	_bool match;
	
	for(i = 0; i < table->rec_count; i ++) {
		match = 
			test_match(table->records[i], filter_funcs, and_or, 
						filter_index, filters, n_filter);
		if(match) {
			for(j = 0; j < n_target; j ++) {
				set_val(table->records[i], new_vals[j], target_index[j]);
			}
			count++;
		}
	}
	return count;
}

int remove_multi(st_tableP table,
				_bool (*filter_funcs[])(const char*, const char*),
				_bool (*and_or)(_bool, _bool), int *filter_index,
				const char *filters[], int n_filter) {
	int count = 0;
	int i;
	_bool match;
	
	for(i = table->rec_count-1; i >= 0; i --) {
		match = 
			test_match(table->records[i], filter_funcs, and_or, 
						filter_index, filters, n_filter);
		if(match) {
			
			count+=remove_rec(table, i);
		}
	}
	return count;
}

int load_table(const char *folder_path, const char* file_name, st_tableP *table) {
	
	FILE *fp;
	st_recP rec;
	char buf[1001]={0};
	char *full_path;
	char table_name[101];
	char *tokens[TOKEN_MAXNUM];
	int total = 0;
	int count = 0;
	int prim_key;
	int i;
	int full_len;
	
	full_len = strlen(folder_path) + strlen(file_name) + strlen(PATH_DIVIDE);
	full_path = (char*)malloc(sizeof(char)*(full_len+1));
	sprintf(full_path, "%s%s%s", folder_path, PATH_DIVIDE, file_name);
	fp = fopen(full_path, "r");
	free(full_path);
	if(fp == NULL) {	
		return -1;
	}
	
	if(fscanf(fp, "%d", &prim_key) != 1) {
		fclose(fp);
		return -1;
	}
	fgetc(fp);
	fgetline(fp, buf, 1001);
	count = split_str_by(buf, tokens, "\r\n \t");
	if(count <= 0){
		fclose(fp);
		return -1;
	}
	remove_suffix(file_name, table_name, 100);
	(*table) = new_table(table_name, (const char**)tokens, prim_key, count);
	for(i = 0; i < count; i ++){
		free(tokens[i]);
	}
	
	while(fgetline(fp, buf, 1001) != 0) {
		
		total++;
		count = split_str_by(buf, tokens, " \t\r\n");
		if(count == (*table)->title_count) {
			
			rec = new_rec((const char**)tokens, count);
			add_rec(*table, rec, false);
		}
		for(i = 0; i < count; i ++){
			free(tokens[i]);
		}
	}
	
	fclose(fp);
	return total;
}

int save_table(const char *folder_path, const st_tableP table) {
	
	FILE *fp;
	char *full_path;
	int full_len=0;
	int total;
	int len; 
	int i, j;
	
	total = table->title_count;
	full_len += (strlen(folder_path)+strlen(table->name));
	full_len +=	(strlen(PATH_DIVIDE)+strlen(table_file_suffix));
	full_path = (char*)malloc(sizeof(char)*(full_len+1));
	sprintf(full_path, "%s%s%s%s", 
			folder_path, PATH_DIVIDE, table->name, table_file_suffix);
	fp = fopen(full_path, "w");
	free(full_path);
	if(fp == NULL) {
		return 0;
	}
	
	fprintf(fp, "%d\n", table->prim_key);
	for(i = 0; i < table->title_count; i ++) {
		
		if(i > 0)	fprintf(fp, "\t");
		fprintf(fp, "%s", table->titles[i]);
	}
	fprintf(fp, "\n");
	for(i = 0; i < table->rec_count; i ++) {
		
		for(j = 0; j < table->title_count; j ++) {
			
			if(j > 0)	fprintf(fp, "\t");
			len = strlen(table->records[i]->vals[j]);
			if(len == 0) {
				fprintf(fp, "$");
			} else {
				fprintf(fp, "%s", table->records[i]->vals[j]);
			}
		}
		fprintf(fp, "\n");
	}
	
	fclose(fp);
}

void print_line(int *max_width, int title_count) {
	
	int i, j;
	
	for(i = 0; i < title_count; i ++) {
		printf("+");
		for(j = 0; j < max_width[i]+2; j ++) {
			printf("-");
		}
	}
	printf("+\n");
}

void print_table(st_tableP table, _bool delete_after_print) {
	
	int i, j;
	
	print_line(table->max_width, table->title_count);
	for(i = 0; i < table->title_count; i ++) {
		printf("| %-*s ", table->max_width[i], table->titles[i]);
	}
	printf("|\n");
	for(i = 0; i < table->rec_count; i ++) {				
		print_line(table->max_width, table->title_count);
		for(j = 0; j < table->title_count; j ++) {
			printf("| %-*s ", table->max_width[j], table->records[i]->vals[j]);
		}
		printf("|\n");
	}
	print_line(table->max_width, table->title_count);
	if(delete_after_print){
		destroy_table(table);
	}
}

int clear_table(st_tableP table) {
	
	int i;
	int res = table->rec_count;
	
	for(i = 0; i < table->rec_count; i ++) {
		destroy_rec(table->records[i]);
	}
	table->rec_count = 0;
	return res;
}

int destroy_table(st_tableP table) {
	
	int i;
	int res = table->rec_count;
	
	for(i = 0; i < table->title_count; i ++) {
		free(table->titles[i]);
	}
	for(i = 0; i < table->rec_count; i ++) {
		destroy_rec(table->records[i]);
	}
	free(table->name);
	free(table->records);
	free(table->titles);
	return res;
}

