#include "dotabuff-prepend.h"

#define HERO_FILE_EXT ".lua"

#define LUA_END_PREPEND_DELIM "--@EndAutomatedHeroData"

// Will destroy old files

extern HERO_DATA CurrHeroData;

static char hero_file_name[256];

static char* prev_file_from_delim;

const static char* create_hero_file_name() {
	int len = strlen(CurrHeroData.heroBuiltIn);
	strcpy(hero_file_name, CurrHeroData.heroBuiltIn);
	strcpy(&(hero_file_name[len]), HERO_FILE_EXT);
	return hero_file_name;
}

int out_syntax_initialize() {
	return OUT_SUCCESS;
}

int prepend_hero_file() {
	FILE* hero_file;

	create_hero_file_name();

	if ( ( prev_file_from_delim = save_prev_after_delim(
				hero_file_name, LUA_END_PREPEND_DELIM ) )
			== NULL)
		return OUT_FAILED;
	if ((hero_file = fopen(hero_file_name, "w")) == NULL)
		return OUT_FAILED;
	/* TEMPORARY - HACK JOB ENDING FAST */
	// Hoping for array-to-print and function to print it recursion, but moving on..
	// Multiple file operations could be much slower than formulating a big string
	fprintf(hero_file, "local hero_data = {\n\t\"%s\",\n\t{", CurrHeroData.heroBuiltIn, CurrHeroData.abilityBuild[0]);
	int i=0;
	while(i<MAX_SKILL_UPS) {
		if (CurrHeroData.abilityBuild[i+1] == -1)
			break;
		fprintf(hero_file, "%d, ", CurrHeroData.abilityBuild[i++]+1);
	}
	fprintf(hero_file, "%d},\n\t{\n\t\t", CurrHeroData.abilityBuild[i]+1);
	i=0;
	while(i<CurrHeroData.itemBuildLen)
		fprintf(hero_file, "\"%s\",", CurrHeroData.itemBuild[i++]);
	fprintf(hero_file, "\n\t},\n\t{ {");
	i=0;
	while(i<MAX_LANE_TYPE) {
		if (CurrHeroData.preferredLanes[i] < 1)
			break;
		fprintf(hero_file, "%d,", CurrHeroData.preferredLanes[i++]);
	}
	fprintf(hero_file, "}, {");
	i=0;
	while(i<MAX_ROLE_TYPE) {
		if (CurrHeroData.preferredRoles[i] < 1)
			break;
		fprintf(hero_file, "%d,", CurrHeroData.preferredRoles[i++]);
	}
	fprintf(hero_file, "}, %.1f },\n\t{\n\t\t", CurrHeroData.soloMultiplier);
	i=0;
	while(i<CurrHeroData.abilityIndexToNameLen)
		fprintf(hero_file, "\"%s\",", CurrHeroData.abilityIndexToName[i++]);
	fprintf(hero_file, "\n\t}\n}\n");

	fprintf(hero_file, "%s", prev_file_from_delim);
	
	fclose(hero_file);
	return OUT_SUCCESS;
}

void free_out_syntax() { }
