#ifndef __TUI_H__
#define __TUI_H__
#include "mybool.h"

/* 
 * module function: realization of user interface
 */ 

// return true if go on running
_bool is_running();

void initialize();
char *input_command();
void analyze_command(const char *command);
void terminate();

#endif // __TUI_H__
