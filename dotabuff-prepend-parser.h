#include <assert.h>

#define MAX_MATCHES 5
#define NUM_PARSE_MATCH_INSTR 5
/* assume nobody buys more than 256 items in a match */
#define MAX_MATCH_DATA_INDICES 256
#define MAX_HTML_READ 2048

const typedef struct sanitize_escape {
	char* code;
	int code_len;
	char* replace;
	int replace_len;
} SANITIZE_ESCAPE;

const typedef enum search_result {
	SEARCH_KEY_FOUND,
	SEARCH_END_FOUND,
	SEARCH_CONTINUE
} SEARCH_RESULT;

const typedef enum parse_state {
	PARSE_CONTINUE,
	PARSE_END,
	PARSE_FAILED
} PARSE_STATE;

typedef struct {
	char* data[MAX_MATCH_DATA_INDICES];
	int* length;
	int in_use;
} MATCH_DATA;

typedef struct parse_instr PARSE_INSTR;
typedef int (*PARSE_FUNC)(int, char*, PARSE_INSTR*);
struct parse_instr {
	char key[512]; // Initial key -- Edited in f() on the fly
	char end_if_key[512]; // Indicator of next section, set parameter 1 PARSE_END
	PARSE_FUNC f; // Will return if we are still to continue feeding buffer
	MATCH_DATA* match_data; // processed and amalgamated in main
	int state;
	const PARSE_INSTR* template;
};
	/* end_if_key is indicator of malformity if the key trigger is immediately deducible
	 * - otherwise it is an indicator of an end of a logical section of data. i.e. no 
	 * - more items to parse.  */

int set_ability_ignore_data(const char*);
int parse_hero_guide(FILE*);
void zero_state_parse_match_instr();
void free_parse_data();
