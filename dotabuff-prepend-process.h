#ifndef _H_DB_PROCESS
#define _H_DB_PROCESS

typedef struct parse_instr PARSE_INSTR;

const typedef enum process_set_metadata {
	PR_HERO_READABLE,
	PR_ABILITY_PRE_TALENT_LEN,
	PR_M_STARTING_ITEM_LEN,
	PR_M_ITEM_BUILD_LEN,
	PR_M_ABILITY_BUILD_LEN
} PROCESS_ACTION;

const typedef enum process_result {
	PROCESS_SUCCESS,
	PROCESS_FAILED
} PROCESS_RESULT;

int process_set_data(PROCESS_ACTION, void*);
int process_analyse_and_set_prepend_data();
int create_readable_to_built_in_hash();
void process_link_static_indices(
		PARSE_INSTR* ability_list, PARSE_INSTR* role, PARSE_INSTR* lane,
		PARSE_INSTR* wards, PARSE_INSTR* items, PARSE_INSTR* abilities
	);
void hash_item_built_ins();
int form_hero_built_in_name(char*, char*);
void str_replace(char*, char*, char*);
void free_process_hash_lists();
#endif
