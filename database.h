#ifndef __DATABASE_H__
#define __DATABASE_H__

/* 
 * module function: realization of database and related functions
 */ 
 
#include "table.h"

// database 
struct database;
typedef struct database st_dbase;
typedef struct database *st_dbaseP;

// create new database and return
st_dbaseP new_db(const char *name);

// add a table into database, return 1 if succeed
int add_table(st_dbaseP db, const st_tableP table);

// return size of database
int get_size(const st_dbaseP db); 

// get table in database by table name
st_tableP get_table(const st_dbaseP db, const char *name);

// get name of content (table) by index
const char *get_content_name(const st_dbaseP db, int index);

// get titles of content (table) by index
const char **get_content_titles(const st_dbaseP db, int index);

// get number of titles contenr (table) by index
int get_content_title_count(const st_dbaseP db, int index);

// show tables in database
void show_tables(const st_dbaseP db);

// remove table in database by table name, return 1 if succeed
int remove_table(st_dbaseP db, const char *name);

// remove all tables in database, return number of tables in database
int clear_tables(st_dbaseP db);

// release space allocated for database, return number of tables in databse
int destroy_database(st_dbaseP db);

// load database from file, return number of tables read
int load_database(st_dbaseP db);

// save databse as file, read number of tables written
int save_database(const st_dbaseP db);

#endif	// __DATABASE_H__
