#include "dotabuff-prepend.h"

#define TOWN_PORTAL_SCROLL_LEN 18

#define ABILITY_IGNORE_DAT_DELIM '@'

#define MAX_HERO_OR_ABILITY_NAME_LEN 128

#define NO_IGNORES_FOR_HERO -1

extern HERO_DATA CurrHeroData;

#define NUM_SANITIZERS 4
const static SANITIZE_ESCAPE sanitizers[] = {
	{"&#47;", 4, "/", 1}, {"&#39;", 4, "'", 1}, {"\r", 1, "", 0}, {"';", 2, "'", 1}
};

int match_iteration = 0;

static int f_hero_href(int, char*, PARSE_INSTR*);
static int f_readable_name(int, char*, PARSE_INSTR*);
static int f_ability_list(int, char*, PARSE_INSTR*);
static int f_lane_role(int, char*, PARSE_INSTR*);
static int f_ward_desire(int, char*, PARSE_INSTR*);
static int f_item_build(int, char*, PARSE_INSTR*);
static int f_ability_build(int, char*, PARSE_INSTR*);

static void parse_instr_template_cpy(PARSE_INSTR*);
static void replace_str_in_instr_keys(char*, char*);

typedef struct {
	char hero_name[MAX_HERO_OR_ABILITY_NAME_LEN];
	char** abilities;
	int ability_count;
} IGNORE_DATA;

typedef struct {
	IGNORE_DATA* data;
	int hero_count; // number of heroes with ignoreable abilities
} IGNORE_ABILITY_ARR;

IGNORE_ABILITY_ARR ignore_ability_arr;

int ignore_hero_index = NO_IGNORES_FOR_HERO;

static int any_data_search_success(char* buffer, char* key, char* end_if_key, int *seek_diff) {
	int len = strlen(key);
	int end_len = strlen(end_if_key);
	int stop_search_for_full_compare = MAX_HTML_READ - len;
	int stop_search_for_full_compare_end = MAX_HTML_READ - end_len;
	int i = 0;
	while(i < MAX_HTML_READ-1) {
		if (buffer[i] == key[0]) {
#ifdef VERBOSE
	printf("\\(%d: '%c')\\", i, buffer[i], key[0]);
#endif
			if (i > stop_search_for_full_compare) {
#ifdef VERBOSE
	printf("__any_data_search_success signals give more for key at i:%d.\n", i);
#endif
				*seek_diff = i; // buffer forward from here, need to cmp past end
				return SEARCH_CONTINUE;
			} else if (memcmp(&buffer[i], key, len) == 0) {
#ifdef VERBOSE
	printf("__any_data_search_success \"%s\" found at i:%d.\n", key, i);
#endif
				*seek_diff = i+len;
				return SEARCH_KEY_FOUND;
			}
		}
		if (buffer[i] == end_if_key[0]) {
#ifdef VERBOSE
	printf("\\(%d: '%c')\\", i, buffer[i], end_if_key[0]);
#endif
			if (i > stop_search_for_full_compare_end) {
#ifdef VERBOSE
	printf("__any_data_search_success signals give more for end_if_key at i:%d.\n", i);
#endif
				*seek_diff = i; // buffer forward from here, need to cmp past end
				return SEARCH_CONTINUE;
			} else if (memcmp(&buffer[i], end_if_key, end_len) == 0) {
#ifdef VERBOSE
	printf("\n__any_data_search_success \"%s\" found at i:%d.\n", end_if_key, i);
#endif
				*seek_diff = i+end_len;
				return SEARCH_END_FOUND;
			}
		} else if (buffer[i] == EOF) {
#ifdef VERBOSE
	printf("\n__any_data_search_success([buffer], %s, %s, [int*]) EOF at i:%d.\n", key, end_if_key, i);
#endif
			break; // parse_hero_guide will catch and return failed.
		}
#ifdef VERBOSE
	else {printf("%c", buffer[i]);}
#endif
		i++;
	}
#ifdef VERBOSE
	printf("\n");
#endif
	*seek_diff = i;
	return SEARCH_CONTINUE;
}
static int sanitize_html_special_chars(char* str) {
	int end_check = strlen(str);
	// blah
	str[end_check] = '\0';
	int i=0;
	static char trailingSwapBuffer[MAX_HTML_READ];
	SANITIZE_ESCAPE* san;
	while(i<end_check) {
		for (int s_index=0; s_index<NUM_SANITIZERS; s_index++) {
			san = &(sanitizers[s_index]);
			if (memcmp(&(str[i]), san->code, san->code_len) == 0) {
				strcpy(trailingSwapBuffer, &(str[i+san->code_len]));
				strncpy(&(str[i]), san->replace, san->replace_len);
				strcpy(&(str[i+san->replace_len]), trailingSwapBuffer);
				end_check += san->replace_len - san->code_len;
				str[end_check + 4] = '\0'; // « should never fail¿
			}
		}
		i++;
	}
}
static void set_match_data_str(char* src, int len, MATCH_DATA* md) {
	int i = md->in_use;
	if (md->data[i] == NULL || len > md->length[i]) {
		if (md->length[i] != 0) {
#ifdef VERBOSE
			printf("Increasing size of index's match data.\n");
#endif
			md->data[i][md->length[i]] = '\0';
			free(md->data[i]);
		}
		md->length[i] = len;
		md->data[i] = malloc(sizeof(char)*(len+1)*2);
	}
	strncpy(md->data[i], src, len);
	md->data[i][len] = '\0';
	md->in_use++;
}
static int set_ability_list_ability(char* buffer, PARSE_INSTR* instr) {
#ifdef VERBOSE
	char str[20]; memcpy(str, buffer, 19); str[19] = '\0';
	printf("__set_ability_list_ability(%s, [this_instr])\n", str);
#endif
	int ability_str_readable_index;
	int search_result = any_data_search_success(
			buffer, "title=\"", "jpg\" /", &ability_str_readable_index
		);
	if (search_result == SEARCH_KEY_FOUND) {
		MATCH_DATA* md = &(instr->match_data[match_iteration]);
		char* start_key = &buffer[ability_str_readable_index];
		char* end = strchr(start_key, '\"');
		if (end == NULL) { printf("ERR - no end chr.\n"); return PARSE_FAILED; }
		int ability_readable_len = end - start_key;
		set_match_data_str(
				&buffer[ability_str_readable_index], ability_readable_len, md
			);
		sanitize_html_special_chars(md->data[md->in_use-1]);

#ifdef DEBUG
		printf("ABILITY SET %d \"%s\"\n", md->in_use, md->data[md->in_use-1]);
#endif

#ifdef REMOVE_ADDITIONAL_ABILITIES
		// Remove the assumed additional unskillable abilities. eg. Flame cloak
		if (ignore_hero_index != NO_IGNORES_FOR_HERO) {
			IGNORE_DATA* ignoreData
				= &ignore_ability_arr.data[ignore_hero_index];
			for (int i=0; i<ignoreData->ability_count; i++) {
				if (strcmp(md->data[md->in_use-1], ignoreData->abilities[i]) == 0) {
#ifdef DEBUG
					printf("ABILITY UNSET (REMOVE_ADDITIONAL_ABILITIES) %d \"%s\".\n",
							md->in_use, md->data[md->in_use-1]);
#endif
					--md->in_use;
					break;
				}
			}
		}
#endif
		return PARSE_CONTINUE;
	} else { 
		// Because the buffer is shifted to this (probably empty) image-skill before
		// / talent, just see if "Talent Tree" is in the buffer to determine data ok.
		// / this is probably the most redundant check in the code.
		if (strstr(buffer, "Talent Tree") != NULL) {
			return PARSE_CONTINUE;
		} else {
			printf("ERR - malformed ability list.\n"); 
			return PARSE_FAILED;
		}
	}
}
static int set_item_readable(char* buffer, PARSE_INSTR* instr) {
#ifdef VERBOSE
	char str[20]; memcpy(str, buffer, 19); str[19] = '\0';
	printf("__set_item_readable(%s, [this_instr])\n", str);
#endif
	int item_str_readable_index;
	int search_result = any_data_search_success(
			buffer, "title=\"", "jpg\" </a>", &item_str_readable_index
		);
	if (search_result == SEARCH_KEY_FOUND) {
		char* start_key = &buffer[item_str_readable_index];
		char* end = strchr(start_key, '\"');
		if (end == NULL) { printf("ERR - no end chr.\n"); return PARSE_FAILED; }
		int item_readable_len = end - start_key;
		if (item_readable_len == TOWN_PORTAL_SCROLL_LEN && start_key[0] == 'T' && start_key[1] == 'o' && start_key[2] == 'w')
			return PARSE_CONTINUE;
		MATCH_DATA* md = &(instr->match_data[match_iteration]);
		set_match_data_str(
				&buffer[item_str_readable_index], item_readable_len, md
			);
		sanitize_html_special_chars(md->data[md->in_use-1]);
#ifdef DEBUG
	printf("ITEM SET %d \"%s\"\n", md->in_use, md->data[md->in_use-1]);
#endif
		return PARSE_CONTINUE;
	} else { printf("ERR - malformed item.\n"); return PARSE_FAILED; }
}
static int set_ability_readable(char* buffer, PARSE_INSTR* instr) {
	int ability_str_readable_index;
	int search_result = any_data_search_success(
			buffer, "title=\"", "jpg\" </a>", &ability_str_readable_index
		);
	if (search_result == SEARCH_KEY_FOUND) {
		char* start_key = &buffer[ability_str_readable_index];
		if (strncmp(start_key, "Talent: ", 8) == 0)
			start_key += 8;
		char* end = strchr(start_key, '\"');
		if (end == NULL) { printf("ERR - no end chr.\n"); return PARSE_FAILED; }
		int ability_readable_len = end - start_key;
		MATCH_DATA* md = &(instr->match_data[match_iteration]);
		set_match_data_str(
				start_key, ability_readable_len, md
			);
		sanitize_html_special_chars(md->data[md->in_use-1]);
#ifdef DEBUG
	printf("SKILL BUILD SET %d \"%s\"\n", md->in_use, md->data[md->in_use-1]);
#endif
		return PARSE_CONTINUE;	} else { printf("ERR - malformed ability.\n"); return PARSE_FAILED; }
}

/*		PARSING INSTRUCTIONS		*/
PARSE_INSTR parse_hero_href = {"data-tooltip-url=\"/heroes/", "<h1>", f_hero_href, NULL, 0, NULL}; // maformity end_if_key
PARSE_INSTR parse_hero_name = {"<h1>", "<small>Guides", f_readable_name, NULL, 0, NULL}; // malformity end_if_key
PARSE_INSTR parse_ability_list;
static const PARSE_INSTR parse_ability_list_template = 
		{"image-skill", "Talent Tree", f_ability_list, NULL, 0, NULL}; // indicate talent section
static const PARSE_INSTR parse_match_template[] = {
	{"lane-icon ", "Role", f_lane_role, NULL, 0, NULL}, // malformity end_if_key
	{"role-icon ", "Wards", f_lane_role, NULL, 0, NULL}, // malformity end_if_key
	{"sentry-ward\">", "image-container-item", f_ward_desire, NULL, 0, NULL}, // catch core no wards
	{"<a href=\"/items/", "Starting Items", f_item_build, NULL, 0, NULL}, // indicate next item sect.
	{"image-skill", "Teammates", f_ability_build, NULL, 0, NULL} // indicate end of ability build
};
PARSE_INSTR parse_match[NUM_PARSE_MATCH_INSTR];

static void replace_str_in_instr_keys(char* remove, char* replace) {
	for (int i=0; i<NUM_PARSE_MATCH_INSTR; i++) {
		// Could have replace_key_index iterate, but that limits key dynamics.
		// calling strchr() on the string for '@' first is probably faster as optimized.
		// Probably needs to be deduced on instruction key changes. moving on.
		str_replace(parse_match[i].key, remove, replace);
		str_replace(parse_match[i].end_if_key, remove, replace);
	}
}

static int f_hero_href(int key_type_found, char* buffer, PARSE_INSTR* instruction) {
#ifdef VERBOSE
	char str[20]; memcpy(str, buffer, 19); str[19] = '\0';
	printf("__f_hero_href(%d, %s, [this_instr])\n", key_type_found, str);
#endif
	static char href_buffer[128];
	if (key_type_found == SEARCH_KEY_FOUND) {
		char* end = strchr(buffer, '/');
		if (end == NULL) {
			printf("ERR - no hero href end.\n");
			return PARSE_FAILED;
		}
		int href_len = end - buffer;
		memcpy(href_buffer, buffer, href_len);
		href_buffer[href_len] = '\0';
		replace_str_in_instr_keys("@hero_href", href_buffer);
		return PARSE_END;
	}
	return PARSE_FAILED;
}
static int f_readable_name(int key_type_found, char* buffer, PARSE_INSTR* instruction) {
	if (key_type_found == SEARCH_KEY_FOUND) {
#ifdef VERBOSE
		char str[20]; memcpy(str, buffer, 19); str[19] = '\0';
		printf("__f_readable_name(%d, %s, [this_instr])\n", key_type_found, str);
#endif
		char* end = strchr(buffer, '<');
		if (end == NULL) { 
			printf("ERR - no readable name end.\n");
			return PARSE_FAILED;
		}
		int name_len = end - buffer;
		strncpy(CurrHeroData.heroReadable, buffer, name_len);
		CurrHeroData.heroReadable[name_len] = '\0';
		sanitize_html_special_chars(CurrHeroData.heroReadable);

		// Check for ignores
		for (int i=0; i<ignore_ability_arr.hero_count; i++) {
#ifdef VERBOSE
			printf("%d %s %s check ignore\n", i, ignore_ability_arr.data[i].hero_name, CurrHeroData.heroReadable);
#endif
			if (strcmp(CurrHeroData.heroReadable, ignore_ability_arr.data[i].hero_name) == 0) {
				printf("Ignore abilities is on for %s\n", ignore_ability_arr.data[i].hero_name);
				ignore_hero_index = i;
				break;
			}
		}

		if (process_set_data(PR_HERO_READABLE, NULL) == PROCESS_SUCCESS)
			return PARSE_END;
		else
			return PARSE_FAILED;
	} else {
		return PARSE_FAILED; 
	}
}
static int f_ability_list(int key_type_found, char* buffer, PARSE_INSTR* instruction) {
#ifdef VERBOSE
	char str[20]; memcpy(str, buffer, 19); str[19] = '\0';
	printf("__f_ability_list(%d, %s, [this_instr])\n", key_type_found, str);
#endif
	MATCH_DATA* md = &(instruction->match_data[match_iteration]);
	if (instruction->state == 0) {
		if (key_type_found == SEARCH_KEY_FOUND) {
			return set_ability_list_ability(buffer, instruction);
		} else {
			instruction->state = 1;
			strcpy(instruction->key, "talent-cell\">");
			strcpy(instruction->end_if_key, "header-content-nav-container");
			if (process_set_data(PR_ABILITY_PRE_TALENT_LEN, &(md->in_use)) == PROCESS_SUCCESS)
				return PARSE_CONTINUE;
			else
				return PARSE_FAILED;
		}
	} else if (key_type_found == SEARCH_KEY_FOUND) {
		char* end = strchr(buffer, '<');
		if (end == NULL) { 
			printf("ERR - no readable name end.\n");
			return PARSE_FAILED;
		}
		int ability_len = end - buffer;
		set_match_data_str(buffer, ability_len, md);
		sanitize_html_special_chars(md->data[md->in_use-1]);
#ifdef DEBUG
printf("ABILITY TALENT SET %d \"%s\"\n", md->in_use, md->data[md->in_use-1]);
#endif
		return PARSE_CONTINUE;
	} else {
		parse_instr_template_cpy(instruction);	
		return PARSE_END;
	}
}
static int f_lane_role(int key_type_found, char* buffer, PARSE_INSTR* instruction) {
	if (key_type_found == PARSE_CONTINUE) {
		char* end = strchr(buffer, '-');
		if (end == NULL) { return PARSE_FAILED; }
		int lane_len = end - buffer;
		MATCH_DATA* md = &(instruction->match_data[match_iteration]);
		set_match_data_str(buffer, lane_len, md);
#ifdef DEBUG
	printf("LANE_ROLE SET %s\n", md->data[md->in_use-1]);
#endif
		return PARSE_END;
	}
	return PARSE_FAILED;
}
static int f_ward_desire(int key_type_found, char* buffer, PARSE_INSTR* instruction) { 
	/* UNIMPLEMENTED */
	return PARSE_END;
}
static int f_item_build(int key_type_found, char* buffer, PARSE_INSTR* instruction) {
	int state = instruction->state;
#ifdef VERBOSE
	char str[40];
	str[39] = '\0';
	memcpy(str, buffer, 39);
	printf("__f_item_build(%d, \"%s\", [instr]) STATE:%d\n", key_type_found, str, state);
#endif
	switch(state) {
	case 0:
		if (key_type_found == SEARCH_KEY_FOUND) {
			return set_item_readable(buffer, instruction);
			return PARSE_CONTINUE;
		} else { // SEARCH_END_FOUND
			instruction->state = 1;
			strcpy(instruction->end_if_key, "image-container-skill");
			if (process_set_data(
						PR_M_STARTING_ITEM_LEN, 
						&(instruction->match_data[match_iteration].in_use)
					) == PROCESS_SUCCESS)
				return PARSE_CONTINUE;
			else
				return PARSE_FAILED;
		}
	case 1:
		if (key_type_found == SEARCH_KEY_FOUND) {
			return set_item_readable(buffer, instruction);
		} else {
			parse_instr_template_cpy(instruction);
			int item_build_len = instruction->match_data[match_iteration].in_use;
			if (process_set_data(
						PR_M_ITEM_BUILD_LEN, 
						&(instruction->match_data[match_iteration].in_use)
					) == PROCESS_SUCCESS)
				return PARSE_END;
			else
				return PARSE_FAILED;

			return PARSE_END;
		}
		PARSE_FAILED;
	}
}
static int f_ability_build(int key_type_found, char* buffer, PARSE_INSTR* instruction) {
#ifdef DEBUG
	char str[40];
	str[39] = '\0';
	memcpy(str, buffer, 39);
	printf("__f_ability_build(%d, \"%s\", [instr])\n", key_type_found, str);
#endif
	if (key_type_found == SEARCH_KEY_FOUND) {
		return set_ability_readable(buffer, instruction);
	} else { // SEARCH_END_FOUND
		if (match_iteration == MAX_MATCHES-1) 
			parse_instr_template_cpy(instruction);
		if (process_set_data(
					PR_M_ABILITY_BUILD_LEN, 
					&(instruction->match_data[match_iteration].in_use)
				) == PROCESS_SUCCESS)
			return PARSE_END;
		else
			return PARSE_FAILED;
	}
	return PARSE_FAILED;
}
/*		END PARSING INSTRCUTIONS */

/* STATEVAL */static int buffer_unsearched = 0;
static int do_parse_instr(FILE* hero_guide, PARSE_INSTR* instr) {
	static char buffer[MAX_HTML_READ+1]; // this is faster, yes? avoid rdclr large arr
	int next_seek;
	/* Get first (non-key, non-next) buffer */
	if (buffer_unsearched == 0) {
		if (fread(&buffer, sizeof(char), MAX_HTML_READ, hero_guide) == 0) {
			printf("ERR - Guide has no data.\n");
			return PARSE_FAILED;
		}
	}
	/* ... Loop a buffer size to find key or end_if_key.. */
	while (1) {
		buffer[MAX_HTML_READ] = '\0'; // NULL-terminate first and continue buffers
		/* search */
		int search_result = any_data_search_success(
				buffer, 
				instr->key, 
				instr->end_if_key,
				&next_seek
			);
		buffer_unsearched = 0;
		/* set seek to the end of the key we found and continue, or exit */	
		if (search_result != SEARCH_CONTINUE) {
			/* set to end of key and run parse_func */
			fseek(hero_guide, next_seek-MAX_HTML_READ, SEEK_CUR);
			fread(&buffer, sizeof(char), MAX_HTML_READ, hero_guide); // non-zero.
			buffer_unsearched = 1;
#ifdef VERBOSE
	char str[40]; memcpy(str, buffer, 39); str[39] = '\0';
	printf("FSEEK for parse %d. '%s'||'%s' ..%s...\n",
			next_seek-MAX_HTML_READ, instr->key, instr->end_if_key,
			str
		);
#endif
			buffer[MAX_HTML_READ] = '\0';

			int parse_result = (instr->f)(search_result, buffer, instr);
			if ( parse_result == PARSE_END ) {
				return PARSE_CONTINUE;
			} else if ( parse_result == PARSE_FAILED )
				return PARSE_FAILED;
		/* Read next continued data */
		} else {
			fseek(hero_guide, next_seek-MAX_HTML_READ, SEEK_CUR);
			buffer_unsearched = 1; // pre-set
			if (fread(&buffer, sizeof(char), MAX_HTML_READ, hero_guide) == 0) {
				printf("ERR - EOF do_parse_instr(..)\n");
				return PARSE_FAILED; // EOF failure, because parse function did not end.
			}
#ifdef VERBOSE
	char str[40]; memcpy(str, buffer, 39); str[39] = '\0';
	printf("FSEEK %d. '%s' || '%s'\n ..%s...\n",
			next_seek-MAX_HTML_READ, instr->key, instr->end_if_key,
			str
		);
#endif
		}
	}
	return PARSE_CONTINUE;
}

/*		INTERFACE		*/
// Uses the array of PARSE_INSTR to call the functions inside each for any keys found.
// PARSE_INSTR func will return error if bad / missing data.
int parse_hero_guide(FILE* hero_guide) {
	/* Parse hero name */
//	if (do_parse_instr(hero_guide, &parse_hero_href) != PARSE_CONTINUE)
//		return PARSE_FAILED;
	if (do_parse_instr(hero_guide, &parse_hero_name) != PARSE_CONTINUE)
		return PARSE_FAILED;
	if (do_parse_instr(hero_guide, &parse_ability_list) != PARSE_CONTINUE)
		return PARSE_FAILED;
	/* For each match for this hero.. */
	PARSE_INSTR* pm = parse_match;
	while(match_iteration < MAX_MATCHES) {
		/* For each parse_match instruction, ordered */
		for (int i_instr=0; i_instr<NUM_PARSE_MATCH_INSTR; i_instr++) {
			if ( do_parse_instr(hero_guide, &(pm[i_instr])) != PARSE_CONTINUE ) {
				printf("ERR - Parsing failed. '%s'||'%s', Match %d.\n",
						pm->key, pm->end_if_key, match_iteration);
				return PARSE_FAILED;
			}
		}
#ifdef DEBUG
	printf("~~END MATCH~~~ %s\n", CurrHeroData.heroReadable);
#endif
		match_iteration++;
	}
	/* Set CurrHeroData object to prependable type data */
	process_analyse_and_set_prepend_data();
	return PARSE_CONTINUE;
}

static void parse_instr_template_cpy(PARSE_INSTR* instr) {
	const PARSE_INSTR* T = instr->template;
	strcpy(instr->key, T->key);
	strcpy(instr->end_if_key, T->end_if_key);
	instr->state = T->state;
}
static void alloc_parse_instr(PARSE_INSTR* instr, const PARSE_INSTR* template) {
	instr->f = template->f;
	instr->match_data = malloc(sizeof(MATCH_DATA)*MAX_MATCHES);
	instr->template = template;
	for (int i_match=0; i_match<MAX_MATCHES; i_match++) {
		instr->match_data[i_match].length = calloc(
				MAX_MATCH_DATA_INDICES,
				sizeof(int)
			);
	}
}
void zero_state_parse_match_instr() {
	if (parse_match[0].f == NULL) {
		// alloc ability list instr
		alloc_parse_instr(&parse_ability_list, &parse_ability_list_template);
		// alloc match-based instr
		for (int i=0; i<NUM_PARSE_MATCH_INSTR; i++) {
			alloc_parse_instr(&(parse_match[i]), &(parse_match_template[i]));
		// give static refs to parsed data for process.c
		process_link_static_indices(
				&(parse_ability_list), &(parse_match[0]), &(parse_match[1]), 
				&(parse_match[2]), &(parse_match[3]), &(parse_match[4])
			);
		}
	}
	parse_instr_template_cpy(&parse_ability_list);
	parse_ability_list.match_data->in_use = 0;
	for (int i=0; i<NUM_PARSE_MATCH_INSTR; i++) {
		parse_instr_template_cpy(&(parse_match[i]));
		for (int i_match=0; i_match<MAX_MATCHES; i_match++)
			parse_match[i].match_data[i_match].in_use = 0;
	}
	match_iteration = 0;
	buffer_unsearched = 0;
}

static void free_match_data(PARSE_INSTR* instr) {
	for (int i_match=0; i_match<MAX_MATCHES; i_match++) {
		MATCH_DATA* md = &(instr->match_data[i_match]);
		int i_match_str_data = 0;
		while(i_match_str_data < MAX_MATCH_DATA_INDICES) {
			if (md->length[i_match_str_data] == 0) {
				// was not alloc'd
				break;
			}
			free(md->data[i_match_str_data++]);
			i_match_str_data++;
		}
		free(md->length);
	}
	free(instr->match_data);
}
static void free_ignore_ability_arr() {
	for (int i=0; i<ignore_ability_arr.hero_count; i++) {
		IGNORE_DATA* ignoreData = &ignore_ability_arr.data[i];
		for (int k=0; k<ignoreData->ability_count; k++) {
			free(ignoreData->abilities[k]);
		}
		free(ignoreData->abilities);
		free(ignoreData);
	}
}
void free_parse_data() {
	free_match_data(&parse_ability_list);
	for (int i=0; i<NUM_PARSE_MATCH_INSTR; i++) {
		free_match_data(&(parse_match[i]));
	}
	free_ignore_ability_arr();
	// (others have no match data)
}
int set_ability_ignore_data(const char* filename) {
	FILE* fp_ability_ignore = fopen(filename, "r");
	if (fp_ability_ignore == NULL)
		return PARSE_FAILED;

	char buffer[512];

	ignore_ability_arr.hero_count = 0;
	IGNORE_DATA** ignoreAbilityDataArr = &ignore_ability_arr.data;
	*ignoreAbilityDataArr = NULL;
	while (fgets(buffer, 512, fp_ability_ignore) != NULL) {
		char* end_str = strchr(buffer, ABILITY_IGNORE_DAT_DELIM);
		if (end_str == NULL) {
			break;
		}
		// ++ new hero ignore data
		*ignoreAbilityDataArr = (IGNORE_DATA*)realloc((*ignoreAbilityDataArr), // about 5-15 heroes
				sizeof(IGNORE_DATA) * (ignore_ability_arr.hero_count+1));
		if (*ignoreAbilityDataArr == NULL) {
			printf("Err - Unable to allocate IGNORE_DATA\n.");
			return PARSE_FAILED;
		} else { ignore_ability_arr.hero_count++; }

		IGNORE_DATA* ignoreData = &(*ignoreAbilityDataArr)[ignore_ability_arr.hero_count-1];
		ignoreData->abilities = NULL;

		// hero name
		short name_len = end_str-buffer;
		strncpy(ignoreData->hero_name, buffer, name_len);
		ignoreData->hero_name[name_len] = '\0';
		ignoreData->ability_count = 0;

#ifdef DEBUG
		printf("Ignore Ability Line: %s\n", buffer);
		printf("Ignores for Hero \"%s\"...\n", ignoreData->hero_name);
#endif

		// add the ignorable abilities
		while(1) {
			char* this_name = &end_str[1]; // skip to next after delim
			if (this_name != NULL && strlen(this_name) > 1) {
				char*** abilityArr = &(ignoreData->abilities);

				// ++ new ability string
				*abilityArr = (char**)realloc(*abilityArr, // usually only happens a one to four times
						sizeof(char**) * ignoreData->ability_count+1);

				if (*abilityArr == NULL) {
					printf("Err - Unable to allocate ability char* pointer");
					return PARSE_FAILED;
				} else { ignoreData->ability_count++; };

				char** abilityStr = &(*abilityArr)[ignoreData->ability_count-1];
				*abilityStr = (char*)malloc(sizeof(char)*MAX_HERO_OR_ABILITY_NAME_LEN);

				end_str = strchr(this_name, ABILITY_IGNORE_DAT_DELIM);
				if (end_str == NULL) {
					// correct for end of line / final line
					end_str = strchr(this_name, '\n');
					if (end_str == NULL) {
						end_str = this_name + strlen(this_name);
					}
				}

				// ability name
				short name_len = end_str - this_name;
				strncpy(*abilityStr, this_name, name_len);
				(*abilityStr)[name_len] = '\0';
#ifdef DEBUG
				printf("IGNORE ability %d, %s\n", ignoreData->ability_count, *abilityStr);
#endif
			} else break;
		} 
	}
	return PARSE_CONTINUE;
}
