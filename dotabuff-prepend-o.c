#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dotabuff-prepend-o.h"

#define MAX_BUFFER_LINE 2048
#define MAX_BUFFER_READ 2046

int prev_file_data_max_size = 16384;
char* buffer;
char* prev_to_new_ptr;
char file_line_buffer[MAX_BUFFER_LINE];
int delim_found;

int out_initialize() {
	if ((buffer = malloc(sizeof(char)*prev_file_data_max_size)) == NULL) {
		printf("Err - Alloc previous file buffer failed.\n");
		return OUT_FAILED;
	}

	return out_syntax_initialize();
}

int is_whitespace(char c) {
	switch(c) {
		case ' ':
		case '\n':
			return 1;
		default:
			return 0;
	}		
}

char* save_prev_after_delim(char* filename, char* prepend_delim) {
	static int prev_file_data_size;
	static int prepend_delim_len;
	static char* prev_delim;
	static FILE* prev_file;

	if ((prev_file = fopen(filename, "r")) == NULL) {
		printf("bad read of prev file\n");
		strcpy(buffer, prepend_delim);
		return buffer;
	}
	if (prev_delim != prepend_delim) {
		prepend_delim_len = strlen(prepend_delim);
		prev_delim = prepend_delim;	
	}

	int delim_found = DELIM_NOT_FOUND;
	int curr_line_size = -1;
	int check_increased_buffer_okay = 0;
	// max size check is 1-step forward to allow integer size arithmetic off 1 strlen
	while(1) {
		if (fgets(file_line_buffer, MAX_BUFFER_READ, prev_file) != NULL) {
			if (delim_found == DELIM_FOUND) {
				curr_line_size = strlen(file_line_buffer);
				if (prev_file_data_size+curr_line_size+4+MAX_BUFFER_READ >= prev_file_data_max_size) {
					prev_file_data_max_size *= 2;
					printf("REALLOC save_prev_after_delim() retrying.. \nFILELINE:\n%s",
							file_line_buffer
						);
					if ((buffer = realloc(buffer, sizeof(char)*prev_file_data_max_size)) == NULL)
					{
						printf("Err - Allocation error reading prev file\n");
						fclose(prev_file);
						return NULL;
					}
					check_increased_buffer_okay = 2;
					printf("prev_file buffer size is now: %d bytes.. should see 2x \"okay\": ...\n",
							prev_file_data_max_size
						);
				}
				strcpy(&(buffer[prev_file_data_size]),
						file_line_buffer);
				if (check_increased_buffer_okay > 0) {
					printf("strcpy to buffer okay%d...\n", check_increased_buffer_okay);
					check_increased_buffer_okay--;
				}
				prev_file_data_size = prev_file_data_size + curr_line_size;
			} else if (delim_found == DELIM_NOT_FOUND) {
				if (memcmp(file_line_buffer, prepend_delim, prepend_delim_len)
						== 0) {
					delim_found = DELIM_FOUND;
					strcpy(buffer, prepend_delim);
					buffer[prepend_delim_len] = '\n';
					buffer[prepend_delim_len+1] = '\0';
					prev_file_data_size = prepend_delim_len+1;
				}
			}
		} else {
			break;
		}
	}
	if (delim_found == DELIM_NOT_FOUND) {
		printf("hard-copying delim for DELIM_NOT_FOUND\n");
		strcpy(buffer, prepend_delim);
		buffer[prepend_delim_len] = '\n';
		buffer[prepend_delim_len] = '\0';
	}
	fclose(prev_file);
	return buffer;
}

void free_out() {
	free(buffer);
	free_out_syntax();
}
