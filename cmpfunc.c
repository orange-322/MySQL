#include "cmpfunc.h"
#include <string.h>

_bool str_eq(const char *s1, const char *s2) {
	
	return strcmp(s1, s2) == 0; 
}

_bool str_lt(const char *s1, const char *s2) {
	
	return strcmp(s1, s2) < 0;
}

_bool str_gt(const char *s1, const char *s2) {

	return strcmp(s1, s2) > 0;
}

_bool AND(_bool exp1, _bool exp2) {
	
	return exp1 && exp2;
}

_bool OR(_bool exp1, _bool exp2) {
	
	return exp1 || exp2;
}
