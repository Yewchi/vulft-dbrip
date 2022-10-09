#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <windows.h>

#ifndef DBD
/* in order to prevent abuse of DotaBuff server, request at local if the user doesn't
 * know what they are doing */
#define DOMAIN "127.0.0.1"
#else
#define DOMAIN "www.dotabuff.com"
#endif

int main(int argc, char** argv) {
	if (argc > 1 && strcmp(argv[1], "-d") == 0) {
		printf("Doing stuff");
		system("wget -O dotabuff-heroes.dat https://"DOMAIN"/heroes");
		Sleep(5);
	}
	
	setvbuf(stdout, 0, _IONBF, 0);
	FILE* fp_dotabuff_heroes;
	char** hero_path_part;
	int buffer_size = 2*500*1000;

	int jump_forward_to_name = 24;

	char* buffer = (char*)malloc(buffer_size*sizeof(char));
	char* test_correct = (char*)malloc(512*sizeof(char));
	hero_path_part = (char**)calloc(200, sizeof(char**));

	fp_dotabuff_heroes = fopen("dotabuff-heroes.dat", "r+");

	unsigned short hero_path_size = 100;
	char* curr_char;

	printf("Parsing...\n");
	unsigned short hero_index = 0;
	while(fgets(buffer, buffer_size, fp_dotabuff_heroes) != NULL) {
		if ((curr_char = strstr(buffer, "div class=\"hero-grid\"")) != NULL) {
			printf("Found the hero key\n");
			while ((curr_char = strstr(buffer, "jpg)\"><div class=\"name\"")) != NULL) {
				curr_char = curr_char+jump_forward_to_name;
				buffer = curr_char;
				int i=0;
				hero_path_part[hero_index] = (char*)calloc(hero_path_size, sizeof(char));
				printf("alloc'd %dth hero\n", hero_index);
				while(curr_char[i] != '<') {
					hero_path_part[hero_index][i] = curr_char[i];
					i++;
					if(i > 1000) { break; }
				}
				printf("Hero is named '%s'\n", hero_path_part[hero_index]);
				hero_path_part[hero_index][i++] = '\n';
				hero_path_part[hero_index++][i] = '\0';
			}
			break;
		}
		printf("%s", buffer);
	}
	printf("Names found, storing in 'hero_list.dat'.\n");
	
	FILE* fp_hero_list = fopen("hero_list.dat", "w+");

	hero_index = 0;
	while(hero_path_part[hero_index] != NULL) {
		fwrite(hero_path_part[hero_index], strlen(hero_path_part[hero_index]), sizeof(char), fp_hero_list);	
		free(hero_path_part[hero_index++]);	
	}

	fclose(fp_hero_list);
	fclose(fp_dotabuff_heroes);

	free(hero_path_part);
	free(buffer);
	free(test_correct);

	return EXIT_SUCCESS;
}
