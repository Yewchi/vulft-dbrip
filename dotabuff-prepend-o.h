#ifndef _H_DB_OUT
#define _H_DB_OUT
const typedef enum out_result {
	OUT_SUCCESS,
	OUT_FAILED
} OUT_RESULT;

const typedef enum delim_state {
	DELIM_NOT_FOUND,
	DELIM_FOUND
} DELIM_STATE;

/* After delim copy of code file structured:
 * [remove/edit data]
 * [prepend end delimiter] // must be on a new line by itself
 * [keep data] */
/* prev_saved will be overwritten for each subsequent call to 
 * save_prev_after_delim. Will be resized as needed. */
/* Data is only saved if a delimiter is found. The original may be lost. */
/* If no data is found or !file_exist, returns a copy of the delimiter. */
char* save_prev_after_delim(char* filename, char* end_prepend_delim);
int out_initialize();
void free_out();

/* Interface */
int out_syntax_initialize();
int prepend_hero_file();
void free_out_syntax(); 
#endif
