#include <string.h>
#include <stdlib.h>
#include "strtools.h"

int suffix_cmp(const char *file_name, const char *suffix) {
	
	int len_total = 0;
	int len_suf = 0;
	int i;
	
	if(!file_name || !suffix) {
		return -1;
	}
	len_total = strlen(file_name);
	len_suf = strlen(suffix);
	
	if(len_total < len_suf) {
		return -1;
	}
	return strcmp(file_name+(len_total-len_suf), suffix);
	 
}

int remove_suffix(const char *str, char buf[], int maxlen) {
	
	int pos=0;
	int i;
	
	for(i = 0; str[i] != '\0'; i ++) {
		if(str[i] == '.') {
			pos = i;
		}
	}
	
	if(pos == 0){
		return 0;
	}
	for(i = 0; i < pos && i < maxlen-1; i ++){
		buf[i] = str[i];
	}
	buf[i] = '\0';
	
	return i;
}

int cut_str(const char *from, const char *to, char **res) {
	
	int i = 0;
	
	if(!from || !to || from > to)
		return 0;
		
	*res = (char*)malloc(sizeof(char)*((to-from)+2));
	while(from+i <= to) {
		(*res)[i] = *(from+i);
		i++;
	}
	(*res)[i] = '\0';
	return 1;
}

int split_str_by(const char *str, char *tokens[], const char *divides) {
	
	const char *ahead, *behind, *divide;
	int count = 0;
	
	ahead = str;
	behind = str;
	
	while(*ahead) {
		
		while( *ahead) {
		
			for(divide = divides; *divide; divide++) {
				if(*ahead == *divide) {
					break;
				}
			}
			if(*divide == '\0') {
				break;
			}
			ahead++;
		}
		behind = ahead;
		while(*ahead) {
			
			for(divide = divides; *divide; divide++) {
				if(*ahead == *divide) {
					break;
				}
			}
			if(*divide != '\0') {
				break;
			}
			ahead++;
		}
		if(ahead-1 >= behind && *(ahead-1)) {
			cut_str(behind, ahead-1, &(tokens[count++]));
		}
	}
	return count;
}

int fgetline(FILE* fp, char *buf, int maxlen) {
	
	int len = 0;
	if(!fp || feof(fp) || !buf || maxlen <= 0)
		return 0;
	buf[len] = fgetc(fp);
	while(!feof(fp) && len < maxlen && buf[len] != '\n' && buf[len] != '\r') {
		
		len++;
		buf[len] = fgetc(fp);
	}
	buf[len] = '\0';
	return len;
}

int get_copy(const char *str, char **res) {
	
	if(!str || !res)	
		return 0;
	*res = (char*)malloc(sizeof(char)*(strlen(str)+1));
	strcpy(*res, str);
	return 1;
}

char upper_case(char ch) {
	
	if(ch >= 'a' && ch <= 'z') {
		return ch-32;
	}
	return ch;
}

int strcmp_omit_case(const char *str1, const char *str2) {
	
	if(!str1 && !str2)
		return 0;
	if(!str1 || !str2)
		return str1 ? 1 : -1;
	
	while(*str1 && *str2) {
		
		if(upper_case(*str1) != upper_case(*str2)) {
			
			return upper_case(*str1)-upper_case(*str2);
		}
		str1++;
		str2++;
	}
	
	return upper_case(*str1)-upper_case(*str2);
}

int get_index(const char *strs[], int n, const char *target) {
	
	int i;
	for(i = 0; i < n; i ++){
		
		if(strcmp(strs[i], target) == 0) {
			return i;
		}
	}
	return -1;
}

int match_len(const char *str1, const char *str2) {
	
	int i = 0;
	
	for(i = 0; *(str1+i) && *(str2+i) && *(str1+i) == *(str2+i); i ++)
		;
	return i;
}

int insert_str(char *str, int maxlen, const char *target, int pos) {
	
	int target_len;
	int str_len;
	int i;
	
	if(!str || !target || pos < 0 || pos >= maxlen) {
		return 0;
	}
	
	target_len = strlen(target);
	str_len = strlen(str);
	if(str_len + target_len >= maxlen || pos > str_len) {
		return 0;
	}
	str[str_len+target_len] = '\0';
	for(i = str_len+target_len-1; i-target_len >= pos; i --) {
		
		str[i] = str[i-target_len];
	}
	for(i = 0; i < target_len; i ++) {
		
		str[i+pos] = target[i];
	}
	return 1;
}

int is_leagal_id_ch(char ch) {
	
	return ( (ch >= 'A' && ch <= 'Z') ||
			 (ch >= '0' && ch <= '9') ||
			 (ch >= 'a' && ch <= 'z') ||
			 ch == '_' );
}

