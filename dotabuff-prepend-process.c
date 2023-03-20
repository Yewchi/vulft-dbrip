#include "dotabuff-prepend.h"

#define MAX_HASH_INDEX 256
#define BLTIN_ITEM_FN "readable-to-built-in.dat"
#define BLTIN_HERO_FN "built-in-diff.dat"
#define BLTIN_DELIM '@'

typedef struct hash_list_node HASH_NODE;
typedef struct hash_list_node {
	char* rdbl;
	char* bltin;
	HASH_NODE* next;
} HASH_NODE;
HASH_NODE* hash_lists[MAX_HASH_INDEX];

extern HERO_DATA CurrHeroData;
extern int match_iteration;

// At post-parse processing
// 	// each is a pointer-to-the-pointer of data, the data may be alloced
// 	// freed and realloced multiple times.
static char** ability_list; // hero ability information
static int ability_pre_talent_len;
static int ability_list_len;
static char** role[MAX_MATCHES]; // role
static char** lane[MAX_MATCHES]; // lane
static char** wards[MAX_MATCHES]; // ward frequency
static char** item_build[MAX_MATCHES]; // item build
static int starting_item_len[MAX_MATCHES];
static int item_build_len[MAX_MATCHES];
static char** ability_build[MAX_MATCHES]; // ability build
static int ability_build_indices[MAX_MATCHES][MAX_SKILL_UPS];
static int ability_build_len[MAX_MATCHES];
// /At

static void hash_insert(char* rdbl, char* bltin) {
	int hash = (rdbl[0]*41 + rdbl[1]*11 - rdbl[2]) % 256;
	HASH_NODE* new = calloc(1, sizeof(HASH_NODE));
	new->rdbl = rdbl;
	new->bltin = bltin;
	if (hash_lists[hash] != NULL)
		new->next = hash_lists[hash];
	hash_lists[hash] = new;
}
static char* to_bltin(char* rdbl) {
	int hash = (rdbl[0]*41 + rdbl[1]*11 - rdbl[2]) % 256;
	HASH_NODE* node = hash_lists[hash];
	while (node != NULL) {
		if (strcmp(node->rdbl, rdbl) == 0) {
			return node->bltin;
		}
		node = node->next;
	}
	//printf("Warning -- Could not resolve bltin from \"%s\".\n", rdbl);
	return NULL;
}
static int lane_to_planar_number(char* lane) {
	switch(lane[0]) {
	case 's':
		return 1;
	case 'm':
		return 2;
	case 'o':
		return 3;
	case 'j':
		return 4;
	case 'r':
		return 5;
	}
}
static int lane_role_to_planar_number(int lane, char* role) {
	switch(role[0]) {
	case 'c':
		switch(lane) {
		case 1:
			return 1;
		case 2:
			return 2;
		case 3:
			return 3;
		case 4:
			return 4;
		case 5:
			return 4;
		}
	case 's':
		switch(lane) {
		case 1:
			return 5;
		case 2:
			return 4;
		case 3:
			return 4;
		case 4:
			return 5;
		case 5:
			return 4;
		}
	}
}
static void whisk_talents_from_frequency(
		int* ability_points_spendable, int at_level_ability_freq[][MAX_SKILL_UPS],
		int* max_ability_levels, int max_ability_build_len,
		int talent_flip, int whisk) {
	for (int point=0; point<max_ability_build_len; point++) {
		if (CurrHeroData.abilityBuild[point] != -1)
			continue; // point is allocated
		for (int ability=0; ability<ability_list_len; ability++) {
			if (ability_points_spendable[ability] > 0
					&& at_level_ability_freq[ability][point] >= whisk) {
				int may_invest_at_level = (point)/2 + 1; // 0th 1st... point index 0
				// ensure it is skillable at this level, with what we have
				if (may_invest_at_level < max_ability_levels[ability]) {
					for (int reverse=(point%2==0?point+1:point-1); reverse>=0; reverse--) {
						if (CurrHeroData.abilityBuild[reverse] == ability)
							may_invest_at_level--;
					}
					if (may_invest_at_level == 0)
						continue;
				}
				// if it is a talent, remove the other talent from spendable
				if (ability >= ability_pre_talent_len) {
					int paired_index = ability - ability_pre_talent_len;
					paired_index = ability + (paired_index%2==0 ?
							talent_flip : -talent_flip);
					ability_points_spendable[paired_index]--;
				}
				ability_points_spendable[ability]--; // < reason whisk is in 3
				CurrHeroData.abilityBuild[point] = ability;	
				ability = 1893;
			}
		}
	}
}
static void deduce_snapshot_meta_ability_build() { // Hungry
	// The decremented list of skill-points invested to create the final build
	static int ability_points_spendable[MAX_ABILITY_SLOTS];
	// Frequency of skill ups per ability slot at a skill-point order (or level)
	static int at_level_ability_freq[MAX_ABILITY_SLOTS][MAX_LEVEL_SKILL_UP];
	// The running count of number of times an ability slot was invested
	// / as match - per highest invested. (it's just an easy memset)
	static int highest_invested_for_ability[MAX_ABILITY_SLOTS][MAX_MATCHES];
	static int max_ability_levels[MAX_ABILITY_SLOTS];
	int max_ability_build_len = ability_build_len[0];
	for (int k=1; k<MAX_MATCHES; k++) { // try to get max ability points
		if (ability_build_len[k] > max_ability_build_len)
			max_ability_build_len = ability_build_len[k];
	}
	memset(
			at_level_ability_freq,
			0,
			MAX_SKILL_UPS*max_ability_build_len*sizeof(int) // usually 8 / 23 set
		);
	memset(highest_invested_for_ability, 0, MAX_MATCHES*ability_list_len*sizeof(int));
	memset(max_ability_levels, 0, MAX_ABILITY_SLOTS*sizeof(int));
	memset(ability_points_spendable, 0, ability_list_len*sizeof(int));
	memset(CurrHeroData.abilityBuild, -1, MAX_SKILL_UPS*sizeof(int));
	for (int k=0; k<MAX_MATCHES; k++) {
		for (int j=0; j<ability_build_len[k]; j++) {
			// Ability Index / Order Invested = number invested at invested order index (or, usually: "at level")
			int ability_index = ability_build_indices[k][j];
			at_level_ability_freq[ability_index][j]++;
			highest_invested_for_ability[ability_index][k]++;
			if (highest_invested_for_ability[ability_index][k] >
					max_ability_levels[ability_index]) {
				max_ability_levels[ability_index] = 
					highest_invested_for_ability[ability_index][k];
			}
		}
	}
	for(int i=0;i<ability_list_len;i++) { ability_points_spendable[i] = max_ability_levels[i]; }
	// whisk the highest frequency off the top of the list and apply the
	// / skill-up to that skill-up-order-index.
	int talent_flip = ability_pre_talent_len%2==0?1:-1;
	for(int i=5; i>1; i--) {
		whisk_talents_from_frequency(
			ability_points_spendable, at_level_ability_freq,
			max_ability_levels, max_ability_build_len,
			talent_flip, i
		);	
	}
	// Cascade the remaining level ups in basic->ult->talent
	int lvl=0;
	int ability=0;
	while(ability<ability_list_len) {
		if (ability_points_spendable[ability] > 0) {
			if (CurrHeroData.abilityBuild[lvl] == -1) {
				if (ability >= ability_pre_talent_len) {
					int paired_index = ability - ability_pre_talent_len;
					paired_index = ability + (paired_index%2==0 ?
							talent_flip : -talent_flip);
					ability_points_spendable[paired_index]--;
				}
				CurrHeroData.abilityBuild[lvl] = ability;
				ability_points_spendable[ability]--;
			}
			if (++lvl > MAX_LEVEL_SKILL_UP) {
				printf("Warning - \"%s\" exceeded level when cascading "
						"remaining abilities\n", CurrHeroData.heroReadable);
				break;
			}
		} else
			ability++;
	}
}
static int rdbl_to_ability_index(char* rdbl) {
	int index = -1;
	for (int i=0; i<ability_list_len; i++) {
		if (strcmp(rdbl, ability_list[i]) == 0) {
#ifdef VERBOSE
			printf("Search for \"%s\" found \"%s\" at %d.\n",
					rdbl, ability_list[i], i);
#endif
			if (i >= ability_pre_talent_len) {
				return ability_list_len + ability_pre_talent_len - 1 - i;
			}
			return i;
		}
	}
}
static void set_ability_build_ability_indices() {
	for (int m=0; m<MAX_MATCHES; m++) {
		for (int i=0; i<ability_build_len[m]; i++) {
			ability_build_indices[m][i] =
				rdbl_to_ability_index(ability_build[m][i]);
		}
	}
}
int process_set_data(PROCESS_ACTION process_action, void* data) {
	if (process_action == PR_HERO_READABLE) {
		// heroReadable sanitized and set in parser.c
		int built_in_result = form_hero_built_in_name(
				CurrHeroData.heroReadable,
				CurrHeroData.heroBuiltIn
			);
		if (built_in_result != 0) { 
			printf("ERR - Failed built-in creation from '%s'", 
					CurrHeroData.heroReadable
			);
		return PROCESS_FAILED;
		}
#ifdef DEBUG
	printf(" %s, %s\n", CurrHeroData.heroReadable, CurrHeroData.heroBuiltIn);
#endif
		return PROCESS_SUCCESS;
	} else if (process_action == PR_ABILITY_PRE_TALENT_LEN) {
		ability_pre_talent_len = *((int*)data); // we get in_use pre +1
		ability_list_len = ability_pre_talent_len + 8;
		return PROCESS_SUCCESS;
	} else if (process_action == PR_M_STARTING_ITEM_LEN) {
		starting_item_len[match_iteration] = *((int*)data);
		return PROCESS_SUCCESS;
	} else if (process_action == PR_M_ITEM_BUILD_LEN) {
		item_build_len[match_iteration] = *((int*)data);
		return PROCESS_SUCCESS;
	} else if (process_action == PR_M_ABILITY_BUILD_LEN) {
		ability_build_len[match_iteration] = *((int*)data);
		return PROCESS_SUCCESS;
	}
	return PROCESS_FAILED;
}

void frequency_sort(int* roleOrLaneTbl) {
	int* frequency = (int*)calloc(MAX_LANE_TYPE, sizeof(int));
	int highestFreq = 1;
	int winningNumerical = 0;
	for (int i=0; i<MAX_LANE_TYPE; i++) {
		if (roleOrLaneTbl[i] < 1)
			break;
		frequency[roleOrLaneTbl[i]]++;
		if (frequency[roleOrLaneTbl[i]] > highestFreq) {
			highestFreq++; // will always catch 1-up
			winningNumerical = roleOrLaneTbl[i];
		}
	}
	for (int i=0; i<highestFreq; i++) {
		if (roleOrLaneTbl[i] != winningNumerical) {
			for (int j=i+1; j<MAX_LANE_TYPE; j++) {
				if (roleOrLaneTbl[j] == winningNumerical) {
					roleOrLaneTbl[j] = roleOrLaneTbl[i];
					roleOrLaneTbl[i] = winningNumerical;
				}
			}
		}
	}
	if (highestFreq == 2) {
		if (roleOrLaneTbl[3] == roleOrLaneTbl[5]) {
			int tmp = roleOrLaneTbl[4];
			roleOrLaneTbl[4] = roleOrLaneTbl[5];
			roleOrLaneTbl[5] = tmp;
		} else if (roleOrLaneTbl[4] == roleOrLaneTbl[5]) {
			int tmp = roleOrLaneTbl[3];
			roleOrLaneTbl[3] = roleOrLaneTbl[5];
			roleOrLaneTbl[5] = tmp;
		}
	}
	return;
}

int process_analyse_and_set_prepend_data() {
	printf("Process %s\n", CurrHeroData.heroBuiltIn);
/*	for (int i=0; i<MAX_MATCHES; i++) {
		printf("%s, %s, %s, %s\n",
				*role[i], *lane[i], item_build[i][2], ability_build[i][4]
			);
	}*/
	/* TEMPORARY */
	short itemBuildIndex = 0;
	short largestItemBuildSize = 0;
	for(int i=0; i<MAX_MATCHES; i++) {
		if (item_build_len[i] > largestItemBuildSize) {
			itemBuildIndex = i;
			largestItemBuildSize = item_build_len[i];
		}
	}
	short bootsFound = 0;
	short skipped = 0;
	for(int i=0; i<item_build_len[itemBuildIndex]; i++) {
		char* builtInItem = to_bltin(item_build[itemBuildIndex][i]);
		if (builtInItem == NULL) {
			skipped++; // Unknown item
		} else {
			if (builtInItem != NULL && strcmp(builtInItem, "item_boots") == 0) {
				if (bootsFound) { 
					skipped++; // bought boots twice
				} else {
					bootsFound = 1;
					CurrHeroData.itemBuild[i-skipped]
						= builtInItem;
				}
			} else {
				CurrHeroData.itemBuild[i-skipped]
					= builtInItem;
			}
		}
	}
	largestItemBuildSize -= skipped;
	CurrHeroData.itemBuildLen = largestItemBuildSize;
	for(int i=0; i<ability_list_len; i++) {
		CurrHeroData.abilityIndexToName[i] =
				ability_list[i];
	}
	for (int i=0; i < MAX_TALENTS; i++) { // talents are loaded top to bottom
		CurrHeroData.abilityIndexToName[ability_pre_talent_len+i] =
				ability_list[ability_list_len-i-1];
	}
	CurrHeroData.abilityIndexToNameLen = ability_list_len;

	set_ability_build_ability_indices();
	deduce_snapshot_meta_ability_build();

	/* MOVE TO ROLE AND LANE FUNCTIONS */
	memset(CurrHeroData.preferredLanes, 0, sizeof(int)*MAX_MATCHES);
	memset(CurrHeroData.preferredRoles, 0, sizeof(int)*MAX_MATCHES);
	for (int i=0; i<MAX_LANE_TYPE; i++) {
		CurrHeroData.preferredLanes[i] = lane_to_planar_number(lane[i][0]);
		CurrHeroData.preferredRoles[i] = lane_role_to_planar_number(
				CurrHeroData.preferredLanes[i],
				role[i][0]
			);
	}
	frequency_sort(CurrHeroData.preferredLanes);
	frequency_sort(CurrHeroData.preferredRoles);

	CurrHeroData.soloMultiplier = 0.1;
	/* /TEMPORARY */
#ifdef DEBUG
	printf("%s, %s\n", CurrHeroData.heroReadable, CurrHeroData.heroBuiltIn);
	for (int i=0; i<16; i++) {
		printf("%s\n", CurrHeroData.itemBuild[i]);
	}
	printf("\"%s\": %d\n", lane[0][0], CurrHeroData.preferredLanes[0]);
	printf("\"%s\": %d\n", role[0][0], CurrHeroData.preferredRoles[0]);
	for (int i=0; i<12; i++) {
		printf("%s\n", CurrHeroData.abilityIndexToName[i]);
	}
	for (int i=0; i<19; i++) {
		printf("%d\n", CurrHeroData.abilityBuild[i]);
	}
#endif
	return PROCESS_SUCCESS;
}

static int load_rdbl_to_bltin(char* filename, char delim) {
	FILE* rdbl_to_bltin_fp = fopen(filename, "r");
	char *rdbl, *bltin;
	char* delim_ptr;
	char line_buffer[256];
	if ((rdbl_to_bltin_fp = fopen(filename, "r")) != NULL) {
		while(fgets(line_buffer, 254, rdbl_to_bltin_fp) != NULL) {
			delim_ptr = strchr(line_buffer, delim);
			if (delim_ptr != NULL) {
				int rdbl_len = delim_ptr - line_buffer;
				int bltin_len = strlen(&(line_buffer[rdbl_len+1])) - 1; // \n
				rdbl = malloc(sizeof(char)*(rdbl_len+1));
				bltin = malloc(sizeof(char)*(bltin_len+1));
				memcpy(rdbl, line_buffer, rdbl_len);
				memcpy(bltin, ++delim_ptr, bltin_len);
				rdbl[rdbl_len] = '\0';
				bltin[bltin_len] = '\0';

				hash_insert(rdbl, bltin);
			}
		}
		fclose(rdbl_to_bltin_fp);
		return PROCESS_SUCCESS;
	} else
		return PROCESS_FAILED;
}
int create_readable_to_built_in_hash() {
	if (load_rdbl_to_bltin(BLTIN_ITEM_FN, BLTIN_DELIM) == PROCESS_SUCCESS
			&& load_rdbl_to_bltin(BLTIN_HERO_FN, BLTIN_DELIM) == PROCESS_SUCCESS)
		return PROCESS_SUCCESS;
	return PROCESS_FAILED;
}


void process_link_static_indices(
		PARSE_INSTR* instr_ability_list, PARSE_INSTR* instr_lane,
		PARSE_INSTR* instr_role, PARSE_INSTR* instr_wards,
		PARSE_INSTR* instr_item_build, PARSE_INSTR* instr_ability_build) {
	ability_list = instr_ability_list->match_data[0].data;
	for (int i=0; i<MAX_MATCHES; i++) {
		wards[i] = instr_wards->match_data[i].data;
		item_build[i] = instr_item_build->match_data[i].data;
		ability_build[i] = instr_ability_build->match_data[i].data;
		// role / lane 1 element
		lane[i] = instr_lane->match_data[i].data;
		role[i] = instr_role->match_data[i].data;
	}
}

int form_hero_built_in_name(char* hero_readable, char* built_in) {
	char* s = to_bltin(hero_readable);
	if (s != NULL) {
		strcpy(built_in, s);
#ifdef DEBUG
	printf("BLTIN REPLACE Found \"%s\" -> \"%s\"\n", hero_readable, s);
#endif
		return 0;
	}
	int i = 0;
	s = hero_readable;
	printf("  Checking string '%s'.\n", s);
	while(s[i] != '\0') {
		if ((s[i] >= 'a' && s[i] <= 'z') || (s[i] == '\'')) {
			built_in[i] = s[i];
		} else if (s[i] == '-' || s[i] == ' ') {
			built_in[i] = '_';
		} else if (s[i] >= 'A' && s[i] <= 'Z') {
			built_in[i] = s[i] + 0x20;
		} else {
			built_in[i]='\0';
			printf("ERR - Invalid readable '%c'. Had \"%s\".", s[i], s);
			return -1;
		}
		i++;
	}
	built_in[i] = '\0';
	return 0;
}

void str_replace(char* str, char* remove, char* replace) {
	int i = 0;
	int str_len = strlen(str);
	int remove_len = strlen(remove);
	int last_possible_i = str_len - remove_len;
	while(i<=last_possible_i) {
		int i_offset = 0;
		while(str[i+i_offset] == remove[i_offset]) {
			i_offset++;
			if(remove[i_offset] == '\0') {
				i_offset--;
				int replace_len = strlen(replace);
				int resultant_len = str_len - remove_len + replace_len;
				memcpy(&(str[i+replace_len]), &(str[i+remove_len]), resultant_len-i);
				memcpy(&(str[i]), replace, replace_len);
				str[resultant_len] = '\0';
#ifdef DEBUG
				printf("REPLACED \"%s\"->\"%s\": \"%s\"\n", remove, replace, str);
#endif
				return;
			}
		}
		i++;
	}
	return;
}

void free_process_hash_lists() {
	HASH_NODE* node;
	for (int i=0; i<MAX_HASH_INDEX; i++) {
		node = hash_lists[i];
		while(node != NULL) {
			HASH_NODE* tmp = node->next;
			free(node->rdbl);
			free(node->bltin);
			free(node);
			node = tmp;
		}
	}
}
