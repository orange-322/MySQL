#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tui.h"
#include "database.h"
#include "strtools.h"
#include "cmpfunc.h"
#include "history.h" 
#include "keyword.h"

#define MAXLEN 100

const char *header = "\nSQL> ";	// input header
st_hisP history_input;			// all history
st_dbaseP database;				// current database
st_keywordP keyword_table;		// table-related keywords
_bool running = false;			// running state
const int n_legal_command = 9;	// number of leagal command
// first word of commands (for matching)
const char *command_heads[9] = {
	"ALTER", "CREATE", "DELETE", "DROP", 
	"INSERT", "QUIT", "SELECT", "SHOW", "UPDATE"
};
// commands (for completion)
const char *legal_commands[9] = {
	"ALTER TABLE", "CREATE TABLE", "DELETE FROM", "DROP TABLE", 
	"INSERT INTO", "QUIT", "SELECT", "SHOW TABLES", "UPDATE"
};

// type of dirctions
typedef enum { none, left, right, up, down } DIRECT;

// as its name
typedef struct input_buffer {
	char *content;
	int size;
	int capacity;
}st_inBuf, *st_inBufP;

#ifdef WIN32
	#define _get_ch() _getch()		// input without echo
	const int chi_len = 2;			// length of one chinese character
	const int key_del = '\b';		// value of backspace
	const int key_direction_head = 224;	// first char of direction key
	DIRECT get_direct(void) {		// get direction type
		char ch;
		ch = _get_ch();
		switch(ch) {
		case 72: return up;
		case 80: return down;
		case 75: return left;
		case 77: return right;
		default: return none;
		}
	}
#endif
#ifdef linux
	#include <termio.h>
	const int chi_len = 3;
	int getch_linux() {
	
		struct termios tm, tm_old;
	    int fd = 0, ch;
	 
	    if (tcgetattr(fd, &tm) < 0) {
	        return -1;
	    }
	 
	    tm_old = tm;
	    cfmakeraw(&tm);
	    if (tcsetattr(fd, TCSANOW, &tm) < 0) {
	    	return -1;
	    }
	 
	    ch = getchar();
	    if (tcsetattr(fd, TCSANOW, &tm_old) < 0) {
	        return -1;
	    }
	    
	    return ch;
	}
	#define _get_ch() getch_linux()
	const int key_del = 0x7F;
	const int key_direction_head = 27;
	DIRECT get_direct(void) {
		char ch;
		ch = _get_ch();
		if(ch != 91) {
			return none;
		}
		ch = _get_ch();
		switch(ch) {
		case 65: return up;
		case 66: return down;
		case 68: return left;
		case 67: return right;
		default: return none;
		}
	}
#endif



_bool is_running() {
	
	return running;
}

int read_keyword(st_keywordP kwd, const st_dbaseP db) {
	
	int i, j;
	int count = 0;
	int ncol = 0;
	int db_size = get_size(db);
	const char **titles;
	
	for(i = 0; i < db_size; i ++) {
		
		add_word(kwd, get_content_name(db, i));
		count++;
		ncol = get_content_title_count(db, i);
		titles = get_content_titles(db, i);
		for(j = 0; j < ncol; j ++) {
			add_word(kwd, titles[j]);
			count++;
		}
	}
	return count;
}

void initialize() {
	
	history_input = new_his();
	database = new_db("test");
	load_database(database);
	keyword_table = new_keyword();
	add_word(keyword_table, "VALUES");
	add_word(keyword_table, "FROM");
	add_word(keyword_table, "WHERE");
	read_keyword(keyword_table, database);
	running = true;
}

int search_command(const char *command) {
	
	int high = n_legal_command-1;
	int low = 0;
	int mid;
	int cmp_res;
	
	if(!command) {
		return -1;
	}
	while(low <= high) {
		
		mid = low+(high-low)/2;
		cmp_res = strcmp_omit_case(command, command_heads[mid]);
		if(cmp_res == 0) {
			return mid;
		} else if(cmp_res > 0) {
			low = mid+1;
		} else {
			high = mid-1;
		}
	}
	
	return -1;
}

int prefix_match_command(const char *prefix, char **res) {
	
	int i, j;
	int pos = -1;
	int res_len = 0;
	int is_dup;
	int len_command, len_prefix;
	
	(*res) = NULL;
	if(!prefix) {
		return 0;
	}
	i = 0; 
	while(i < n_legal_command 
			&& upper_case(prefix[0]) != upper_case(legal_commands[i][0])) {
		i ++;
	}
	if(i == n_legal_command) {
		return 0;
	}
	len_prefix = strlen(prefix);
	if(len_prefix == 0) {
		return 0;
	}
	while(i < n_legal_command 
			&& upper_case(prefix[0]) == upper_case(legal_commands[i][0])) {
		
		len_command = strlen(legal_commands[i]);
		for(j = 1; j < len_command && j < len_prefix; j ++) {
			
			if(upper_case(prefix[j]) != upper_case(legal_commands[i][j])) {
				
				break;
			}
		}
		if(j == len_prefix) {
			
			if(pos == -1 && len_prefix < len_command) {
				pos = i;
				res_len = len_command;
			} else {
				return 0;
			}
		}
		i++;
	}
	if(pos == -1) {
		return 0;
	}
	get_copy(legal_commands[pos]+len_prefix, res);
	return res_len-len_prefix;
}

int switch_content(const char *target, st_inBufP buf) {
	
	int i;
	
	if(!buf || !target || !buf->content)
		return -1;
	
	buf->size = strlen(target);
	if(buf->size > buf->capacity) {
		
		buf->content = (char*)realloc(buf->content, sizeof(char)*(buf->size+1));
		buf->capacity = buf->size;
	}
	strcpy(buf->content, target);
	return 1;
}

char *input_command() {
	
	int i, j;
	int ch, ch2[2];
	int cur_pos;
	int command;
	st_inBuf buf;
	char *match_suffix;
	char *prefix;
	int match_len;
	const char *history;
	int bias = 1;
	DIRECT ret;
	
	buf.content = (char*)malloc(sizeof(char)*51);
	buf.content[0] = '\0';
	buf.size = 0;
	buf.capacity = 50;
	cur_pos = 0;
	
	printf("%s", header);
	ch = _get_ch();
	while(ch != '\n' && ch != '\r') {
		
		if(ch == key_del){
			
			if(cur_pos > 0 && buf.size > 0) {
				
				for(i = cur_pos; i < buf.size; i ++) {
					
					if(buf.content[i]&0x80) {
						printf("  ");
						i += (chi_len-1);
					} else {
						printf(" ");
					}
				}
				for(i = buf.size-1; i >= 0; i --) {
					if(buf.content[i]&0x80) {
						printf("\b\b  \b\b");
						i -= (chi_len-1);
					} else {
						printf("\b \b");
					}
				}
				bias = 1;
				if(buf.content[cur_pos-1]&0x80) {
					bias += (chi_len-1);
				} 
				for(i = cur_pos-bias; i < buf.size-bias; i ++) {
					buf.content[i] = buf.content[i+bias];
				}
				
				buf.size -= bias;
				buf.content[buf.size] = '\0';
				cur_pos -= bias;
				printf("%s", buf.content);
				for(i = buf.size-1; i >= cur_pos; i --) {
					if(buf.content[i]&0x80) {
						printf("\b\b");
						i -= (chi_len-1);
					} else {
						printf("\b");
					}
				}
			}
		} else if(ch == key_direction_head){
		
			ret = get_direct();
			switch(ret) {
			case up: 
				for(i = cur_pos; i < buf.size; i ++) {
					if(buf.content[i]&0x80) {
						printf("  ");
						i += (chi_len-1);
					} else {
						printf(" ");
					}
				}
				for(i = buf.size-1; i >= 0; i --) {
					if(buf.content[i]&0x80) {
						printf("\b\b  \b\b");
						i -= (chi_len-1);
					} else {
						printf("\b \b");
					}
				}
				switch_content(get_last(history_input), &buf); 
				printf("%s", buf.content);
				cur_pos = buf.size; break;
			case down: 
				for(i = cur_pos; i < buf.size; i ++) {
					if(buf.content[i]&0x80) {
						printf("  ");
						i += (chi_len-1);
					} else {
						printf(" ");
					}
				}
				for(i = buf.size-1; i >= 0; i --) {
					if(buf.content[i]&0x80) {
						printf("\b\b  \b\b");
						i -= (chi_len-1);
					} else {
						printf("\b \b");
					}
				}
				switch_content(get_next(history_input), &buf);
				printf("%s", buf.content); 
				cur_pos = buf.size; break;
			case left: 
				if(cur_pos > 0) {
					if(cur_pos > (chi_len-1) && buf.content[cur_pos-1]&0x80) {
						for(i = 0; i < 2; i ++) {
							printf("\b");
						}
						cur_pos -= chi_len;
					} else {
						printf("\b");
						cur_pos--;
					}
				}
				break;
			case right: 
				if(cur_pos < buf.size) {
					if(cur_pos+(chi_len-1) < buf.size && buf.content[cur_pos]&0x80) {
						for(i = 0; i < chi_len; i ++) {
							printf("%c", buf.content[cur_pos+i]);
						}
						cur_pos+=(chi_len);
					} else {
						printf("%c", buf.content[cur_pos]);	
						cur_pos++;
					}
				}
				break;
			default: break;
			}
		} else if(ch == '\t'/* && cur_pos == buf.size*/ ) {
			
			i = cur_pos-1;
			while(i >= 0 && is_leagal_id_ch(buf.content[i]) ) {
				i--;
			}
			if(i != cur_pos-1) {
				
				cut_str(buf.content+i+1, buf.content+cur_pos-1, &prefix);
				match_len = 
					prefix_match_keyword(keyword_table, prefix, &match_suffix);
				if(!match_len) {
					match_len = prefix_match_command(prefix, &match_suffix);
				}
				if(match_len != 0) {
					
					if(match_len + buf.size >= buf.capacity) {
						
						buf.content = 
							(char*)realloc(buf.content, sizeof(char)*(match_len + buf.size+21));
						buf.capacity = match_len + buf.size+21;
					}
					insert_str(buf.content, buf.capacity, match_suffix, cur_pos);
					printf("%s", match_suffix);
					buf.size += match_len;
					cur_pos += match_len;
					
					for(j = cur_pos; j < buf.size; j ++) {
						printf("%c", buf.content[j]);
					}
					for(i = buf.size-1; i >= cur_pos; i --) {
						if(buf.content[i]&0x80) {
							printf("\b\b");
							i -= (chi_len-1);
						} else {
							printf("\b");
						}
					}
					free(match_suffix);
				}
				free(prefix);
			}
		} else if(ch >= 32 && ch <= 126) {
			
			for(i = cur_pos; i < buf.size; i ++) {
				if(buf.content[i]&0x80) {
					printf("  ");
					i += (chi_len-1);
				} else {
					printf(" ");
				}
			}
			for(i = buf.size-1; i >= 0; i --) {
				if(buf.content[i]&0x80) {
					printf("\b\b  \b\b");
					i -= (chi_len-1);
				} else {
					printf("\b \b");
				}
			}
			if(buf.size >= buf.capacity) {
				buf.content = (char*)realloc(buf.content, sizeof(char)*(buf.capacity+21));
				buf.capacity += 20;
			}
			for(i = buf.size; i-1 >= cur_pos; i --) {
				buf.content[i] = buf.content[i-1];
			}
			
			buf.content[cur_pos++] = ch;
			buf.content[++(buf.size)] = '\0';
			printf("%s", buf.content);
			for(i = buf.size-1; i >= cur_pos; i --) {
				if(buf.content[i]&0x80) {
					printf("\b\b");
					i -= (chi_len-1);
				} else {
					printf("\b");
				}
			}
		} else if(ch&0x80) {
			
			for(i = 0; i < chi_len-1; i ++) {
				ch2[i] = _get_ch();
			}
			for(i = cur_pos; i < buf.size; i ++) {
				if(buf.content[i]&0x80) {
					printf("  ");
					i += (chi_len-1);
				} else {
					printf(" ");
				}
			}
			for(i = buf.size-1; i >= 0; i --) {
				if(buf.content[i]&0x80) {
					printf("\b\b  \b\b");
					i -= (chi_len-1);
				} else {
					printf("\b \b");
				}
			}
			if(buf.size+chi_len-1 >= buf.capacity) {
				buf.content = (char*)realloc(buf.content, sizeof(char)*(buf.capacity+21));
				buf.capacity += 20;
			}
			for(i = buf.size+chi_len-1; i-chi_len >= cur_pos; i --) {
				buf.content[i] = buf.content[i-chi_len];
			}
			buf.content[cur_pos] = ch;
			for(i = 0; i < chi_len-1; i ++) {
				buf.content[cur_pos+1+i] = ch2[i];
			}
			cur_pos += chi_len; buf.size += chi_len;
			buf.content[buf.size] = '\0';
			printf("%s", buf.content);
			for(i = buf.size-1; i >= cur_pos; i --) {
				if(buf.content[i]&0x80) {
					printf("\b\b");
					i -= (chi_len-1);
				} else {
					printf("\b");
				}
			}
		}
		ch = _get_ch();
	}
	printf("\n");
	store_input(history_input, buf.content);
	return buf.content;
}

int op_alter(const char *input) {
	
	char *tokens[MAXLEN];
	int n_token = 0;
	const char **titles;
	int title_count = 0;
	int prim = -1;
	st_tableP table;
	int ret;
	int temp;
	int i, j;
	
	n_token = split_str_by(input, tokens, " ;()\t");
	if(n_token <= 0 || strcmp_omit_case(tokens[0], "TABLE") != 0) {
		printf("syntax error: missing \"TABLE\" after \"ALTER\".\n");
		for(i = 0; i < n_token; i ++) {
			free(tokens[i]);
		}
		return 0;
	} 
	if(n_token <= 1) {
		printf("syntax error: missing table name after \"TABLE\".\n");
		for(i = 0; i < n_token; i ++) {
			free(tokens[i]);
		}
		return 0;
	}
	table = get_table(database, tokens[1]);
	if(table == NULL) {
		printf("error: no table of name \"%s\".\n", tokens[1]);
		for(i = 0; i < n_token; i ++) {
			free(tokens[i]);
		}
		return 0;
	}
	if(n_token <= 2) {
		printf("syntax error: missing \"ADD\" or \"DROP\" after table name.\n");
		for(i = 0; i < n_token; i ++) {
			free(tokens[i]);
		}
		return 0;
	}
	titles = get_titles(table);
	title_count = get_ncol(table);
	prim = get_prim_key(table);
	if(strcmp_omit_case(tokens[2], "ADD") == 0) {
		
		if(n_token <= 3) {
			printf("syntax error: column name must be given.\n");
			for(i = 0; i < n_token; i ++) {
				free(tokens[i]);
			}
			return 0;
		}
		j = 3;
		if(strcmp_omit_case(tokens[3], "PRIMARY_KEY") == 0) {
			j++;
		}
		if(n_token <= j) {
			printf("syntax error: column name must be given.\n");
			for(i = 0; i < n_token; i ++) {
				free(tokens[i]);
			}
			return 0;
		} else if(n_token > j+1) {
			printf("syntax error: too many attributes are given.\n");
			for(i = 0; i < n_token; i ++) {
				free(tokens[i]);
			}
			return 0;
		}
		temp = get_index(titles, title_count, tokens[j]);
		if(temp == -1) {
			if(j == 4){
				printf("error: table \"%s\" has no column \"%s\".\n",
						tokens[1], tokens[j]);
				for(i = 0; i < n_token; i ++) {
					free(tokens[i]);
				}
				return 0;
			} else {
				ret = add_column(table, tokens[j]);
				if(ret) {
					printf("column \"%s\" added to table \"%s\" successfully.\n", 
							tokens[j], tokens[1]); 
					add_word(keyword_table, tokens[j]);
				} else {
					printf("column \"%s\" failed to be added.\n",
							tokens[j]);
				}
			}
		} else {
			if(j != 4) {
				printf("error: no column \"%s\" in table \"%s\".\n", 
						tokens[j], tokens[1]);
				for(i = 0; i < n_token; i ++) {
					free(tokens[i]);
				}
				return 0;
			} else {
				if(prim != -1) {
					printf("error: table already has primary_key \"%s\".\n",
							titles[prim]);
					for(i = 0; i < n_token; i ++) {
						free(tokens[i]);
					}
					return 0;
				} else {
					ret = set_prim_key(table, temp);
					if(ret) {
						printf("primary key added successfully.\n");
					} else {
						printf("primary key failed to be added.\n");
					}
				}
			}
		}
		
	} else if(strcmp_omit_case(tokens[2], "DROP") == 0) {
		
		if(n_token <= 3) {
			printf("syntax error: column name must be given.\n");
			for(i = 0; i < n_token; i ++) {
				free(tokens[i]);
			}
			return 0;
		} else if(n_token > 4) {
			printf("syntax error: too many attributes are given.\n");
			for(i = 0; i < n_token; i ++) {
				free(tokens[i]);
			}
			return 0;
		}
		if(strcmp_omit_case(tokens[3], "PRIMARY_KEY") == 0) {
			
			if(prim == -1) {
				printf("error: table \"%s\" has no primary key.\n", 
						tokens[1]);
				for(i = 0; i < n_token; i ++) {
					free(tokens[i]);
				}
				return 0;
			} else {
				ret = set_prim_key(table, -1);
				if(ret) {
					printf("primary key dropped successfully.\n");
				} else {
					printf("primary key failed to be dropped.\n");
				}
			}
		} else {
			
			temp = get_index(titles, title_count, tokens[3]);
			if(temp == -1) {
				printf("error: no column \"%s\" in table \"%s\".\n", 
						tokens[3], tokens[1]);
				for(i = 0; i < n_token; i ++) {
					free(tokens[i]);
				}
				return 0;
			} else {
				ret = remove_column(table, temp);
				if(ret) {
					printf("column \"%s\" removed from table \"%s\" successfully.\n",
							tokens[3], tokens[1]);
				} else {
					printf("column \"%s\" failed to be removed from table \"%s\".\n",
							tokens[3], tokens[1]);
				}
			}
		}
		
	} else {
		printf("syntax error: unrecognized keyword \"%s\".\n", 
				tokens[2]);
		for(i = 0; i < n_token; i ++) {
			free(tokens[i]);
		}
		return 0;
	}
	
	for(i = 0; i < n_token; i ++) {
		free(tokens[i]);
	}
	
	return ret;
}

int op_create(const char *input) {
	
	char *tokens[MAXLEN];
	int n_token = 0;
	char *titles[MAXLEN];
	int title_count=0;
	int prim=-1;
	int ret = 0;
	int i;
	const char *p = input;
	st_tableP table;
	
	n_token = split_str_by(input, tokens, " \t,();");
	if(n_token <= 0 || strcmp_omit_case(tokens[0], "TABLE") != 0) {
		printf("syntax error: missing \"TABLE\" after CREATE.\n");
		for(i = 0; i < n_token; i ++) {
			free(tokens[i]);
		}
		return 0;
	}
	if(n_token <= 1) {
		printf("syntax error: name of table must be given.\n");
		for(i = 0; i < n_token; i ++) {
			free(tokens[i]);
		}
		return 0;
	}
	table = get_table(database, tokens[1]);
	if(table != NULL) {
		printf("error: table of name \"%s\" already exists.\n", tokens[1]);
		for(i = 0; i < n_token; i ++) {
			free(tokens[i]);
		}
		return 0;
	}
	if(n_token <= 2) {
		printf("error: a table must have at least one column.\n");
		for(i = 0; i < n_token; i ++) {
			free(tokens[i]);
		}
		return 0;
	}
	for(i = 2; i < n_token; i ++) {
		if(strcmp_omit_case(tokens[i], "PRIMARY_KEY") == 0) {
			if(i == 2) {
				printf("syntax error: column name can not be \"PRIMARY_KEY\".\n");
				for(i = 0; i < n_token; i ++) {
					free(tokens[i]);
				}
				return 0;
			} else if(prim != -1) {
				printf("error: there can only be one primary key.\n");
				for(i = 0; i < n_token; i ++) {
					free(tokens[i]);
				}
				return 0;
			} else {
				prim = i-3;
			}
		} else {
			titles[title_count++] = tokens[i];
		}
	}
	table = new_table(tokens[1], (const char**)titles, prim, title_count);
	ret = add_table(database, table);
	if(ret) {
		printf("1 table created successfully.\n");
		add_word(keyword_table, tokens[1]);
		for(i = 0; i < title_count; i ++) {
			add_word(keyword_table, titles[i]);
		}
	} else {
		printf("1 table failed to be created.\n");
	}
	for(i = 0; i < n_token; i ++) {
		free(tokens[i]);
	}
	return ret;
}

int op_delete(const char *input) {
	
	const char **titles;
	int title_count=0;
	char *tokens[MAXLEN];
	char *expression[5];
	int n_token = 0;
	int filter_index[MAXLEN];
	char *match_target[MAXLEN];
	_bool (*filter_funcs[MAXLEN])(const char*, const char*);
	_bool (*and_or)(_bool, _bool);
	st_tableP table;
	int n_filter = 0;
	int temp;
	int ret;
	int i, j;
	
	n_token = split_str_by(input, tokens, " ;\t");
	if(n_token <= 0 || strcmp_omit_case(tokens[0], "FROM") != 0) {
		printf("syntax error: missing \"FROM\" after \"DELETE\".\n");
		for(i = 0; i < n_token; i ++) {
			free(tokens[i]);
		}
		return 0;
	}
	if(n_token <= 1) {
		printf("syntax error: name of table must be given.\n");
		for(i = 0; i < n_token; i ++) {
			free(tokens[i]);
		}
		return 0;
	}
	table = get_table(database, tokens[1]);
	if(table == NULL) {
		printf("error: no table of name \"%s\".\n", tokens[1]);
		for(i = 0; i < n_token; i ++) {
			free(tokens[i]);
		}
		return 0;
	}
	titles = get_titles(table);
	title_count = get_ncol(table);
	if(n_token >= 3) {
		
		if( strcmp_omit_case(tokens[2], "WHERE") != 0 ) {
			printf("syntax error: missing \"WHERE\".\n");
			for(i = 0; i < n_token; i ++) {
				free(tokens[i]);
			}
			return 0;
		}
		if(n_token <= 3) {
			printf("syntax error: constrain expressions must be given.\n");
			for(i = 0; i < n_token; i ++) {
				free(tokens[i]);
			}
			return 0;
		}
		for(i = 0; i+3 < n_token; i ++) {
			
			if(i%2 == 0) {
				temp = split_str_by(tokens[i+3], expression, "=<>");
				if(temp != 2) {
					printf("syntax error: unreconized expression \"%s\".\n", tokens[i+3]);
					for(i = 0; i < n_token; i ++) {
						free(tokens[i]);
					}
					for(i = 0; i < temp; i ++) {
						free(expression[i]);
					}
					for(i = 0; i < n_filter; i ++) {
						free(match_target[i]);
					}
					return 0;
				}
				temp = get_index(titles, title_count, expression[0]);
				if(temp == -1) {
					printf("error: table \"%s\" has no column \"%s\".\n", 
							get_table_name(table), expression[0]);
					for(i = 0; i < n_token; i ++) {
						free(tokens[i]);
					}
					for(i = 0; i < n_filter; i ++) {
						free(match_target[i]);
					}
					free(expression[0]); free(expression[1]); 
					return 0;
				}
				filter_index[n_filter] = temp;
				filter_funcs[n_filter] = str_eq;
				get_copy(expression[1], &(match_target[n_filter]));
				n_filter++;
				free(expression[0]); free(expression[1]);
			} else {
				if(strcmp_omit_case(tokens[i+3], "AND") == 0) {
					and_or = AND;
				} else if(strcmp_omit_case(tokens[i+3], "OR") == 0) {
					and_or = OR;
				} else {
					printf("syntax error: unknow operator \"%s\".\n", tokens[i+3]);
					for(i = 0; i < n_token; i ++) {
						free(tokens[i]);
					}
					for(i = 0; i < n_filter; i ++) {
						free(match_target[i]);
					}
					return 0;
				}
			}
		}
	}
	
	ret = remove_multi(	table, filter_funcs, and_or, 
						filter_index, (const char**)match_target, n_filter);
	for(i = 0; i < n_token; i ++) {
		free(tokens[i]);
	}
	for(i = 0; i < n_filter; i ++) {
		free(match_target[i]);
	}
	printf("%d line(s) deleted from table \"%s\".\n", ret, get_table_name(table));
	return ret;
}

int op_drop(const char *input) {
	
	char *tokens[MAXLEN];
	int n_token = 0;
	int ret;
	int i;
	st_tableP table;
	
	n_token = split_str_by(input, tokens, " \t;");
	if(n_token <= 0 || strcmp_omit_case(tokens[0], "TABLE") != 0) {
		printf("syntax error: missing \"TABLE\" after DROP.\n");
		for(i = 0; i < n_token; i ++) {
			free(tokens[i]);
		}
		return 0;
	}
	if(n_token <= 1) {
		printf("syntax error: name of table must given.\n");
		for(i = 0; i < n_token; i ++) {
			free(tokens[i]);
		}
		return 0;
	}
	if(n_token > 2) {
		printf("syntax error: too many parameters.\n");
		return 0;
	}
	table = get_table(database, tokens[1]);
	if(table == NULL) {
		printf("error: table of name \"%s\" dose not exist.\n", tokens[1]);
		for(i = 0; i < n_token; i ++) {
			free(tokens[i]);
		}
		return 0;
	}
	ret = remove_table(database, (const char*)tokens[1]);
	for(i = 0; i < n_token; i ++) {
		free(tokens[i]);
	}
	if(ret == 1) {
		printf("1 table dropped succedsfully.\n");
		return 1;
	} else {
		printf("1 table failed to be dropped.\n");
		return 0;
	}
}

int op_insert(const char *input) {
	
	int i;
	int ret;
	char *tokens[MAXLEN];
	int n_token = 0;
	st_tableP table;
	st_recP rec;
	char *vals[MAXLEN];
	
	n_token = split_str_by(input, tokens, " ,();\t");
	if(n_token <= 0 || strcmp_omit_case(tokens[0], "INTO") != 0) {
		printf("syntax error: missing \"INTO\" after \"INSERT\".\n");
		for(i = 0; i < n_token; i ++) {
			free(tokens[i]);
		}
		return 0;
	}
	if(n_token <= 1) {
		printf("syntax error: name of table must be given.\n");
		for(i = 0; i < n_token; i ++) {
			free(tokens[i]);
		}
		return 0;
	}
	table = get_table(database, tokens[1]);
	if(table == NULL) {
		printf("error: no table of name \"%s\" exist.\n", tokens[1]);
		for(i = 0; i < n_token; i ++) {
			free(tokens[i]);
		}
		return 0;
	}
	if(n_token <= 2 || strcmp_omit_case(tokens[2], "VALUES") != 0) {
		printf("syntax error: missing \"VALUES\" after table name.\n");
		for(i = 0; i < n_token; i ++) {
			free(tokens[i]);
		}
		return 0;
	}
	if(n_token-3 != get_ncol(table)) {
		printf("error: given %d attribute(s) but table has %d column(s).\n",
				n_token-3, get_ncol(table));
		for(i = 0; i < n_token; i ++) {
			free(tokens[i]);
		}
		return 0;
	}
	for(i = 0; i+3 < n_token; i ++) {
		vals[i] = tokens[i+3];
	}
	rec = new_rec((const char**)vals, n_token-3);
	for(i = 0; i < n_token; i ++) {
		free(tokens[i]);
	}
	ret = add_rec(table, rec, false);
	if(ret == 0) {
		printf("error: new value conflict with an existing value of primary key.\n");
		return 0;
	} else {
		printf("1 line added successfully.\n");
		return 1;
	}
}

int op_quit(const char *input) {
	
	const char *p;
	
	p = input;
	while(*p) {
		
		if(*p != ' ' && *p != '\t' && *p != ';') {
			
			printf("syntax error: unrecognized element at \'%c\'\n", *p);
			return 0;
		}
		p++;
	}
	running = false;
	return 1;
}

int op_select(const char *input) {
	
	const char **titles;
	int title_count=0;
	char *tokens[MAXLEN];
	char *expression[5];
	st_tableP table, res;
	int select_index[MAXLEN];
	int n_select = 0;
	int filter_index[MAXLEN];
	char *match_target[MAXLEN];
	_bool (*filter_funcs[MAXLEN])(const char*, const char*);
	_bool (*and_or)(_bool, _bool);
	_bool distinct = false;
	int n_filter = 0;
	int n_token = 0;
	int temp;
	int i, j, k;
	int select_begin = 0;
	
	n_token = split_str_by(input, tokens, " ,;\t");
	j = 0;
	if(n_token <= j) {
		
		printf("syntax error: missing column name(s).\n");
		return 0;
	}
	if(strcmp_omit_case(tokens[j], "DISTINCT") == 0) {
		distinct = true;
		j++;
	}
	if(n_token <= j) {
		printf("syntax error: missing column name(s).\n");
		for(i = 0; i < n_token; i ++) {
			free(tokens[i]);
		}
		return 0;
	}
	select_begin = j;
	if(strcmp(tokens[j], "*") != 0) {
		
		n_select = 1;
		for(i = j+1; i < n_token; i ++) {
			if(strcmp_omit_case(tokens[i], "FROM") == 0) {
				break;
			} else if(strcmp(tokens[i], "*") == 0) {
				printf("error: column name can not be \"*\".\n");
				for(i = 0; i < n_token; i ++) {
					free(tokens[i]);
				}
				return 0;
			}
			n_select++;
		}
		if(i >= n_token) {
			printf("syntax error: missing \"FROM\" after columns.\n");
			for(i = 0; i < n_token; i ++) {
				free(tokens[i]);
			}
			return 0;
		}
		j = i;
	} else {
		j++;
	}
	if(n_token <= j || strcmp_omit_case(tokens[j], "FROM") != 0) {
		printf("syntax error: missing \"FROM\" after columns.\n");
		for(i = 0; i < n_token; i ++) {
			free(tokens[i]);
		}
		return 0;
	}
	if(n_token <= ++j) {
		printf("syntax error: name of table must be given.\n");
		for(i = 0; i < n_token; i ++) {
			free(tokens[i]);
		}
		return 0;
	}
	table = get_table(database, tokens[j]);
	if(table == NULL) {
		printf("error: no table of name \"%s\".\n", tokens[j]);
		for(i = 0; i < n_token; i ++) {
			free(tokens[i]);
		}
		return 0;
	}
	titles = get_titles(table);
	title_count = get_ncol(table);
	for(i = 0; i < n_select; i ++) {
		temp = get_index(titles, title_count, tokens[select_begin+i]);
		if(temp == -1) {
			printf("error: table \"%s\" has no column \"%s\".\n", 
					get_table_name(table), tokens[select_begin+i]);
			for(i = 0; i < n_token; i ++) {
				free(tokens[i]);
			}
			return 0;
		}
		select_index[i] = temp;
	}
	if(n_token > ++j) {
		if(strcmp_omit_case(tokens[j++], "WHERE") != 0) {
			printf("syntax error: missing \"WHERE\".\n");
			for(i = 0; i < n_token; i ++) {
				free(tokens[i]);
			}
			return 0;
		}
		for(i = 0; i+j < n_token; i++) {
			
			if(i%2 == 0) {
				temp = split_str_by(tokens[i+j], expression, "=<>");
				if(temp != 2) {
					printf("syntax error: uknow expression \"%s\".\n", tokens[i+j]);
					for(i = 0; i < n_token; i ++) {
						free(tokens[i]);
					}
					for(i = 0; i < temp; i ++) {
						free(expression[i]);
					}
					for(i = 0; i < n_filter; i ++) {
						free(match_target[i]);
					}
					return 0;
				}
				temp = get_index(titles, title_count, expression[0]);
				if(temp == -1) {
					printf("error: table \"%s\" has no column \"%s\".\n", 
							get_table_name(table), expression[0]);
					for(i = 0; i < n_token; i ++) {
						free(tokens[i]);
					}
					for(i = 0; i < n_filter; i ++) {
						free(match_target[i]);
					}
					free(expression[0]); free(expression[1]); 
					return 0;
				}
				filter_index[n_filter] = temp;
				filter_funcs[n_filter] = str_eq;
				get_copy(expression[1], &(match_target[n_filter]));
				n_filter++;
				free(expression[0]); free(expression[1]);
			} else {
				if(strcmp_omit_case(tokens[i+j], "AND") == 0) {
					and_or = AND;
				} else if(strcmp_omit_case(tokens[i+j], "OR") == 0) {
					and_or = OR;
				} else {
					printf("syntax error: unknow operator \"%s\".\n", tokens[i+j]);
					for(i = 0; i < n_token; i ++) {
						free(tokens[i]);
					}
					for(i = 0; i < n_filter; i ++) {
						free(match_target[i]);
					}
					return 0;
				}
			}
		}
	}
	temp = query_rec(	table, select_index, n_select, 
				filter_funcs, and_or, filter_index,
				(const char **)match_target, n_filter, 
				distinct, &res );
	for(i = 0; i < n_token; i ++) {
		free(tokens[i]);
	}
	for(i = 0; i < n_filter; i ++) {
		free(match_target[i]);
	}
	if(temp > 0){
		print_table(res, true);
	} else {
		destroy_table(res);
	}
	printf("%d line(s).\n", temp);
	return (temp>0);
}

int op_show(const char *input) {
	
	char buf[101];
	const char *p;
	st_tableP table;
	
	if( (sscanf(input, "%[a-zA-Z]", buf) != 1) 
	|| strcmp_omit_case(buf, "TABLES") != 0) {
		
		printf("syntax error: unrecognized element \"%s\"", buf);
		printf("Maybe you want to enter \"SHOW TABLES\"?\n");
		return -1;
	}
	p = input + 6;
	while(*p) {
		
		if(*p != ' ' && *p != '\t' && *p != ';') {
			
			printf("syntax error: unrecognized element at \'%c\'\n", *p);
			return 0;
		}
		p++;
	}
	show_tables(database);
	return 1;
}

int op_update(const char *input) {
	
	char *tokens[MAXLEN];
	int n_token = 0;
	const char **titles;
	int title_count=0;
	char *expression[5];
	st_tableP table;
	int set_index[MAXLEN];
	char *set_target[MAXLEN];
	int set_count = 0; 
	int filter_index[MAXLEN];
	char *match_target[MAXLEN];
	_bool (*filter_funcs[MAXLEN])(const char*, const char*);
	_bool (*and_or)(_bool, _bool);
	int n_filter = 0;
	int ret;
	int temp;
	int i, j;
	
	
	n_token = split_str_by(input, tokens, " ;,\t");
	if(n_token <= 0) {
		printf("syntax error: name of table must be given.\n");
		return 0;
	} 
	table = get_table(database, tokens[0]);
	if(table == NULL) {
		printf("error: table of name \"%s\" dose not exist.\n", tokens[0]);
		for(i = 0; i < n_token; i ++) {
			free(tokens[i]); 
		}
		return 0;
	}
	titles = get_titles(table);
	title_count = get_ncol(table);
	if(n_token <= 1 || strcmp_omit_case(tokens[1], "SET") != 0) {
		printf("syntax error: missing \"SET\" after table name.\n");
		for(i = 0; i < n_token; i ++) {
			free(tokens[i]); 
		}
		return 0;
	}
	if(n_token <= 2 || strcmp_omit_case(tokens[2], "WHERE") == 0) {
		printf("syntax error: at least one expression must be given after \"SET\".\n");
		for(i = 0; i < n_token; i ++) {
			free(tokens[i]); 
		}
		return 0;
	}
	for(j = 0; j+2 < n_token; j ++) {
		
		if(j > 0 && strcmp_omit_case(tokens[j+2], "WHERE") == 0) {
			break;
		}
		temp = split_str_by(tokens[j+2], expression, "=");
		if(temp != 2) {
			printf("syntax error: unrecogonized expression \"%s\".\n", tokens[j+2]);
			for(i = 0; i < n_token; i ++) {
				free(tokens[i]); 
			}
			for(i = 0; i < temp; i ++) {
				free(expression[i]);
			}
			for(i = 0; i < set_count; i ++) {
				free(set_target[i]);
			}
			return 0;
		}
		temp = get_index(titles, title_count, expression[0]);
		if(temp == -1) {
			printf("error: table \"%s\" has no column \"%s\".\n", tokens[0], expression[0]);
			for(i = 0; i < n_token; i ++) {
				free(tokens[i]); 
			}
			free(expression[0]); free(expression[1]);
			for(i = 0; i < set_count; i ++) {
				free(set_target[i]);
			}
			return 0;
		}
		set_index[set_count] = temp;
		get_copy(expression[1], &(set_target[set_count]));
		set_count++;
		free(expression[0]); free(expression[1]);
	}
	if(set_count + 2 < n_token) {
		
		if( strcmp_omit_case(tokens[set_count+2], "WHERE") != 0 ) {
			printf("syntax error: missing \"WHERE\".\n");
			for(i = 0; i < n_token; i ++) {
				free(tokens[i]); 
			}
			for(i = 0; i < set_count; i ++) {
				free(set_target[i]);
			}
			return 0;
		}
		if(n_token <= set_count+3) {
			printf("syntax error: at least one expression after \"WHERE\".\n");
			for(i = 0; i < n_token; i ++) {
				free(tokens[i]); 
			}
			for(i = 0; i < set_count; i ++) {
				free(set_target[i]);
			}
			return 0;
		}
		for(j = 0; j+set_count+3 < n_token; j ++) {
			
			if(j%2==0) {
				temp = split_str_by(tokens[j+set_count+3], expression, "=<>");
				if(temp != 2) {
					printf("syntax error: unrecogonized expression \"%s\".\n",
							tokens[j+set_count+3]);
					for(i = 0; i < n_token; i ++) {
						free(tokens[i]); 
					}
					for(i = 0; i < set_count; i ++) {
						free(set_target[i]);
					}
					for(i = 0; i < n_filter; i ++) {
						free(match_target[i]);
					}
					for(i = 0; i < temp; i ++) {
						free(expression[i]);
					}
					return 0;
				}
				temp = get_index(titles, title_count, expression[0]);
				if(temp == -1) {
					printf("error: table \"%s\" has no column \"%s\".\n", 
							tokens[0], expression[0]);
					for(i = 0; i < n_token; i ++) {
						free(tokens[i]); 
					}
					for(i = 0; i < set_count; i ++) {
						free(set_target[i]);
					}
					for(i = 0; i < n_filter; i ++) {
						free(match_target[i]);
					}
					free(expression[0]); free(expression[1]);
					return 0;
				}
				filter_index[n_filter] = temp;
				get_copy(expression[1], &(match_target[n_filter]));
				filter_funcs[n_filter] = str_eq;
				n_filter++;
				free(expression[0]); free(expression[1]);
				
			} else {
				if(strcmp_omit_case(tokens[j+set_count+3], "AND") == 0) {
					and_or = AND;
				} else if(strcmp_omit_case(tokens[j+set_count+3], "AND") == 0) {
					and_or = OR;
				} else {
					printf("syntax error: unreconized keyword \"%s\".\n",
							tokens[j+set_count+3]);
					for(i = 0; i < n_token; i ++) {
						free(tokens[i]); 
					}
					for(i = 0; i < set_count; i ++) {
						free(set_target[i]);
					}
					for(i = 0; i < n_filter; i ++) {
						free(match_target[i]);
					}
					return 0;
				}
			}
		}
	}
	
	ret = set_multi(table, set_index, set_count, (const char**)set_target, 
					filter_funcs, and_or, filter_index, 
					(const char**)match_target, n_filter);
	for(i = 0; i < n_token; i ++) {
		free(tokens[i]); 
	}
	for(i = 0; i < set_count; i ++) {
		free(set_target[i]);
	}
	for(i = 0; i < n_filter; i ++) {
		free(match_target[i]);
	}
	printf("%d lines in table \"%s\" updated.\n", ret, get_table_name(table));
} 

void analyze_command(const char *command) {
	
	int (*op_funcs[9])(const char *input) = 
	{ op_alter, op_create, op_delete, op_drop, op_insert,
	  op_quit, op_select, op_show, op_update };
	const char *p_ahead, *p_behind;
	int match_res = 0;
	char *part;
	
	st_tableP table;
	
	if(!command) {
		return;
	}
	p_ahead = command;
	while(*p_ahead && (*p_ahead == ' ' || *p_ahead == '\t')) {
		p_ahead++;
	}
	p_behind = p_ahead;
	while(*p_ahead && *p_ahead != ' ' && *p_ahead != '\t' && *p_ahead != ';'){
		p_ahead++;
	}
	p_ahead--;
	if(p_ahead >= p_behind) {
		
		cut_str(p_behind, p_ahead, &part);
		match_res = search_command(part);
		if(match_res == -1) {
			
			printf("syntax error: unrecognized command \"%s\"", part);
			free(part);
			return;
		}
		free(part);
		p_ahead ++;
		while(*p_ahead == ' ' || *p_ahead == '\t') {
			p_ahead++;
		}
		op_funcs[match_res](p_ahead);
		save_database(database);
	}
}

void terminate() {
	
	destroy_his(history_input);
	save_database(database);
	destroy_database(database);
	destroy_keyword(keyword_table);
}


