#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUILT_IN_DAT_FILE_NAME "readable-to-built-in.dat"
#define ITEM_KEY "DOTA_Tooltip_Ability_i"
#define ITEM_KEY_FIRST_CHAR 'D'

int search_len = strlen(ITEM_KEY);
char* set_any_item_readable_pair(char* line, char* built_in, char* readable) {
	short len = strlen(ITEM_KEY);
	for (int i=0;i<len;i++) {
		if(line[i] == ITEM_KEY_FIRST_CHAR) {
			if(memcmp(&line[i], ITEM_KEY, len) == 0) {
				i = i+len-1;
				int j=i;
				// cpy to built_in
				while(line[j] != '\"' && line[j] != '\0') {	j++; }
				memcpy(built_in, &line[i], j-i);
				built_in[j-i] = '\0';
				j++;
				// cpy to readable
				while(line[j] != '\"' && line[j] != '\0') { j++; }
				j++;
				i = j;
				while(line[j] != '\"' && line[j] != '\0') { j++; }
				memcpy(readable, &line[i], j-i);
				readable[j-i] = '\0';
				return &line[i];
			}
		}
	}
	return NULL;
}

int main(int argc, char** argv) {
	FILE* d2vpkr = fopen("d2vpkr.dat", "r");
	char* line_buffer = calloc(2000, sizeof(char));
	FILE* readable_to_built_in = fopen(BUILT_IN_DAT_FILE_NAME, "w");

	char* found_item_at;
	char* built_in_name = calloc(512, sizeof(char));
	char* readable_name = calloc(512, sizeof(char));
	char* write_line = calloc(1025, sizeof(char));
	while(fgets(line_buffer, 2000-1, d2vpkr) != NULL) {
		if((found_item_at = set_any_item_readable_pair(line_buffer, built_in_name, readable_name)) != NULL) {
			int len_readable = strlen(readable_name);
			int len_built_in = strlen(built_in_name);
			memcpy(&write_line[0], readable_name, len_readable);
			write_line[len_readable] = '@'; // delim
			memcpy(&write_line[len_readable+1], built_in_name, len_built_in);
			write_line[len_readable+1+len_built_in] = '\n';
			write_line[len_readable+1+len_built_in+1] = '\0';
			printf("%s", write_line);
			fwrite(write_line, sizeof(char), strlen(write_line), readable_to_built_in);
		}
	
	}
	printf("\n - - - seems good, saved to '%s'.", BUILT_IN_DAT_FILE_NAME);

	free(line_buffer);
	free(built_in_name);
	free(readable_name);
	free(write_line);

	return 0;
}
