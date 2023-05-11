#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "database.h"
#include "strtools.h"

#ifdef WIN32
	#include <io.h>
	#include <direct.h>
	#include <conio.h>
	#define get_cur_dir(buf,size) _getcwd(buf,size)	// return current directory
	#define mk_dir(path) mkdir(path)				// make new folder
	#define folder_not_exists(path) _access(path,0)	// return 0 if file exist
	#define cd_dir(path) _chdir(path)				// change current dir
	typedef intptr_t p_traverse;	
	typedef struct _finddata_t p_folder;
#endif	// WIN32 
#ifdef linux
	#include <dirent.h>
	#include <sys/types.h> 
	#include <sys/stat.h>
	#include <unistd.h>
	#define get_cur_dir(buf,size) getcwd(buf,size)
	#define mk_dir(path) mkdir(path,0777)
	#define folder_not_exists(path) access(path,0)
	#define cd_dir(path) chdir(path)
	typedef DIR* p_traverse;
	typedef struct dirent* p_folder;
#endif	// linux

const char *database_file = "database";	

struct database{		// database structure 
	char *name;	
	st_tableP *content;	// all tables 
	int size;			// number of tables
	int capacity;		// capacity
};

st_dbaseP new_db(const char *name) {
	
	st_dbaseP db = (st_dbaseP)malloc(sizeof(st_dbase));
	
	db->name = (char*)malloc(sizeof(char)*(strlen(name)+1));
	strcpy(db->name, name);
	db->content = (st_tableP*)malloc(sizeof(st_tableP)*INIT_SIZE);
	db->capacity = INIT_SIZE;
	db->size = 0;
	return db;
}

int add_table(st_dbaseP db, const st_tableP table) {
	
	int high=db->size-1, low=0;
	int mid = 0;
	int cmp_ret;
	int bias = 0;
	int i;
	
	while(low <= high) {
		
		mid = low+(high-low)/2;
		cmp_ret = strcmp(get_table_name(table), get_table_name(db->content[mid]));
		if(cmp_ret == 0) {
			return 0;
		} else if(cmp_ret > 0) {
			low = mid+1;
			bias = 1;
		} else {
			high = mid-1;
			bias = 0;
		}
	}
	
	if(db->size >= db->capacity) {
		
		db->content = (st_tableP*)realloc(db->content, 
										sizeof(st_tableP)*(db->capacity+SIZE_INCREMENT));
		db->capacity += SIZE_INCREMENT;
	}
	for(i = db->size++; i-1 >= mid+bias; i --) {
		db->content[i] = db->content[i-1];
	}
	db->content[mid+bias] = table;
	return 1;
}

st_tableP get_table(const st_dbaseP db, const char *name) {
	
	int high = db->size-1, low = 0;
	int cmp_res;
	int mid;
	
	while(low <= high) {
		
		mid = low+(high-low)/2;
		cmp_res = strcmp(get_table_name(db->content[mid]), name);
		if(cmp_res == 0){
			return db->content[mid];
		} else if(cmp_res > 0) {
			high = mid-1;
		} else {
			low = mid+1;
		}
	}
	
	return NULL;
}

int get_size(const st_dbaseP db) {
	
	return db->size;
}

const char *get_content_name(const st_dbaseP db, int index) {
	
	if(index < 0 || index >= db->size) {
		return NULL;
	}
	return get_table_name(db->content[index]);
}

const char **get_content_titles(const st_dbaseP db, int index) {
	
	if(index < 0 || index >= db->size) {
		return NULL;
	}
	return get_titles(db->content[index]);
}

int get_content_title_count(const st_dbaseP db, int index) {
	
	if(index < 0 || index >= db->size) {
		return 0;
	}
	return get_ncol(db->content[index]);
}

void show_tables(const st_dbaseP db) {
	
	int i, j;
	int col;
	const char **titles;
	
	printf("%d table(s):\n", db->size);
	for(i = 0; i < db->size; i ++) {
		
		col = get_ncol(db->content[i]);
		titles = get_titles(db->content[i]);
		printf("%s(", get_table_name(db->content[i]));
		for(j = 0; j < col; j ++) {
			
			if(j > 0) {
				printf(",");
			}
			printf("%s", titles[j]);
			if(j == get_prim_key(db->content[i])) {
				printf(" PRIMARY_KEY");
			}
		}
		printf(")\n");
	}
}

int remove_table(st_dbaseP db, const char *name) {
	
	int high = db->size-1, low = 0;
	int cmp_res;
	int mid;
	int i;
	st_tableP table;
	
	while(low <= high) {
		
		mid = low + (high-low)/2;
		cmp_res = strcmp(get_table_name(db->content[mid]), name);
		if(cmp_res == 0) {
			break;
		} else if(cmp_res > 0) {
			high = mid-1;
		} else {
			low = mid+1;
		}
	}
	
	if(low > high) {
		return 0;
	}
	table = db->content[mid];
	for(i = mid; i+1 < db->size; i ++) {
		db->content[i] = db->content[i+1];
	}
	destroy_table(table);
	db->size--;
	return 1;
}

int clear_tables(st_dbaseP db) {
	
	int i;
	int res = db->size;
	
	for(i = 0; i < db->size; i ++) {
		destroy_table(db->content[i]);
	}
	db->size = 0;
	return res;
}

int destroy_database(st_dbaseP db) {
	
	int i;
	int res = db->size;
	
	for(i = 0; i < db->size; i ++) {
		destroy_table(db->content[i]);
	}
	free(db->name);
	free(db);
	return res;
}

int load_database(st_dbaseP db) {
	
	char *cur_dir = NULL;
	char *folder_path = NULL;
	char *match_dir = NULL;
	int success = 0;
	int ret = 0;
	int path_len = 0;
	st_tableP table;
	p_traverse dir;
	p_folder f;
	
	cur_dir = get_cur_dir(NULL, 0);
	if(!cur_dir) {
		return 0;
	}
	
	#ifdef WIN32
		path_len = 
			strlen(cur_dir) + 2*strlen(PATH_DIVIDE) 
			+ strlen(database_file);	
		match_dir = 
			(char*)malloc(sizeof(char)*(path_len + 4));
		sprintf(match_dir, "%s%s%s%s*.*", 
				cur_dir, PATH_DIVIDE, database_file, PATH_DIVIDE);
		dir = _findfirst(match_dir, &f);
		free(match_dir);
		if(dir == -1) {
			free(cur_dir);
			return 0;
		}
		do{
			if (f.attrib & _A_SUBDIR
            && strcmp(f.name, ".") != 0
            && strcmp(f.name, "..") != 0) {
            	path_len = 
					strlen(cur_dir) 
					+ 3*strlen(PATH_DIVIDE) 
					+ strlen(f.name)
					+ strlen(database_file); 
            	folder_path = (char*)malloc(sizeof(char)*(path_len+4));
            	sprintf(folder_path, "%s%s%s%s%s%s*.*",
						cur_dir, PATH_DIVIDE, database_file, 
						PATH_DIVIDE, f.name, PATH_DIVIDE);
			}
		} while(_findnext(dir, &f) == 0 && folder_path == NULL);
		_findclose(dir);
		if(folder_path == NULL) {
			free(cur_dir);
			return 0;
		} 
		dir = _findfirst(folder_path, &f);
		folder_path[path_len-strlen(PATH_DIVIDE)] = '\0';
		if(dir == -1) {
			free(cur_dir);
			free(folder_path);
			return 0;
		}
		do{
			if ( !(f.attrib & _A_SUBDIR)
				&& suffix_cmp(f.name, table_file_suffix) == 0 ) {
            	ret = load_table(folder_path, f.name, &table);
            	if(ret == -1) {
            		free(cur_dir);
					free(folder_path);
					return 0;
				}
				add_table(db, table);
				success++;
			}
		} while(_findnext(dir, &f) == 0);
		_findclose(dir);
		free(folder_path);
	#endif	// WIN32
	#ifdef linux
		path_len = 
			strlen(cur_dir) 
			+ strlen(PATH_DIVIDE) 
			+ strlen(database_file);	
		match_dir = 
			(char*)malloc(sizeof(char)*(path_len + 1));
		sprintf(match_dir, "%s%s%s", 
				cur_dir, PATH_DIVIDE, database_file);
		dir = opendir(match_dir);
		free(match_dir);
		if(dir == NULL) {
			free(cur_dir);
			return 0;
		}
		while((f = readdir(dir)) != NULL && folder_path == NULL) {
			if(f->d_type == DT_DIR && strcmp(f->d_name, ".") != 0
			&& strcmp(f->d_name, "..") != 0) {
				
				path_len = 
					strlen(cur_dir)
					+ 2*strlen(PATH_DIVIDE)
					+ strlen(database_file);
					+ strlen(f->d_name);
				folder_path = (char*)malloc(sizeof(char)*(path_len+1));
				sprintf(folder_path, "%s%s%s%s%s", 
						cur_dir, PATH_DIVIDE, database_file, PATH_DIVIDE, f->d_name);
			}
		}
		if(folder_path == NULL) {
			free(cur_dir);
			return 0;
		}
		dir = opendir(folder_path);
		if(dir == NULL) {
			free(folder_path);
			free(cur_dir);
			return 0;
		}
		while((f = readdir(dir)) != NULL) {
			if(f->d_type == DT_REG 
			&& suffix_cmp(f->d_name, table_file_suffix) == 0) {
				
				ret = load_table(folder_path, f->d_name, &table);
            	if(ret == -1) {
            		free(cur_dir);
					free(folder_path);
					return 0;
				}
				add_table(db, table);
				success++;
			}
		}
		free(folder_path);
	#endif	// linux
	
	free(cur_dir);
	return success;
}

int save_database(const st_dbaseP db) {
	
	char *cur_dir = NULL;
	char *folder_path = NULL;
	int path_len = 0;
	int i;
	
	cur_dir = get_cur_dir(NULL, 0);
	if(!cur_dir) {
		return 0;
	}
	if(folder_not_exists(database_file)) {
		mk_dir(database_file);
	}
	path_len = 
		strlen(cur_dir)+strlen(db->name) 
		+ 2*strlen(PATH_DIVIDE)
		+ strlen(database_file);
	folder_path = (char*)malloc(sizeof(char)*(path_len+1));
	sprintf(folder_path, "%s%s%s%s%s", 
			cur_dir, PATH_DIVIDE, database_file, PATH_DIVIDE, db->name);
	if( folder_not_exists(folder_path) ) {
		mk_dir(folder_path);
	}
	cd_dir(folder_path);
	for(i = 0; i < db->size; i ++) {
		save_table(folder_path, db->content[i]);
	}
	cd_dir(cur_dir);
	free(folder_path);
	free(cur_dir);
	return db->size;
}


