#ifndef __CMPFUNC_H__
#define __CMPFUNC_H__

/* 
 * module function: realization of some compare functions
 */ 

#include "mybool.h"

// return true if two strings resemble
_bool str_eq(const char *s1, const char *s2);

// return true if s1 < s2
_bool str_lt(const char *s1, const char *s2);

// return true if s1 > s2
_bool str_gt(const char *s1, const char *s2);

// return true if exp1=true and exp2=true
_bool AND(_bool exp1, _bool exp2);

// rerutn true if exo1=true or exp2=true
_bool OR(_bool exp1, _bool exp2);

#endif	// __CMPFUNC_H__
