#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dotabuff-prepend-process.h"
#include "dotabuff-prepend-parser.h"
#include "dotabuff-prepend-o.h"

#define MAX_ABILITY_SLOTS 23
#define MAX_TALENTS 8
#define MAIN_ABILITIES 4
#define MAX_ITEMS 128
// Invoker 7*3 + 4
#define MAX_SKILL_UPS 25
#define MAX_LEVEL_SKILL_UP 25
// max number of points investible in one ability
#define MAX_ABILITY_LEVEL 7

#define MAX_ROLE_TYPE 5
#define MAX_LANE_TYPE 5

#define DO_NOT_RUN_SINGLE 0

#define REMOVE_ADDITIONAL_ABILITIES

#ifdef VERBOSE
#define DEBUG
#endif

typedef struct hero_data {
	char 	heroReadable[64];
	char 	heroBuiltIn[64];
	int 	abilityBuild[MAX_SKILL_UPS];
	char* 	itemBuild[MAX_ITEMS];
	int		itemBuildLen;
	int		preferredLanes[MAX_LANE_TYPE];
	int		preferredRoles[MAX_ROLE_TYPE];	
	float	soloMultiplier;
	char*	abilityIndexToName[MAX_ABILITY_SLOTS]; 
	int		abilityIndexToNameLen;
} HERO_DATA;
