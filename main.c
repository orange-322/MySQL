#include <stdio.h>
#include <stdlib.h>
#include "tui.h"

int main() {
	
	char *command = NULL;
	
	initialize();
	while(is_running()) {
		
		command = input_command();
		analyze_command(command);
		free(command);
	}
	terminate();
	
	return 0;
}
