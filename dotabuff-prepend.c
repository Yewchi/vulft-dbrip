#include "dotabuff-prepend.h"
//#include "dotabuff-prepend-o.h"

// Early version. Will not notice Ember Spirit is using Magic / Physical build.
// Parsed currently into Lua array.

struct hero_data CurrHeroData;

void set_guide_number_string(int num, char* number_string) {
	memset(number_string, '\0', 3);
	if(num<10) {
		number_string[0] = 0x30 + num;
	} else if(num<100) {
		number_string[0] = 0x30 + num/10;
		number_string[1] = 0x30 + num%10;
	} else {
		number_string[0] = 0x30 + num/100;
		number_string[1] = 0x30 + num%100/10;
		number_string[2] = 0x30 + num%10;
	}
	return;
}

int main(int argc, char** argv) {
	setvbuf(stdout, 0, _IONBF, 0);
	FILE* fp_hero_names = fopen("hero-list.dat", "r");
	FILE* fp_hero_name_diffs = fopen("built-in-diff.dat", "r");
	FILE* fp_solo_potential = fopen("solo-potential.dat", "r");
	FILE* fp_readable_to_built_in_items = fopen("readable-to-built-in.dat", "r");
	FILE* this_hero_guide;
	char* guide_name = calloc(20, sizeof(char));
	char* number_string = calloc(4, sizeof(char));
	strcpy(guide_name, "guides.");
	int index_guide_name_num = strlen(guide_name);
	int n = 1;
	int single_run_guide_num = DO_NOT_RUN_SINGLE;
	int full_run = 1;

	printf("Locked and loaded..\nSummoning demons...\n");

	// Check if single run
	if (argc == 2) {
		single_run_guide_num = atoi(argv[1]);
		if (single_run_guide_num != 0) {
			n = single_run_guide_num;
			full_run = 0;
		}
		printf("attempting to run single on %s%d\n", guide_name, n);
	}

	// and other file checks
	if (create_readable_to_built_in_hash() != PROCESS_SUCCESS) {
		printf("Err - Process file open \"r\" failed.\n");
		return EXIT_FAILURE;
	} else if (out_initialize() != OUT_SUCCESS) {
		printf("Err - Output buffer alloc failed\n");
		free_process_hash_lists();
		return EXIT_FAILURE;
	}


	//f_readable_name(PARSE_CONTINUE, guide_name, &parse_hero_name);
	//alloc_parse_match_data();
	while(full_run || n == single_run_guide_num) {
		set_guide_number_string(n, number_string);
		strcpy(&guide_name[index_guide_name_num], number_string);
		//strcpy(&guide_name[strlen(guide_name)], ".dat");
		printf("%s\n", guide_name);
		if((this_hero_guide = fopen(guide_name, "r")) != NULL) {
			zero_state_parse_match_instr(); // prep parser for next guide
			if (parse_hero_guide(this_hero_guide) == PARSE_CONTINUE) { // parse
				printf("%s data loaded.\n", CurrHeroData.heroReadable);
			} else {
				printf("Exit - Failed on %s.\n", guide_name);
				break;
			}
		} else
			break;
		n++;
		if (CurrHeroData.heroReadable != NULL) {
			prepend_hero_file();
		}
	}

	printf("\nEnded parsing, exiting..\n");

	fclose(fp_hero_names);
	fclose(fp_hero_name_diffs);
	fclose(fp_solo_potential);
	fclose(fp_readable_to_built_in_items);

	free_parse_data();
	free_process_hash_lists();
	free_out();

	free(guide_name);
	free(number_string);

	printf("Exit...\n");

	fflush(stdout);

	return 0;
}
