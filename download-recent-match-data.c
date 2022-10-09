#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <ctype.h>

#ifndef DBD
/* in order to prevent abuse of DotaBuff server, request at local if the user doesn't
 * know what they are doing */
#define DOMAIN "127.0.0.1"
#else
#define DOMAIN "www.dotabuff.com"
#endif

#define DOTABUFF_GUIDE_URL_PREFIX "https://"DOMAIN"/heroes/"
#define DOTABUFF_GUIDE_URL_SUFFIX "/guides"

int PREFIX_LEN = strlen(DOTABUFF_GUIDE_URL_PREFIX);
int SUFFIX_LEN = strlen(DOTABUFF_GUIDE_URL_SUFFIX);

char* sanitize_and_formulate_url(char* name) {
	char* url = calloc(150, sizeof(char));
	unsigned after_hero_name_index = PREFIX_LEN + strlen(name)-1; // name has newline
	unsigned skipped_characters = 0;

	strcpy(url, DOTABUFF_GUIDE_URL_PREFIX);

	printf("name is %s", name);

	for (int i=PREFIX_LEN;i<after_hero_name_index;i++) {
		if (name[i-PREFIX_LEN] == ' ') {
			url[i-skipped_characters] = '-';
		} else if (name[i-PREFIX_LEN] == '\'') {
			skipped_characters++;
		} else {
			url[i-skipped_characters] = tolower(name[i-PREFIX_LEN]);
		}
	}
	after_hero_name_index = after_hero_name_index - skipped_characters;
	
	strcpy((url+after_hero_name_index), DOTABUFF_GUIDE_URL_SUFFIX);

	return url;
}

int main() {
	FILE *fp_hero_list = fopen("hero_list.dat", "r");
	int hero_index = 0;
	
	char buffer[64];
	char commandstring[200];
	strcpy(commandstring, "wget ");

	while(fgets(buffer, 64, fp_hero_list) != NULL) {
		strcpy((commandstring+5), sanitize_and_formulate_url(buffer));
		system(commandstring);
		Sleep(3);
	}

	return EXIT_SUCCESS;
}
