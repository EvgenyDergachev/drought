/*
 * drawt.c
 * 
 * Copyright 2013 Evgeny Dergachev, ead1@gmx.us
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 * 
 */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <termios.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>

#define INVENTORY 20
#define E_INVENTORY 3

#define BG_BLACK "\33[40m"
#define RED "\33[0;31;40m"
#define B_RED "\33[1;31;40m"
#define GREEN "\33[0;32;40m"
#define B_GREEN "\33[1;32;40m"
#define YELLOW "\33[0;33;40m"
#define B_YELLOW "\33[1;33;40m"
#define BLUE "\33[0;34;40m"
#define B_BLUE "\33[1;34;40m"
#define MAGENTA "\33[0;35;40m"
#define B_MAGENTA "\33[1;35;40m"
#define CYAN "\33[0;36;40m"
#define B_CYAN "\33[1;36;40m"
#define WHITE "\33[0;97;40m"
#define B_WHITE "\33[1;97;40m"
#define RESET "\33[0m"
#define SAVE_CURSOR "\033[s"
#define RESTORE_CURSOR "\033[u"

typedef struct
{
	char* name;
	char* descr;
	char* sig; //which letter shows it
	char* ego; //flame, frost, spark, venom
	int egoDmg;
	int weight;
	int damage;
	int bonusDmg;
	int type; //0-99 - weapon, 100-199 - potions, 200-299 - scrolls, 300 - corpse
}
item;
typedef struct node
{
		item Item;
		struct node* next;
} node; //for linked list of Room[current] items
typedef struct
{
	int maxhp, minDmg, maxDmg; //characteristics
	char* descr; char* name; char* gender; char* effect; char* effect2;
	char* sig; //which letter shows it
	int weaponState, exp, id;
}
sobject; //for lib arrays
typedef struct
{
	int i, j; //coordinates
	int hp, maxhp, isAlive, weaponState, maxDmg, minDmg; //characteristics
	item inventory[INVENTORY];//inventory[0] - weapon
	char* descr; char* name; char* gender; char* effect; char* effect2;
	char* sig; //which letter shows it
	char* previousDot; //not to wipe things while moving
	int capacity; //available load
	int id, exp; //id and exp worth/accumulated exp
	int condition[2], duration[2]; //effects and their duration, in this order: poisoned, empowered
}
object;
typedef struct
{
	int DIMENSION_I, DIMENSION_J; // max 18, 76
	char envir[200], prompt[200], messg[200], user_messg[200], drop_screen[5500];
	object enemy[50];
	object trap[100];
	node* drop[100][100];
	int numTraps, numEnemy, numAliveEnemy;
	char* descr;
	char* grid[100][100];
	int keys[6][3]; //right + coord, left, down, up, dlvl, ulvl
}
room;
typedef struct
{
	int i, j , ii, jj;
}
wall;
struct
{
	char* log[20];
	int index, messg_count, cnt, thisTurnInd[10];
}
Log;

//prototypes
void draw(void);
int getInput(void);
int getInputInv(char* drop_screen, int range, int queue);
void getVoidInput(void);
int moveTrue(int i, int j, object enemy[], int numEnemy);
object moveEnemy(object user, object enemy);
object attack(object victim, object offender);
void gameOver(char* messg);
void clrscr(void);
int moveEnemyTrue (int i, int j);
void iniWall(int n);
void iniExits(int n);
object iniTrap(int n);
object iniMeleeEnemy(int n);
object iniWeaponlessEnemy(int n);
void iniItem(int n);
void iniHero(void);
int checkEnemy(int i, int j, object enemy[], int numEnemy);
void simpleMoves(int user_move);
void trapChecking(void);
void enemyAction(int k, int user_move);
void envir(void);
void plog(char* toLog);
int getEnter(void);
wall division(int dimension_1, int dimension_2, int kind, int lowbound_1, int lowbound_2, int n);
void dropItem(int i, int j, item dropped_item, int n);
void itemMoves(int user_move);
item egoGeneration(item item);
void egoMessg(int i);
void renameAll(int s);
item renameCheck(item Item, int s);
void scan(char cc[2500]);
object loadObject(char* cc);
void ciph(char* a);
char* deciph(char* c);
void parse(char* cc, int j, char temp[250]);
item loadItem(char* cc);
void save(void);
void load(void);
void freeing(void);
void iniKeys(void);
void iniRoom(int n);
void doorStep(void);
object iniDragon(void);
void enemyDeath(int k, char* messg);
void prompt(void);
void hittingEnemy(int k);
int getYesNo(void);
void levelUp(void);
void conditionCheck(void);
int differentMoves(int user_move);
void placeRelatedMsg(void);

//global variables, less error-prone than passing pointers to them to 2/3 of all functions
room Room[49];
object user;
int current;
FILE* f;
item zero_item = {"", "empty", "", " ", 0, 0, 0, 0, 0};
item simple_weapon[] = 
{{"knarry club", "A knarry club. It even has some bark remained on it.", WHITE"["RESET, " ", 0, 50, 1, 0, 0},//tier 1 weapon
{"rusty scimitar", "A rusty scimitar. It wasn't cleaned like forever.", WHITE"["RESET, " ", 0, 40, 1, 0, 1},
{"crude sword", "A crude sword. Badly forged, it's nonetheless deadly.", WHITE"["RESET, " ", 0, 40, 1, 0, 2},
{"curved spear", "A curved spear. Two fastened pieces of wood and a sharp tip.", WHITE"["RESET, " ", 0, 70, 2, 0, 3},
{"limberjack axe", "Some limberjack forgot his axe here.", WHITE"["RESET, " ", 0, 50, 2, 0, 4},
{"wooden staff", "A wooden staff. Favorite weapon of monks.", WHITE"["RESET, " ", 0, 60, 1, 0, 5},
{"knobstick", "A knobstick. Favorite weapon of thieves.", WHITE"["RESET, " ", 0, 40, 1, 0, 6},
{"butcher knife", "A butcher kinfe. Remember that cows don't usually resist.", WHITE"["RESET, " ", 0, 30, 1, 0, 7},
{"hefty spear", "A huge, but crude spear with an ashen shaft.", WHITE"["RESET, " ", 0, 80, 4, 0, 8},//tier 2 weapon
{"simple mace", "A simple mace. Stock weapon of human infantry.", WHITE"["RESET, " ", 0, 60, 3, 0, 9},
{"short sword", "A short sword. Stock weapon of human infantry.", WHITE"["RESET, " ", 0, 40, 3, 0, 10},
{"goblin scimitar", "A goblin scimitar. Crescent blade of black steel.", WHITE"["RESET, " ", 0, 40, 3, 0, 11},
{"battle axe", "A battle axe. Stock weapon of human infantry.", WHITE"["RESET, " ", 0, 60, 3, 0, 12},
{"torch", "A blazing oil torch.", WHITE"["RESET, "flame", 2, 40, 3, 0, 13},       //always flame ego
{"spiked chain", "A spiked chain. Favorite weapon of orcish youth.", WHITE"["RESET, " ", 0, 50, 4, 0, 14},
{"dagger", "Just a generic dagger.", WHITE"["RESET, " ", 0, 30, 3, 0, 15},
{"lance", "A well-made long spear with a razor sharp tip.", WHITE"["RESET, " ", 0, 80, 7, 0, 16},//tier 3 weapon
{"giant mace", "A giant mace. Huge spiked ball on a long curved shaft.", WHITE"["RESET, " ", 0, 80, 7, 0, 17},
{"morning star", "A morning star. Spiked ball on a long chain.", WHITE"["RESET, " ", 0, 70, 6, 0, 18},
{"long sword", "A long straight double-edged knightly sword.", WHITE"["RESET, " ", 0, 70, 6, 0, 19},
{"flamberge", "A long wave-bladed two-handed sword.", WHITE"["RESET, " ", 0, 70, 6, 0, 20},
{"dwarven axe", "A dwarven axe. Favorite weapon of dwarves.", WHITE"["RESET, " ", 0, 60, 6, 0, 21},
{"demon trident", "They say, demons use them to torture sinners.", WHITE"["RESET, " ", 0, 70, 6, 0, 22},
{"kindjal", "A long dagger, made of damask steel.", WHITE"["RESET, " ", 0, 30, 6, 0, 23}};
item consumables[] =
{{"potion", "A glass vial with some liquid.", WHITE"!"RESET, " ", 0, 5, 0, 0, 100}, //healing potion 
{"scroll", "A scroll of parchment with a magical spell written on it.", WHITE"?"RESET, " ", 0, 5, 0, 0, 200}, //scroll of teleportation
{"potion", "A glass vial with some liquid.", WHITE"!"RESET, " ", 0, 5, 0, 0, 101}, //potion of poison
{"potion", "A glass vial with some liquid.", WHITE"!"RESET, " ", 0, 5, 0, 0, 102}, //useless potion
{"potion", "A glass vial with some liquid.", WHITE"!"RESET, " ", 0, 5, 0, 0, 103}, //potion of exp
{"potion", "A glass vial with some liquid.", WHITE"!"RESET, " ", 0, 5, 0, 0, 104}, //potion of power
{"scroll", "A scroll of parchment with a magical spell written on it.", WHITE"?"RESET, " ", 0, 5, 0, 0, 201} //useless scroll
};
sobject simple_melee[] = 
{{5, 0, 2, "A hideous goblin is heading to you! He wields", "goblin", "him", "", "", B_GREEN"g"RESET, 1, 10, 0},//tier 1, weapon tier 1
{4, 1, 3, "An ugly kobold looks angry. She wields", "kobold", "her", "", "", B_GREEN"k"RESET, 1, 10, 1},
{5, 0, 2, "A skeleton, animated with black magic. It wields", "skeleton", "it", "", "", WHITE"z"RESET,1, 10, 2},
{4, 1, 3, "A gnome hates those who are taller. He wields", "gnome", "him", "", "", YELLOW"d"RESET, 1, 10, 3},
{4, 2, 4, "A clockwork spiderling is sliding to you!", "clockwork spiderling", "it", "steel mandibulas", "", B_MAGENTA"c"RESET, 0, 10, 4},//tier 1, no weapon
{5, 2, 4, "A cave boar doesn't like your appearance.", "cave boar", "it", "long tusks", "", CYAN"p"RESET, 0, 10, 5},
{4, 1, 3, "A snake coils here.", "snake", "it", "teeth", "", GREEN"s"RESET, 0, 10, 6},
{5, 1, 3, "A wild dog barks at you.", "wild dog", "it", "teeth", "", B_RED"w"RESET, 0, 10, 7},
{7, 2, 4, "An orc wants to smash your head! He wields", "orc", "him", "", "", B_RED"o"RESET, 1, 15, 8},//tier 2, weapon tier 1
{6, 1, 3, "This caveman dislike trespassers. He wields", "cave man", "him", "", "", WHITE"h"RESET, 1, 15, 9},                      //knarry club
{6, 1, 3, "A half-rotten zombie wants your brains! It wields", "zombie", "it", "", "", GREEN"z"RESET, 1, 15, 10},
{7, 2, 4, "A sturdy dwarf muck about here. He wields", "dwarf", "him", "", "", B_MAGENTA"d"RESET, 1, 15, 11},
{7, 2, 4, "A clockwork sheep reels here.", "clockwork sheep", "it", "steel horns", "", B_GREEN"c"RESET, 0, 15, 12},//tier 2, no weapon
{6, 3, 5, "A leopard roams in shadows.", "leopard", "it", "claws", "", YELLOW"l"RESET, 0, 15, 13},
{7, 2, 4, "A giant snake is sliding to you!", "giant snake", "it", "teeth", "", B_GREEN"s"RESET, 0, 15, 14},
{6, 3, 5, "A snow-white owl rushes to you!", "cave owl", "it", "talons", "", B_CYAN"b"RESET, 0, 15, 15},
{10, 1, 3, "A big scarred goblin stays here. He wields", "goblin warrior", "him", "", "", GREEN"g"RESET, 1, 22, 16},//tier 3, weapon tier 2  //goblin scimitar
{9, 2, 4, "A kobold warrior looks at you. He wields", "kobold warrior", "him", "", "", GREEN"k"RESET, 1, 22, 17},
{10, 2, 4, "A girl in black bares her fangs. She wields", "vampire", "her", "", "", MAGENTA"v"RESET, 1, 22, 18},
{9, 1, 3, "A half-mad gnome giggles here. He wields", "gnome arsonist", "him", "", "", B_RED"d"RESET, 1, 22, 19},                            //torch
{9, 5, 7, "A clockwork panther sneaks to you.", "clockwork panther", "it", "steel claws", "", YELLOW"c"RESET, 0, 22, 20},//tier 3, no weapon
{10, 5, 7, "A huge brown bear looks dopey.", "cave bear", "it", "claws", "", CYAN"B"RESET, 0, 22, 21},
{9, 4, 6, "A half-sapient mollusc brandishes its shell.", "mollusc fighter", "it", "sharp shell", "", MAGENTA"m"RESET, 0, 22, 22},
{10, 4, 6, "A spiky crab move sideways.", "spiky crab", "it", "pincers", "", B_GREEN"m"RESET, 0, 22, 23},
{12, 1, 3, "A stout orc. He wields", "orc warrior", "her", "", "", B_YELLOW"o"RESET, 1, 33, 24},//tier 4, weapon tier 3
{13, 2, 4, "A huge undead made of meat. It wields", "meat giant", "it", "", "", B_RED"z"RESET, 1, 33, 25},
{13, 2, 4, "An ogre pick his teeth here. He wields", "ogre", "him", "", "", B_CYAN"O"RESET, 1, 33, 26},                         //giant mace or morining star
{12, 1, 3, "An ironclad dwarf. He wields", "dwarf axeman", "him", "", "", B_WHITE"d"RESET, 1, 33, 27},                              //dwarven axe
{13, 7, 9, "A clockwork horse rears!", "clockwork horse", "it", "steel hooves", "", CYAN"c"RESET, 0, 33, 28},//tier 4, no weapon
{13, 8, 10, "A black silhouette of horse with no body.", "nightmare", "her", "ethereal hooves", "", B_BLUE"n"RESET, 0, 33, 29},
{12, 8, 10, "A giant lion lashes its tail.", "cave lion", "it", "claws", "", B_MAGENTA"l"RESET, 0, 33, 30},
{12, 7, 9, "A foul red worm squirms here.", "giant worm", "it", "digestive acid", "", B_CYAN"m"RESET, 0, 33, 31},
{50, 9, 11, "A huge cobalt dragon wants to eat you.", "cobalt dragon", "her", "claws", "", B_BLUE"D"RESET, 0, 99, 32}}; //boss
sobject different_traps[] = 
{{1, 1, 3, "", "acid pit", "it", "have fallen into an acid pit. Acid burns", "shrieks while an acid pit dissolves", YELLOW"^"RESET, 0, 0, 0},
{1, 0, 0, "", "teleportation pad", "it", "You have stepped on a teleportation pad. You blink.", "vanishes and appears in a different place.", CYAN"^"RESET, 0, 0, 1},
{1, 0, 0, "", "venom pond", "it", "You have stepped into a venom pond. You are poisoned!", "is poisoned by a venom pond.", B_GREEN"^"RESET, 0, 0, 1}};

int main ()
{
	//initialization of the game board, (do not use ~ and ` in the in-game text, it will mess with saving)
	srand(time(NULL));
	for (int i = 0; i < 100; i++) //windows cmd always have black background so this for linux only?
		printf(BG_BLACK"\n");
	int cc =  1329; //pic was drawn by me and made to array with a special script, font is one of standard figlet's
	char ascii[] = {' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ','^','.',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ','_','.','-','\'','\'','_','_','.','\'','\n',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ','_','_','_','_','.','-','\'',' ','.','\\',' ',' ',' ',' ',' ',' ',' ',' ',' ',',','\'','\'','\'','\n',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ','`','.',',',' ',' ',' ',' ',' ',' ',' ','\'',' ',' ',' ',' ','_',',','-','`','\n',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ','.','-','|','.','\'','\'','\\','\\',' ','\'','|','`','|','\n',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ','|','.',' ',' ',' ',' ',' ','.','\'',' ',' ',',','\n',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ','\'',' ','`','\'','-','-',' ',' ',' ',',','\n',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',',','\'','/',' ',' ','\'','.',' ','/',' ',' ',' ',' ',' ','_','_','_','_','_','_',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ','_',' ',' ',' ',' ',' ','_','\n',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ','_',',','\'',' ','\'',' ',' ',',',' ',',',' ','|',' ',' ',' ',' ',' ','|',' ',' ','_',' ',' ','\\',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ','|',' ','|',' ',' ',' ','|',' ','|','\n',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',',','\'',' ',' ',' ',' ',' ','\'',' ','|',' ',',',' ','|',' ',' ',' ',' ',' ','|',' ','|',' ','|',' ','|','_',' ','_','_',' ','_','_','_',' ',' ','_',' ',' ',' ','_',' ',' ','_','_',' ','_','|',' ','|','_','_',' ','|',' ','|','_','\n',' ',' ',' ',' ',' ',' ','/','\\',' ',' ',' ',' ',' ',' ',' ',' ',' ',',','\'',' ',' ',' ',' ',' ',' ',' ','|',' ','\'',' ',',',' ','\'',' ',' ',' ',' ',' ','|',' ','|',' ','|',' ','|',' ','\'','_','_','/',' ','_',' ','\\','|',' ','|',' ','|',' ','|','/',' ','_','`',' ','|',' ','\'','_',' ','\\','|',' ','_','_','|','\n',' ',' ','\\','"','\'','/',' ',' ','\\','\'','"','/',' ',' ',' ',',','\'',' ',' ',' ',' ',' ',' ',' ',' ',' ','\'',' ','.',' ','-','\'','\'',' ','\\',' ',' ',' ','|',' ','|','/',' ','/','|',' ','|',' ','|',' ','(','_',')',' ','|',' ','|','_','|',' ','|',' ','(','_','|',' ','|',' ','|',' ','|',' ','|',' ','|','_','\n','\\','"','\'','\\',' ',' ',' ',' ',' ',' ','/','\'','"','/','\'',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ','/','.',' ','|',' ',' ',' ',' ',' ',' ','\'',' ',' ','|','_','_','_','/',' ','|','_','|',' ',' ','\\','_','_','_','/',' ','\\','_','_',',','_','|','\\','_','_',',',' ','|','_','|',' ','|','_','|','\\','_','_','|','\n',' ','\\',' ',' ','_',' ','|',' ','_','_',' ',' ','/',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ','\'','.','|',' ','|',' ',' ',' ',' ','_',',','\'','.',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ','_','_','/',' ','|','\n',' ','/','\'',' ',' ','/',' ','\\',' ',' ',' ','\'','\\',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',',','\'',' ',',','=','.','-','-','\'','\'',' ','.',' ',' ','\'','.',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ','|','_','_','_','/','\n','/','_',' ','/',' ',' ',' ',' ',' ',' ','\\',' ','_','\\',' ',' ',' ',' ',' ',' ',' ',' ','/',' ',' ',',','\'','-','\'',' ',' ',',',' ','\'',' ','\'','.',' ',' ','\\','\n',' ',' ','/','_','.','\\',' ',' ','/','.','_','\\',' ',' ',' ',' ',' ',' ',' ',' ',' ','\'',' ',' ','/',' ',' ',' ',' ',' ',' ','|',' ',' ','\\',' ',' ','\\',' ',' ','\'',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ','v','.',' ','0','.','1',' ','a','l','p','h','a','\n',' ',' ',' ',' ',' ',' ','\\','/',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',',','\'',' ',',','\'',' ',' ',' ',' ',' ',' ',' ','\'',' ',' ',' ',',',' ',' ','\'','.',' ','\'','.','\n',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ','/','=',':','\'',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ','\'',' ',' ','|',' ',' ',' ',' ','\'',':','=','\\','\n',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ','/',' ','/',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ','\'','.','\'','_',',',' ',' ',' ',' ','\\',' ','\\','\n',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ','/',' ','\'',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ','\'',' ','\\','\n',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',',',' ','/',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ','\\',' ',',',' ',' ',' ','P','r','e','s','s',' ','a','n','y',' ','k','e','y',' ','t','o',' ','c','o','n','t','i','n','u','e','\n',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',',','-','>',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ','<','-','.','\n',' ',' ',' ','_',',','=','.','-','-','\'',':','\'','\'',' ',' ','\'','_','_','|',' ',' ','.','.','=','\'','\'',':',',','=','=','=','=','\'','=','=','.','.','\'','-','\'','.','.',' ','|','_','_','\'',' ',' ','\'','\'','.',',','_','_','.'};
	for(int i =0; i < cc; i++)
		printf(WHITE"%c", ascii[i]);
	printf("\n");
	getVoidInput();
	for (int i = 0; i < 20; i++)
		Log.log[i] = " ";
	Log.index = -1;
	for (int k = 0; k < 49; k++)
	{
		for (int i = 0; i < 100; i++)
			for (int j = 0; j < 100; j++)					
				Room[k].drop[i][j] = NULL;
		sprintf(Room[k].messg, "%s", " "); sprintf(Room[k].user_messg, "%s", " "); sprintf(Room[k].drop_screen, "%s", " ");
	}
	if ((f = fopen("save", "r")) == NULL)
	{
		current = 0;
		//randomizing items
		char* colors[] = {"white","black","red","sparkling","yellow","green","blue","beige","violet","purple","muddy"}; 
		char line[100]; //adding random colors to potions
		int ad_ind[5]; 
		ad_ind[0] = rand()%11;
		sprintf(line, "%s potion", colors[ad_ind[0]]);
		consumables[0].name = malloc((strlen(colors[ad_ind[0]])+8)*sizeof(char));
		sprintf(consumables[0].name, "%s", line);
		while (1)
		{
			ad_ind[1] = rand()%11;
			if (ad_ind[1] != ad_ind[0])
				break;
		}
		sprintf(line, "%s potion", colors[ad_ind[1]]);
		consumables[2].name = malloc((strlen(colors[ad_ind[1]])+8)*sizeof(char));
		sprintf(consumables[2].name, "%s", line);
		while (1)
		{
			ad_ind[2] = rand()%11;
			if (ad_ind[2] != ad_ind[0] && ad_ind[2] != ad_ind[1])
				break;
		}
		sprintf(line, "%s potion", colors[ad_ind[2]]);
		consumables[3].name = malloc((strlen(colors[ad_ind[2]])+8)*sizeof(char));
		sprintf(consumables[3].name, "%s", line);
		while (1)
		{
			ad_ind[3] = rand()%11;
			if (ad_ind[3] != ad_ind[0] && ad_ind[3] != ad_ind[1] && ad_ind[3] != ad_ind[2])
				break;
		}
		sprintf(line, "%s potion", colors[ad_ind[3]]);
		consumables[4].name = malloc((strlen(colors[ad_ind[3]])+8)*sizeof(char));
		sprintf(consumables[4].name, "%s", line);
		while (1)
		{
			ad_ind[4] = rand()%11;
			if (ad_ind[4] != ad_ind[0] && ad_ind[4] != ad_ind[1] && ad_ind[4] != ad_ind[2] && ad_ind[4] != ad_ind[3])
				break;
		}
		sprintf(line, "%s potion", colors[ad_ind[4]]);
		consumables[5].name = malloc((strlen(colors[ad_ind[4]])+8)*sizeof(char));
		sprintf(consumables[5].name, "%s", line);		
		char vovels[] = {'A','E','I','O','U','Y'};
		char consonants[] = {'B','C','D','F','G','H','J','K','L','M','N','P','Q','R','S','T','V','W','X','Z'};
		char spell[11]; //adding random title to scrolls
		int len = rand()%6+ 5;
		for (int i = 0; i < len; i++)
			if (rand()%2 == 0)
				spell[i] = vovels[rand()%6];
			else
				spell[i] = consonants[rand()%20];
		spell[len] = '\0';
		sprintf(line, "scroll named %s", spell);
		consumables[1].name = malloc((strlen(spell)+14)*sizeof(char));
		sprintf(consumables[1].name, "%s", line);
		len = rand()%6+ 5;
		for (int i = 0; i < len; i++)
			if (rand()%2 == 0)
				spell[i] = vovels[rand()%6];
			else
				spell[i] = consonants[rand()%20];
		spell[len] = '\0';
		sprintf(line, "scroll named %s", spell);
		consumables[6].name = malloc((strlen(spell)+14)*sizeof(char));
		sprintf(consumables[6].name, "%s", line);
		iniKeys();
		for (int i = 0; i < 49; i++)
			iniRoom(i);
		iniHero(); //hero
		user.i = Room[0].keys[5][1]; user.j = Room[0].keys[5][2];
		Room[0].grid[user.i][user.j] = user.sig;
		sprintf(Room[0].user_messg, "%s", "There is an exit from the dungeon here."); //user_messg since log makes using messg impossible at the 1st turn
		clrscr();
		printf("A terrible dragon devastates the neighborhood. Many adventures seeked the\n"); 
		printf("glory and riches trying to kill him, but all either died or fled in terror\n"); 
		printf("from the foul goblin-infested dungeon, where he made a lair. Will you share\n");
		printf("the same fate or you will succeed and slay the monster?\n\n");
		printf("Movement:\n");
		printf("Arrows, num pad or vi keys.\n");
		printf(" 7 8 9               y k u               > - move downstairs\n");
		printf("  \\|/                 \\|/                < - move upstairs\n");
		printf(" 4-5-6 - num pad     h-.-l - vi keys     . or 5 - pass a turn\n");  
		printf("  /|\\                 /|\\\n");
		printf(" 1 2 3               b j n\n\n");
		printf("Commands:\n");
		printf(" g - pick item       i - show inventory & equipment\n");
		printf(" d - drop item       p - show log of recent messages\n");
		printf(" w - wield weapon    s - save\n");
		printf(" r - read scroll     S - save&quit\n");
		printf(" q - quaff potion    Q - quit the game (commit suicide)\n\n");
		printf("Beware for death is permanent and only potions can restore your health!\n"); 
		printf("Press any key to continue.\n");
		save();
		getVoidInput();
	}
	else
	{
		load();
		sprintf(Room[current].user_messg, "Welcome back!");
	}
	envir();
	prompt();
	clrscr();
	draw();
	while (1) //the main game loop
	{
		sprintf(Room[current].messg, "%s", " "); 
		sprintf(Room[current].user_messg, "%s", " "); 
		sprintf(Room[current].drop_screen, "%s", " ");
		Log.messg_count = 0; Log.cnt = 0;
		for (int i = 0; i < 10; i++)
			Log.thisTurnInd[i] = 0;
		int user_move = getInput();
		simpleMoves(user_move);//user movement
		if (user.isAlive == 0)
			return 0;	
		trapChecking();//user vs traps
		if (user.isAlive == 0) 
			return 0;
		for (int k = 0; k < Room[current].numEnemy; k++)//enemy loop
		{
			enemyAction(k, user_move);
			if (user.isAlive == 0) 
				return 0;
		}
		itemMoves(user_move);//item interactions
		if (differentMoves(user_move) == 0)
			return 0;
		placeRelatedMsg();
		conditionCheck();
		if (user.isAlive == 0) 
			return 0;
		levelUp();
		prompt();
		envir();
		if (strcmp(Room[current].user_messg, "Wrong command.") == 0) //to avoid double error message
			sprintf(Room[current].user_messg, "%s", " ");
		draw();
	}
    return 0;
}

void draw()
{
	printf("\033[%d;%dH", 0, 0);
	printf("\n"); //a bit of space at top
	for (int i = 0; i < Room[current].DIMENSION_I; i++)
	{
		printf("  ");//to add "  " for left border
		for (int j = 0; j < Room[current].DIMENSION_J;j++)
			printf("%s"BG_BLACK"", Room[current].grid[i][j]);//to add " " for distance between dots
		printf("\n");
	}
	if (strcmp(Room[current].user_messg, " ") == 0)//to keep prompt on the same lvl and to wipe old strings
	{
		printf(BG_BLACK"                                                                          ");
		printf("\n");
	}
	if (strcmp(Room[current].messg, " ") == 0)//same
	{
		printf(BG_BLACK"                                                                          ");
		printf("\n");
	}
	printf(BG_BLACK"                                                                          ");
	printf("\r%s\n", Room[current].envir);
	if (strcmp(Room[current].user_messg, " ") != 0)
	{
		printf(BG_BLACK"                                                                          ");
		printf("\r%s\n", Room[current].user_messg);
	}
	if (strcmp(Room[current].messg, " ") != 0)
	{
		if (Log.messg_count == 1)
		{
			printf(BG_BLACK"                                                                          ");
			printf("\r%s\n", Room[current].messg);
		}
		else if (Log.messg_count > 1)
		{
			printf(SAVE_CURSOR BG_BLACK"                                                                          ");
			while(Log.messg_count !=1)
			{
				printf(RESTORE_CURSOR BG_BLACK"                                                                          ");
				printf(RESTORE_CURSOR"%s (Enter)", Log.log[Log.thisTurnInd[Log.cnt]]);
				int user_pick = 0;
				do
				{
					user_pick = getEnter();
				}
				while (user_pick !=1);
				Log.messg_count--; Log.cnt++;
			}
			printf(RESTORE_CURSOR BG_BLACK"                                                                          ");
			printf(RESTORE_CURSOR"%s\n", Log.log[Log.thisTurnInd[Log.cnt]]);
		}
	}
	printf(BG_BLACK"                                                                          ");
	printf("\r%s\n", Room[current].prompt);
}

void clrscr()
{
    printf("\33[2J");
	printf("\33[%d;%dH", 0, 0);
}

int getInput()
{
	while (1)//Esc 27, arrows: up 1065 |down 1066 | left 1068 | right 1067
	{//upper left 1055 | upper right 1053 | lower left 1056 | lower right 1054, in cmd.exe and on ubuntu lower left 1052 | upper left 1049
		struct termios newt, oldt;
		tcgetattr(STDIN_FILENO, &oldt);
		newt = oldt;
		newt.c_lflag &= ~(ICANON | ECHO);
		tcsetattr(STDIN_FILENO, TCSANOW, &newt); //enable raw mode
		int ch = 0, input = 0;
		ch = getchar();
		if (ch == 126)
			ch = getchar();
		if (ch == 27)
			ch = getchar();
		if (ch == '[')
			input = getchar();
		tcsetattr(STDIN_FILENO, TCSANOW, &oldt); //return old config
		if (input == 55 || input == 53 || input == 56 || input == 54 || input == 65 || input == 68 || input == 66 
		|| input == 67 || input == 52 || input == 49 || ch == 49 || ch == 50 || ch == 51 || ch == 52 || ch == 53 || ch == 54 || ch == 55 || ch == 56 
		|| ch == 57 || ch == 'y' || ch == 'k' || ch == 'u' || ch == 'h' || ch == 'l' || ch == 'b' || ch == 'j' 
		|| ch == 'n' || ch == '<' || ch == 'w' || ch == 'g' || ch == 'd' || ch == 'Q' || ch == 'p' 
		|| ch == '.' || ch == 'r' || ch == 'q' || ch == 'i' || ch == 'S' || ch == '>' || ch == 's')
		{
			if (input == 55 || input == 53 || input == 56 || input == 54 || input == 65 || input == 68 
			|| input == 66 || input == 67 || input == 52 || input == 49)
				return input + 1000; //not to mistake for an ordinary button
			else
				return ch;
		}
		sprintf(Room[current].user_messg, "Wrong command.");
		plog(Room[current].user_messg);
		draw();
	}
	return 1;
}

int getInputInv(char* drop_screen, int range, int queue)
{
	while (1)
	{
		struct termios newt, oldt;
		tcgetattr(STDIN_FILENO, &oldt);
		newt = oldt;
		newt.c_lflag &= ~(ICANON | ECHO);
		tcsetattr(STDIN_FILENO, TCSANOW, &newt); //enable raw mode	
		int input, ch; //ch is set but not used intentionally
		input = getchar();
		if (input == 126)
			input = getchar();
		if (input == 27)
			input = getchar();
		if (input == '[')
			ch = getchar();	
		tcsetattr(STDIN_FILENO, TCSANOW, &oldt); //return old config
		if ((input >= 65 && input <= 90) || (input >=97 && input <= 122) || input == '0' || (input == '-' && queue == 1))
		{
			if (islower(input))
			{
				input = toupper(input);
				range = toupper(range);
			}
			else
			{
				input = tolower(input);
				range = tolower(range);
			}
			if (input > range)
			{
				printf("\33[%d;%dH", 0, 0);
				printf(WHITE"%s", drop_screen);
				printf(BG_BLACK"                                                                          ");
				printf("\r"B_RED"You don't see such item.");
				continue;
			}
			else
			{
				clrscr();
				if (islower(input))
					input = toupper(input);
				else
					input = tolower(input);
				return input;
			}
		}
		printf("\33[%d;%dH", 0, 0);
		printf(WHITE"%s", drop_screen);
		printf(BG_BLACK"                                                                          ");
		printf("\r"B_RED"Wrong command.");
	}
	return 1;
}

int moveTrue(int i, int j, object enemy[], int numEnemy)
{
	if (i < 0 || j < 0 || i >= Room[current].DIMENSION_I || j >= Room[current].DIMENSION_J)
		return 3;
	else if (strcmp(Room[current].grid[i][j], YELLOW"#"RESET) == 0)
		return 2;
	for (int k = 0; k < numEnemy; k++)
		if (enemy[k].isAlive == 1 && enemy[k].i == i && enemy[k].j == j)
		return 1;
	return 0;
}

object iniTrap(int n)
{
	object trap;
	int roll = rand()%3;
	int lvlBonus = n/7;
	trap.maxDmg = different_traps[roll].maxDmg + lvlBonus; trap.minDmg = different_traps[roll].minDmg + lvlBonus;
	trap.name = different_traps[roll].name; trap.effect = different_traps[roll].effect;
	trap.effect2 = different_traps[roll].effect2; trap.sig = different_traps[roll].sig;
	while (1)
	{
		trap.i = 1 + rand()%(Room[n].DIMENSION_I - 2);
		trap.j = 1 + rand()%(Room[n].DIMENSION_J - 2);
		if (strcmp(Room[n].grid[trap.i][trap.j], YELLOW"."RESET) == 0)
		{
			Room[n].grid[trap.i][trap.j] = trap.sig; //to be able to check, they will be hidden later
			return trap;
		}
	}
}

object iniMeleeEnemy(int n)
{
	object enemy;
	int roll;
	if (n <= 6) roll = rand()%4; //1st floor, tier 1
	else if (n >= 14 && n <= 20) roll = 8 + rand()%4; //3rd floor, tier 2
	else if (n >= 28 && n <= 34) roll = 16 + rand()%4; //5th floor, tier 3
	else if (n >= 42) roll = 24 + rand()%4; //7th floor, tier 4
	if (n >=7 && n <= 13) //2nd floor, 75% tier 1, 25% tier 2
	{
		if (rand()%4 == 0) roll = 8 + rand()%4;
		else roll = rand()%4;
	}
	if (n >=21 && n <= 27) //4th floor, 50% tier 2, 50% tier 3
	{
		if (rand()%2 == 0) roll = 8 + rand()%4;
		else roll = 16 + rand()%4;
	}	
	if (n >=35 && n <= 41) //6th floor, 25% tier 3, 75% tier 4
	{
		if (rand()%4 == 0) roll = 16 + rand()%4;
		else roll = 24 + rand()%4;
	} 
	enemy.weaponState = 1; //it has weapon, to change for possibility to generate bare-handed ones
	enemy.hp = enemy.maxhp = simple_melee[roll].maxhp; enemy.isAlive = 1;
	if (simple_melee[roll].id == 9) enemy.inventory[0] = egoGeneration(simple_weapon[0]); //club for caveman
	else if (simple_melee[roll].id == 16) enemy.inventory[0] = egoGeneration(simple_weapon[11]); //scimitar for gobl. warrior
	else if (simple_melee[roll].id == 19) enemy.inventory[0] = egoGeneration(simple_weapon[13]); //torch for arsonist
	else if (simple_melee[roll].id == 26) enemy.inventory[0] = egoGeneration(simple_weapon[17 + rand()%2]); //maces for ogre
	else if (simple_melee[roll].id == 27) enemy.inventory[0] = egoGeneration(simple_weapon[21]); //axe for dwarf warrior
	else if (simple_melee[roll].id <= 11) enemy.inventory[0] = egoGeneration(simple_weapon[rand()%8]); //tier 1 and 2 mobs, tier 1 weapon
	else if (simple_melee[roll].id >= 16 && simple_melee[roll].id <= 19) enemy.inventory[0] = egoGeneration(simple_weapon[8 + rand()%8]); //tier 3 mobs, tier 2 weapon
	else if (simple_melee[roll].id >= 24 && simple_melee[roll].id <= 27) enemy.inventory[0] = egoGeneration(simple_weapon[16 + rand()%8]); //tier 4 mobs, tier 3 weapon	
	if (rand()%5 < 2) //0.4 chance for healing potion
		enemy.inventory[1] = consumables[0];
	else if (rand()%5 == 1) //(1-0.4)*0.2 = 0.12 chance for poison
		enemy.inventory[1] = consumables[2];
	else if (rand()%5 == 1) //(1-0.4-0.12)*0.2 = 0.096 chance for useless potion
		enemy.inventory[1] = consumables[3];
	else if (rand()%10 == 1) //(1-0.4-0.12-0.096)*0.1 = 0.038 chance for exp potion
		enemy.inventory[1] = consumables[4];
	else if (rand()%5 == 1) //(1-0.4-0.12-0.096-0.038)*0.2 = 0.069 chance for power potion
		enemy.inventory[1] = consumables[5];
	else //1-0.4-0.12-0.096-0.038-0.069 = 0.28 chance for no potion (dem mobs are thirsty)
		enemy.inventory[1] = zero_item;
	if (rand()%5 == 1) //0.25 chance to have a scroll
	{
		int r = rand()%2;
		if (r == 0) //0.25*0.5 = 0.125 chance for scroll of teleportation, same chance for any scroll
			enemy.inventory[2] = consumables[1];
		else if (r == 1) //useless scroll
			enemy.inventory[2] = consumables[6];
	}
	else
		enemy.inventory[2] = zero_item;
	enemy.descr = simple_melee[roll].descr; enemy.name = simple_melee[roll].name; enemy.gender = simple_melee[roll].gender;
	enemy.minDmg = simple_melee[roll].minDmg; enemy.maxDmg = simple_melee[roll].maxDmg; enemy.sig = simple_melee[roll].sig;
	enemy.maxDmg += enemy.inventory[0].damage; enemy.minDmg += enemy.inventory[0].damage;
	if (strcmp(enemy.inventory[0].ego, "venom") != 0) //venom doesn't do damage immidiately but over time
	{
		enemy.maxDmg += enemy.inventory[0].egoDmg; 
		enemy.minDmg += enemy.inventory[0].egoDmg;
	}
	enemy.maxDmg += enemy.inventory[0].bonusDmg; enemy.minDmg += enemy.inventory[0].bonusDmg;
	enemy.previousDot = YELLOW"."RESET; enemy.exp = simple_melee[roll].exp; enemy.id = simple_melee[roll].id;
	for (int i = 0; i < 2; i++)
	{
		enemy.condition[i] = 0;
		enemy.duration[i] = 0;
	}
	while (1)
	{
		enemy.i = 1 + rand()%(Room[n].DIMENSION_I - 2);
		enemy.j = 1 + rand()%(Room[n].DIMENSION_J - 2);
		if (strcmp(Room[n].grid[enemy.i][enemy.j], YELLOW"."RESET) == 0)
		{
			Room[n].grid[enemy.i][enemy.j] = enemy.sig;
			break;
		}
	}
	return enemy;
}

object iniWeaponlessEnemy(int n)
{
	object enemy;
	int roll;
	if (n <= 6) roll = 4 + rand()%4; //1st floor, tier 1
	else if (n >= 14 && n <= 20) roll = 12 + rand()%4; //3rd floor, tier 2
	else if (n >= 28 && n <= 34) roll = 20 + rand()%4; //5th floor, tier 3
	else if (n >= 42) roll = 28 + rand()%4; //7th floor, tier 4
	if (n >=7 && n <= 13) //2nd floor, 75% tier 1, 25% tier 2
	{
		if (rand()%4 == 0) roll = 12 + rand()%4;
		else roll = 4 + rand()%4;
	}
	if (n >=21 && n <= 27) //4th floor, 50% tier 2, 50% tier 3
	{
		if (rand()%2 == 0) roll = 12 + rand()%4;
		else roll = 20 + rand()%4;
	}	
	if (n >=35 && n <= 41) //6th floor, 25% tier 3, 75% tier 4
	{
		if (rand()%4 == 0) roll = 20 + rand()%4;
		else roll = 28 + rand()%4;
	}
	enemy.weaponState = 0;
	enemy.hp = enemy.maxhp = simple_melee[roll].maxhp; enemy.isAlive = 1; enemy.id = simple_melee[roll].id;
	enemy.inventory[0] = zero_item;
	enemy.inventory[0].name = malloc((1+strlen(simple_melee[roll].effect))*sizeof(char));
	strcpy(enemy.inventory[0].name, simple_melee[roll].effect); //not a real item, do not change description!
	if (enemy.id == 6 || enemy.id == 14 || enemy.id == 31) //to make snakes and worm poisonous
	{
		enemy.inventory[0].ego = malloc(6*sizeof(char));
		strcpy(enemy.inventory[0].ego, "venom");
		if (enemy.id == 31)
			enemy.inventory[0].egoDmg = 3;
		else
			enemy.inventory[0].egoDmg = 1;
	}	
	if (rand()%5 < 2) //0.4 chance for healing potion
		enemy.inventory[1] = consumables[0];
	else if (rand()%5 == 1) //(1-0.4)*0.2 = 0.12 chance for poison
		enemy.inventory[1] = consumables[2];
	else if (rand()%5 == 1) //(1-0.4-0.12)*0.2 = 0.096 chance for useless potion
		enemy.inventory[1] = consumables[3];
	else if (rand()%10 == 1) //(1-0.4-0.12-0.096)*0.1 = 0.038 chance for exp potion
		enemy.inventory[1] = consumables[4];
	else if (rand()%5 == 1) //(1-0.4-0.12-0.096-0.038)*0.2 = 0.069 chance for power potion
		enemy.inventory[1] = consumables[5];
	else //1-0.4-0.12-0.096-0.038-0.069 = 0.28 chance for no potion (dem mobs are thirsty)
		enemy.inventory[1] = zero_item;
	if (rand()%5 == 1) //0.25 chance to have a scroll
	{
		int r = rand()%2;
		if (r == 0) //0.25*0.5 = 0.125 chance for scroll of teleportation, same chance for any scroll
			enemy.inventory[2] = consumables[1];
		else if (r == 1) //useless scroll
			enemy.inventory[2] = consumables[6];
	}
	else
		enemy.inventory[2] = zero_item;
	enemy.descr = simple_melee[roll].descr; enemy.name = simple_melee[roll].name; enemy.gender = simple_melee[roll].gender;
	enemy.minDmg = simple_melee[roll].minDmg; enemy.maxDmg = simple_melee[roll].maxDmg; enemy.sig = simple_melee[roll].sig;
	enemy.previousDot = YELLOW"."RESET; enemy.exp = simple_melee[roll].exp;
	for (int i = 0; i < 2; i++)
	{
		enemy.condition[i] = 0;
		enemy.duration[i] = 0;
	}
	while (1)
	{
		enemy.i = 1 + rand()%(Room[n].DIMENSION_I - 2);
		enemy.j = 1 + rand()%(Room[n].DIMENSION_J - 2);
		if (strcmp(Room[n].grid[enemy.i][enemy.j], YELLOW"."RESET) == 0)
		{
			Room[n].grid[enemy.i][enemy.j] = enemy.sig;
			break;
		}
	}
	return enemy;
}

object moveEnemy(object user, object enemy)
{//very clumsy but it works somehow
	if (enemy.i > user.i && enemy.j > user.j && moveEnemyTrue(enemy.i-1, enemy.j-1) == 0)  {enemy.i--; enemy.j--;}
		else if (enemy.i > user.i && enemy.j > user.j && moveEnemyTrue(enemy.i-1, enemy.j-1) == 1
		&& moveEnemyTrue(enemy.i-1, enemy.j) == 0) enemy.i--;
		else if (enemy.i > user.i && enemy.j > user.j && moveEnemyTrue(enemy.i-1, enemy.j-1) == 1
		&& moveEnemyTrue(enemy.i-1, enemy.j) == 1 && moveEnemyTrue(enemy.i, enemy.j-1) == 0) enemy.j--;
	else if (enemy.i > user.i && enemy.j < user.j && moveEnemyTrue(enemy.i-1, enemy.j+1) == 0)  {enemy.i--; enemy.j++;}
		else if (enemy.i > user.i && enemy.j < user.j && moveEnemyTrue(enemy.i-1, enemy.j+1) == 1
		&& moveEnemyTrue(enemy.i-1, enemy.j) == 0) enemy.i--;
		else if (enemy.i > user.i && enemy.j < user.j && moveEnemyTrue(enemy.i-1, enemy.j+1) == 1
		&& moveEnemyTrue(enemy.i-1, enemy.j) == 1 && moveEnemyTrue(enemy.i, enemy.j+1) == 0) enemy.j++;
	else if (enemy.i < user.i && enemy.j < user.j && moveEnemyTrue(enemy.i+1, enemy.j+1) == 0)  {enemy.i++; enemy.j++;}
		else if (enemy.i < user.i && enemy.j < user.j && moveEnemyTrue(enemy.i+1, enemy.j+1) == 1
		&& moveEnemyTrue(enemy.i+1, enemy.j) == 0) enemy.i++;
		else if (enemy.i < user.i && enemy.j < user.j && moveEnemyTrue(enemy.i+1, enemy.j+1) == 1
		&& moveEnemyTrue(enemy.i+1, enemy.j) == 1 && moveEnemyTrue(enemy.i, enemy.j+1) == 0) enemy.j++;
	else if (enemy.i < user.i && enemy.j > user.j && moveEnemyTrue(enemy.i+1, enemy.j-1) == 0)  {enemy.i++; enemy.j--;}
		else if (enemy.i < user.i && enemy.j > user.j && moveEnemyTrue(enemy.i+1, enemy.j-1) == 1
		&& moveEnemyTrue(enemy.i+1, enemy.j) == 0) enemy.i++;
		else if (enemy.i < user.i && enemy.j > user.j && moveEnemyTrue(enemy.i+1, enemy.j-1) == 1
		&& moveEnemyTrue(enemy.i+1, enemy.j) == 1 && moveEnemyTrue(enemy.i, enemy.j-1) == 0) enemy.j--;
	else if (enemy.i > user.i && enemy.j == user.j && moveEnemyTrue(enemy.i-1, enemy.j) == 0) enemy.i--;
		else if (enemy.i > user.i && enemy.j == user.j && moveEnemyTrue(enemy.i-1, enemy.j) == 1
		&& enemy.j-1 >= 0 && moveEnemyTrue(enemy.i-1, enemy.j-1) == 0) {enemy.i--; enemy.j--;}
		else if (enemy.i > user.i && enemy.j == user.j && moveEnemyTrue(enemy.i-1, enemy.j) == 1 && enemy.j+1 <= (Room[current].DIMENSION_J - 2)
		&& moveEnemyTrue(enemy.i-1, enemy.j-1) == 1 && moveEnemyTrue(enemy.i-1, enemy.j+1) == 0) {enemy.i--; enemy.j++;}
	else if (enemy.i < user.i && enemy.j == user.j && moveEnemyTrue(enemy.i+1, enemy.j) == 0)  enemy.i++;
		else if (enemy.i < user.i && enemy.j == user.j && moveEnemyTrue(enemy.i+1, enemy.j) == 1
		&& enemy.j-1 >= 0 && moveEnemyTrue(enemy.i+1, enemy.j-1) == 0) {enemy.i++; enemy.j--;}
		else if (enemy.i < user.i && enemy.j == user.j && moveEnemyTrue(enemy.i+1, enemy.j) == 0 && enemy.j+1 <= (Room[current].DIMENSION_J - 2) 
		&& moveEnemyTrue(enemy.i+1, enemy.j-1) == 1 && moveEnemyTrue(enemy.i+1, enemy.j+1) == 0) {enemy.i++; enemy.j++;}
	else if (enemy.i == user.i && enemy.j < user.j && moveEnemyTrue(enemy.i, enemy.j+1) == 0)  enemy.j++;
		else if (enemy.i == user.i && enemy.j < user.j && moveEnemyTrue(enemy.i, enemy.j+1) == 1
		&& enemy.i-1 >= 0 && moveEnemyTrue(enemy.i-1, enemy.j+1) == 0) {enemy.i--; enemy.j++;}
		else if (enemy.i == user.i && enemy.j < user.j && moveEnemyTrue(enemy.i, enemy.j+1) == 1 && enemy.i+1 <= (Room[current].DIMENSION_I - 2)
		&& moveEnemyTrue(enemy.i-1, enemy.j+1) == 1 && moveEnemyTrue(enemy.i+1, enemy.j+1) == 0) {enemy.i++; enemy.j++;}
	else if (enemy.i == user.i && enemy.j > user.j && moveEnemyTrue(enemy.i, enemy.j-1) == 0)  enemy.j--;
		else if (enemy.i == user.i && enemy.j > user.j && moveEnemyTrue(enemy.i, enemy.j-1) == 1
		&& enemy.i-1 >= 0 && moveEnemyTrue(enemy.i-1, enemy.j-1) == 0) {enemy.i--; enemy.j--;}
		else if (enemy.i == user.i && enemy.j > user.j && moveEnemyTrue(enemy.i, enemy.j-1) == 1 && enemy.i+1 <= (Room[current].DIMENSION_I - 2)
		&& moveEnemyTrue(enemy.i-1, enemy.j-1) == 1 && moveEnemyTrue(enemy.i+1, enemy.j-1) == 0) {enemy.i++; enemy.j--;}
	return enemy;
}

object attack(object victim, object offender)
{
	victim.hp -= rand()%(offender.maxDmg + 1 - offender.minDmg) + offender.minDmg;
	return victim;
}

void iniWall(int n)
{
	for (int i_upp = 0, i_lwr = Room[n].DIMENSION_I - 1, j = 0; j < Room[n].DIMENSION_J; j++) //drawing border walls
	{
		Room[n].grid[i_upp][j] = YELLOW"#"RESET;
		Room[n].grid[i_lwr][j] = YELLOW"#"RESET;
	}
	for (int i = 0, j_lft = 0, j_rght = Room[n].DIMENSION_J - 1; i < Room[n].DIMENSION_I; i++)
	{
		Room[n].grid[i][j_lft] = YELLOW"#"RESET;
		Room[n].grid[i][j_rght] = YELLOW"#"RESET;
	}	
	wall wall_1, wall_2, wall_3;
	int dimension_i, dimension_j;
	if (rand()%2 == 0)
	{
	wall_1 = division(Room[n].DIMENSION_I-1, Room[n].DIMENSION_J-1, 1, 1, 1, n); //- vert-horiz-vert, division #1, vert
	if (Room[n].DIMENSION_J < 18)
		return;
	wall_2 = division(wall_1.j, Room[n].DIMENSION_I-1, 2, 1, 1, n);// - division #2, horiz, left part
	dimension_j = Room[n].DIMENSION_J-1 - wall_1.j;// - division #2, horiz, right part
	wall_3 = division(dimension_j, Room[n].DIMENSION_I-1, 2, wall_1.j, 1, n);
	if (Room[n].DIMENSION_J < 42)
		return;
	division(wall_2.i, wall_1.j, 1, 1, 1, n); // - division #3, vert, upper left part
	dimension_i = Room[n].DIMENSION_I-1 - wall_2.i;// - division #3, vert, lower left part
	division(dimension_i, wall_1.j, 1, wall_2.i, 1, n); 
	division(wall_3.i, dimension_j, 1, 1, wall_1.j, n); //- division #3, vert, upper right part
	dimension_i = Room[n].DIMENSION_I-1 - wall_3.i; //- division #3, vert, lower right part
	division(dimension_i, dimension_j, 1, wall_3.i, wall_1.j, n);
	}
	else
	{
	wall_1 = division(Room[n].DIMENSION_J-1, Room[n].DIMENSION_I-1, 2, 1, 1, n); //- horiz-vert-horiz, division #1, horiz
	if (Room[n].DIMENSION_J < 18)
		return;
	wall_2 = division(wall_1.i, Room[n].DIMENSION_J-1, 1, 1, 1, n);// - division #2, vert, upper part
	dimension_i = Room[n].DIMENSION_I-1 - wall_1.i;// - division #2, vert, lower part
	wall_3 = division(dimension_i, Room[n].DIMENSION_J, 1, wall_1.i, 1, n);
	if (Room[n].DIMENSION_J < 42)
		return;
	division(wall_2.j, wall_1.i, 2, 1, 1, n); // - division #3, horiz, upper left part
	dimension_j = Room[n].DIMENSION_J-1 - wall_2.j;// - division #3, horiz, upper right part
	division(dimension_j, wall_1.i, 2, wall_2.j, 1, n); 
	division(wall_3.j, dimension_i, 2, 1, wall_1.i, n); //- division #3, horiz, lower left part
	dimension_j = Room[n].DIMENSION_J-1 - wall_3.j; //- division #3, horiz, lower right part
	division(dimension_j, dimension_i, 2, wall_3.j, wall_1.i, n);
	}
}

void getVoidInput()
{
	struct termios newt, oldt;
	tcgetattr(STDIN_FILENO, &oldt);
	newt = oldt;
	newt.c_lflag &= ~(ICANON | ECHO);
	tcsetattr(STDIN_FILENO, TCSANOW, &newt);
	getchar();
	tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
}

void gameOver(char* messg)
{
	remove("save");	
	clrscr();
	printf(WHITE"%s\n\nPress any key to continue.", messg);
	getVoidInput();
	for (int j = 0; j < 100; j++)
		printf(RESET"\n");
}

void iniExits(int n)
{
	int roll;
	if (Room[n].keys[0][0] != -2) //right
	{
		roll = 1 + rand()%(Room[n].DIMENSION_I - 2);
		if (strcmp(Room[n].grid[roll][Room[n].DIMENSION_J-2], YELLOW"#"RESET) == 0) //to avoid doors at wall's ends
			roll -= 1;
		Room[n].grid[roll][Room[n].DIMENSION_J-1] = WHITE"/"RESET;
		Room[n].keys[0][1] = roll;
		Room[n].keys[0][2] = Room[n].DIMENSION_J-1;
	}
	if (Room[n].keys[1][0] != -2) //left
	{
		roll = 1 + rand()%(Room[n].DIMENSION_I - 2);
		if (strcmp(Room[n].grid[roll][1], YELLOW"#"RESET) == 0) //to avoid doors at wall's ends
			roll -= 1;
		Room[n].grid[roll][0] = WHITE"/"RESET;
		Room[n].keys[1][1] = roll;
		Room[n].keys[1][2] = 0;
	}
	if (Room[n].keys[2][0] != -2) //down
	{
		roll = 1 + rand()%(Room[n].DIMENSION_J - 2);
		if (strcmp(Room[n].grid[Room[n].DIMENSION_I-2][roll], YELLOW"#"RESET) == 0) //to avoid doors at wall's ends
			roll -= 1;		
		Room[n].grid[Room[n].DIMENSION_I-1][roll] = WHITE"/"RESET;
		Room[n].keys[2][1] = Room[n].DIMENSION_I-1;
		Room[n].keys[2][2] = roll;
	}
	if (Room[n].keys[3][0] != -2) //up
	{
		roll = 1 + rand()%(Room[n].DIMENSION_J - 2);
		if (strcmp(Room[n].grid[1][roll], YELLOW"#"RESET) == 0) //to avoid doors at wall's ends
			roll -= 1;			
		Room[n].grid[0][roll] = WHITE"/"RESET;
		Room[n].keys[3][1] = 0;
		Room[n].keys[3][2] = roll;
	}
	if (Room[n].keys[4][0] != -2) //dlvl
	{
		int i, j;
		while (1)
		{
			i = 1 + rand()%(Room[n].DIMENSION_I - 2);
			j = 1 + rand()%(Room[n].DIMENSION_J - 2);
			if (strcmp(Room[n].grid[i][j], YELLOW"."RESET) == 0)
			{
				Room[n].grid[i][j] = B_WHITE">";
				break;
			}
		}
		Room[n].keys[4][1] = i;
		Room[n].keys[4][2] = j;
	}
	if (Room[n].keys[5][0] != -2) //ulvl
	{
		int i, j;
		while (1)
		{
			i = 1 + rand()%(Room[n].DIMENSION_I - 2);
			j = 1 + rand()%(Room[n].DIMENSION_J - 2);
			if (strcmp(Room[n].grid[i][j], YELLOW"."RESET) == 0)
			{
				Room[n].grid[i][j] = B_WHITE"<";
				break;
			}
		}
		Room[n].keys[5][1] = i;
		Room[n].keys[5][2] = j;
	}
}

void iniHero(void)
{
	user.hp = 20; user.maxhp = 20; user.capacity = 600;
	for (int i = 0; i < INVENTORY; i++)
		user.inventory[i] = zero_item;
	user.isAlive = 1;
	user.inventory[0] = simple_weapon[rand()%(2) + 2];
	user.inventory[0] = egoGeneration(user.inventory[0]);
	if (rand()%5 != 1) //0.8 chance for healing potion
		user.inventory[1] = consumables[0];
	else if (rand()%3 == 1) //(1-0.8)*0.333 = 0.067 chance for useless potion
		user.inventory[1] = consumables[3];
	else //1-0.8-0.067 = 0.133 chance for poison
		user.inventory[1] = consumables[2];
	int r = rand()%2;
	if (r == 0) //0.5 chance for any scroll, scroll of teleportation
		user.inventory[2] = consumables[1];
	else //useless scroll
		user.inventory[2] = consumables[6];
	user.minDmg = 0; user.maxDmg = 1; user.sig = B_WHITE"@";
	user.maxDmg += user.inventory[0].damage; user.minDmg += user.inventory[0].damage;
	if (strcmp(user.inventory[0].ego, "venom") != 0) //venom doesn't do damage immidiately but over time
	{
		user.maxDmg += user.inventory[0].egoDmg;
		user.minDmg += user.inventory[0].egoDmg;
	}
	user.maxDmg += user.inventory[0].bonusDmg; user.minDmg += user.inventory[0].bonusDmg;
	user.previousDot = B_WHITE"<"; user.exp = 0;
	for (int i = 0; i < 2; i++)
	{
		user.condition[i] = 0;
		user.duration[i] = 0;
	}
}

int moveEnemyTrue (int i, int j)
{
	if (strcmp(Room[current].grid[i][j], YELLOW"."RESET) == 0 || strcmp(Room[current].grid[i][j], B_WHITE"@") == 0 || strcmp(Room[current].grid[i][j], B_WHITE"<") == 0 || strcmp(Room[current].grid[i][j], WHITE"%"RESET) == 0
	|| strcmp(Room[current].grid[i][j], WHITE"["RESET) == 0 || strcmp(Room[current].grid[i][j], WHITE"!"RESET) == 0 || strcmp(Room[current].grid[i][j], WHITE"?"RESET ) == 0 || strcmp(Room[current].grid[i][j], B_WHITE">") == 0)
		return 0;
	return 1;
}

int checkEnemy(int i, int j, object enemy[], int numEnemy)
{
	for (int k = 0; k < numEnemy; k++)
		if (enemy[k].i == i && enemy[k].j == j && enemy[k].isAlive == 1)
			return k;
	return -1;//it shouldn't be returned ever
}

void simpleMoves(int user_move)
{
	int enemy_index;
	if ((user_move == 'l' || user_move == 1067 || user_move == 54) && moveTrue(user.i, user.j+1, Room[current].enemy, Room[current].numEnemy) == 0)
	{//right move
		Room[current].grid[user.i][user.j] = user.previousDot;
		user.previousDot = Room[current].grid[user.i][user.j+1];
		Room[current].grid[user.i][user.j+1] = user.sig;
		user.j++;
	}
	else if ((user_move == 'l' || user_move == 1067 || user_move == 54) && moveTrue(user.i, user.j+1, Room[current].enemy, Room[current].numEnemy) == 1)
	{
		enemy_index = checkEnemy(user.i, user.j+1, Room[current].enemy, Room[current].numEnemy);
		hittingEnemy(enemy_index);
	}
	else if ((user_move == 'l' || user_move == 1067 || user_move == 54) && moveTrue(user.i, user.j+1, Room[current].enemy, Room[current].numEnemy) == 2)
	{	
		sprintf(Room[current].user_messg, "You cannot move there.");
		plog(Room[current].user_messg);
	}
	else if ((user_move == 'l' || user_move == 1067 || user_move == 54) && moveTrue(user.i, user.j+1, Room[current].enemy, Room[current].numEnemy) == 3)
		doorStep();
	if ((user_move == 'h' || user_move == 1068 || user_move == 52) && moveTrue(user.i, user.j-1, Room[current].enemy, Room[current].numEnemy) == 0)
	{//left move
		Room[current].grid[user.i][user.j] = user.previousDot;
		user.previousDot = Room[current].grid[user.i][user.j-1];
		Room[current].grid[user.i][user.j-1] = user.sig;
		user.j--;
	}
	else if ((user_move == 'h' || user_move == 1068 || user_move == 52) && moveTrue(user.i, user.j-1, Room[current].enemy, Room[current].numEnemy) == 1)
	{
		enemy_index = checkEnemy(user.i, user.j-1, Room[current].enemy, Room[current].numEnemy);
		hittingEnemy(enemy_index);
	}		
	else if ((user_move == 'h' || user_move == 1068 || user_move == 52) && moveTrue(user.i, user.j-1, Room[current].enemy, Room[current].numEnemy) == 2)
	{
		sprintf(Room[current].user_messg, "You cannot move there.");
		plog(Room[current].user_messg);
	}
	else if ((user_move == 'h' || user_move == 1068 || user_move == 52) && moveTrue(user.i, user.j-1, Room[current].enemy, Room[current].numEnemy) == 3)
		doorStep();
	if ((user_move == 'j' || user_move == 1066 || user_move == 50) && moveTrue(user.i+1, user.j, Room[current].enemy, Room[current].numEnemy) == 0)
	{//down move
		Room[current].grid[user.i][user.j] = user.previousDot;
		user.previousDot = Room[current].grid[user.i+1][user.j];
		Room[current].grid[user.i+1][user.j] = user.sig;
		user.i++;
	}
	else if ((user_move == 'j' || user_move == 1066 || user_move == 50) && moveTrue(user.i+1, user.j, Room[current].enemy, Room[current].numEnemy) == 1)
	{
		enemy_index = checkEnemy(user.i+1, user.j, Room[current].enemy, Room[current].numEnemy);
		hittingEnemy(enemy_index);
	}
	else if ((user_move == 'j' || user_move == 1066 || user_move == 50) && moveTrue(user.i+1, user.j, Room[current].enemy, Room[current].numEnemy) == 2)
	{	
		sprintf(Room[current].user_messg, "You cannot move there.");
		plog(Room[current].user_messg);
	}
	else if ((user_move == 'j' || user_move == 1066 || user_move == 50) && moveTrue(user.i+1, user.j, Room[current].enemy, Room[current].numEnemy) == 3)
		doorStep();
	if ((user_move == 'k' || user_move == 1065 || user_move == 56) && moveTrue(user.i-1, user.j, Room[current].enemy, Room[current].numEnemy) == 0)
	{//up move
		Room[current].grid[user.i][user.j] = user.previousDot;
		user.previousDot = Room[current].grid[user.i-1][user.j];
		Room[current].grid[user.i-1][user.j] = user.sig;
		user.i--;
	}
	else if ((user_move == 'k' || user_move == 1065 || user_move == 56) && moveTrue(user.i-1, user.j, Room[current].enemy, Room[current].numEnemy) == 1)
	{
		enemy_index = checkEnemy(user.i-1, user.j, Room[current].enemy, Room[current].numEnemy);
		hittingEnemy(enemy_index);
	}
	else if ((user_move == 'k' || user_move == 1065 || user_move == 56) && moveTrue(user.i-1, user.j, Room[current].enemy, Room[current].numEnemy) == 2)
	{
		sprintf(Room[current].user_messg, "You cannot move there.");
		plog(Room[current].user_messg);
	}
	if ((user_move == 'k' || user_move == 1065 || user_move == 56) && moveTrue(user.i-1, user.j, Room[current].enemy, Room[current].numEnemy) == 3)
		doorStep();
	if ((user_move == 'y' || user_move == 1055 || user_move == 1049 || user_move == 55) && moveTrue(user.i-1, user.j-1, Room[current].enemy, Room[current].numEnemy) == 0)
	{//upper left move
		Room[current].grid[user.i][user.j] = user.previousDot;
		user.previousDot = Room[current].grid[user.i-1][user.j-1];
		Room[current].grid[user.i-1][user.j-1] = user.sig;
		user.i--; user.j--;
	}
	else if ((user_move == 'y' || user_move == 1055 || user_move == 1049 || user_move == 55) && moveTrue(user.i-1, user.j-1, Room[current].enemy, Room[current].numEnemy) == 1)
	{
		enemy_index = checkEnemy(user.i-1, user.j-1, Room[current].enemy, Room[current].numEnemy);
		hittingEnemy(enemy_index);
	}
	else if ((user_move == 'y' || user_move == 1055 || user_move == 1049 || user_move == 55) && moveTrue(user.i-1, user.j-1, Room[current].enemy, Room[current].numEnemy) == 2)
	{	
		sprintf(Room[current].user_messg, "You cannot move there.");
		plog(Room[current].user_messg);
	}
	if ((user_move == 'y' || user_move == 1055 || user_move == 1049 || user_move == 55) && moveTrue(user.i-1, user.j-1, Room[current].enemy, Room[current].numEnemy) == 3)
		doorStep();
	if ((user_move == 'u' || user_move == 1053 || user_move == 57) && moveTrue(user.i-1, user.j+1, Room[current].enemy, Room[current].numEnemy) == 0)
	{//upper right move
		Room[current].grid[user.i][user.j] = user.previousDot;
		user.previousDot = Room[current].grid[user.i-1][user.j+1];
		Room[current].grid[user.i-1][user.j+1] = user.sig;
		user.i--; user.j++;
	}
	else if ((user_move == 'u' || user_move == 1053 || user_move == 57) && moveTrue(user.i-1, user.j+1, Room[current].enemy, Room[current].numEnemy) == 1)
	{
		enemy_index = checkEnemy(user.i-1, user.j+1, Room[current].enemy, Room[current].numEnemy);
		hittingEnemy(enemy_index);
	}
	else if ((user_move == 'u' || user_move == 1053 || user_move == 57) && moveTrue(user.i-1, user.j+1, Room[current].enemy, Room[current].numEnemy) == 2)
	{	
		sprintf(Room[current].user_messg, "You cannot move there.");
		plog(Room[current].user_messg);
	}
	else if ((user_move == 'u' || user_move == 1053 || user_move == 57) && moveTrue(user.i-1, user.j+1, Room[current].enemy, Room[current].numEnemy) == 3)
		doorStep();
	if ((user_move == 'b' || user_move == 1056 || user_move == 1052 || user_move == 49) && moveTrue(user.i+1, user.j-1, Room[current].enemy, Room[current].numEnemy) == 0)
	{//lower left move
		Room[current].grid[user.i][user.j] = user.previousDot;
		user.previousDot = Room[current].grid[user.i+1][user.j-1];
		Room[current].grid[user.i+1][user.j-1] = user.sig;
		user.i++; user.j--;
	}
	else if ((user_move == 'b' || user_move == 1056 || user_move == 1052 || user_move == 49) && moveTrue(user.i+1, user.j-1, Room[current].enemy, Room[current].numEnemy) == 1)
	{
		enemy_index = checkEnemy(user.i+1, user.j-1, Room[current].enemy, Room[current].numEnemy);
		hittingEnemy(enemy_index);
	}
	else if ((user_move == 'b' || user_move == 1056 || user_move == 1052 || user_move == 49) && moveTrue(user.i+1, user.j-1, Room[current].enemy, Room[current].numEnemy) == 2)
	{	
		sprintf(Room[current].user_messg, "You cannot move there.");
		plog(Room[current].user_messg);
	}
	else if ((user_move == 'b' || user_move == 1056 || user_move == 1052 || user_move == 49) && moveTrue(user.i+1, user.j-1, Room[current].enemy, Room[current].numEnemy) == 3)
		doorStep();
	if ((user_move == 'n' || user_move == 1054 || user_move == 51) && moveTrue(user.i+1, user.j+1, Room[current].enemy, Room[current].numEnemy) == 0)
	{//lower right move
		Room[current].grid[user.i][user.j] = user.previousDot;
		user.previousDot = Room[current].grid[user.i+1][user.j+1];
		Room[current].grid[user.i+1][user.j+1] = user.sig;
		user.i++; user.j++;
	}
	else if ((user_move == 'n' || user_move == 1054 || user_move == 51) && moveTrue(user.i+1, user.j+1, Room[current].enemy, Room[current].numEnemy) == 1)
	{
		enemy_index = checkEnemy(user.i+1, user.j+1, Room[current].enemy, Room[current].numEnemy); 
		hittingEnemy(enemy_index);
	}
	else if ((user_move == 'n' || user_move == 1054 || user_move == 51) && moveTrue(user.i+1, user.j+1, Room[current].enemy, Room[current].numEnemy) == 2)
	{	
		sprintf(Room[current].user_messg, "You cannot move there.");
		plog(Room[current].user_messg);
	}
	else if ((user_move == 'n' || user_move == 1054 || user_move == 51) && moveTrue(user.i+1, user.j+1, Room[current].enemy, Room[current].numEnemy) == 3)
		doorStep();
	if (user_move == '<' && strcmp(user.previousDot, B_WHITE"<") == 0 && current < 7)
	{//upstairs move
		
		
		sprintf(Room[current].user_messg, "Are you sure you want to leave the dungeon? Y/N");
		plog(Room[current].user_messg);
		draw();
		int choice = getYesNo();
		if (choice == 'y' || choice == 'Y')
		{			
			sprintf(Room[current].user_messg, "Scared, you fled from the dungeon and never found the courage to return.");
			plog(Room[current].user_messg);
			gameOver(Room[current].user_messg);
			user.isAlive = 0;
			return;
		}
		else
		{
			sprintf(Room[current].user_messg, "Okay then.");
			plog(Room[current].user_messg);
		}
	}
	else if (user_move == '<' && strcmp(user.previousDot, B_WHITE"<") == 0 && current >= 7)
	{
		Room[current].grid[user.i][user.j] = user.previousDot;
		current = Room[current].keys[5][0];
		user.i = Room[current].keys[4][1]; user.j = Room[current].keys[4][2];
		user.previousDot = Room[current].grid[user.i][user.j];
		Room[current].grid[user.i][user.j] = user.sig;
		clrscr();
	}
	else if (user_move == '<')
	{
		sprintf(Room[current].user_messg, "You cannot go upstairs here.");
		plog(Room[current].user_messg);
	}
	if (user_move == '>' && strcmp(user.previousDot, B_WHITE">") == 0)
	{//downstairs move
		Room[current].grid[user.i][user.j] = user.previousDot;
		current = Room[current].keys[4][0];
		user.i = Room[current].keys[5][1]; user.j = Room[current].keys[5][2];
		user.previousDot = Room[current].grid[user.i][user.j];
		Room[current].grid[user.i][user.j] = user.sig;
		clrscr();
	}
	else if (user_move == '>')
	{
		sprintf(Room[current].user_messg, "You cannot go downstairs here.");
		plog(Room[current].user_messg);
	}	
	if (user_move == '.' || user_move == 53)
	{
		sprintf(Room[current].user_messg, "You wait.");
		plog(Room[current].user_messg);
	}
}

void trapChecking()
{
	for (int i = 0; i < Room[current].numTraps; i++)
		if (user.i == Room[current].trap[i].i && user.j == Room[current].trap[i].j)
		{
			if (strcmp(Room[current].trap[i].name, "acid pit") == 0)
			{
				user = attack(user, Room[current].trap[i]);
				if (user.hp <= 0)
				{
					sprintf(Room[current].messg, "Aww! You have died a terrible death in %s.\n", Room[current].trap[i].name);
					plog(Room[current].messg);
					gameOver(Room[current].messg);
					user.isAlive = 0;
					return;
				}
				else
				{
					sprintf(Room[current].messg, "You %s!", Room[current].trap[i].effect);
					user.previousDot = Room[current].trap[i].sig;
				}
			}
			else if (strcmp(Room[current].trap[i].name, "teleportation pad") == 0)
			{
				sprintf(Room[current].messg, "%s", Room[current].trap[i].effect);
				Room[current].grid[Room[current].trap[i].i][Room[current].trap[i].j] = Room[current].trap[i].sig;
				while (1)
				{
					user.i = 1 + rand()%(Room[current].DIMENSION_I - 2);
					user.j = 1 + rand()%(Room[current].DIMENSION_J - 2);
					if (strcmp(Room[current].grid[user.i][user.j], YELLOW"."RESET) == 0)
					{
						Room[current].grid[user.i][user.j] = user.sig;
						user.previousDot = YELLOW"."RESET;
						break;
					}
				}
			}
			else if (strcmp(Room[current].trap[i].name, "venom pond") == 0)
			{
				sprintf(Room[current].messg, "%s", Room[current].trap[i].effect);
				user.previousDot = Room[current].trap[i].sig;
				int strength;
				if (current/7 <= 1)
					strength = 1;
				else if (current/7 >= 2 && current/7 <= 4)
					strength = 2;
				else if (current/7 >= 5)
					strength = 3;
				user.duration[0] += 4;
				if (user.condition[0] < strength) //only the strongest poison applies, but duration sums for all poisons
					user.condition[0] = strength;
			}
			plog(Room[current].messg);
			Log.messg_count++;
			Log.thisTurnInd[Log.messg_count-1] = Log.index;
		}
}

void enemyAction(int k, int user_move)
{
	if (Room[current].enemy[k].hp <=0 && Room[current].enemy[k].isAlive == 1) //don't move this check after trap check?
	{
		sprintf(Room[current].user_messg, "You have slain the %s!", Room[current].enemy[k].name);
		plog(Room[current].user_messg);
		user.exp += Room[current].enemy[k].exp;
		enemyDeath(k, Room[current].user_messg);
		return;
	}
	if (Room[current].enemy[k].condition[0] != 0 && Room[current].enemy[k].isAlive == 1) //enemy suffers from poison
	{
		Room[current].enemy[k].hp -= Room[current].enemy[k].condition[0];
		if (Room[current].enemy[k].hp <= 0)
		{
			user.exp += Room[current].enemy[k].exp;
			sprintf(Room[current].messg, "%c%s succumbs to the poison and dies!", toupper((int)Room[current].enemy[k].name[0]), Room[current].enemy[k].name+1);
			enemyDeath(k, Room[current].messg);
			plog(Room[current].messg);
			Log.messg_count++;
			Log.thisTurnInd[Log.messg_count-1] = Log.index;
			return;
		}
		Room[current].enemy[k].duration[0] -= 1;
		if (Room[current].enemy[k].duration[0] == 0) //enemy overcome the poison
			Room[current].enemy[k].condition[0] = 0;
	}		
	object temporary = moveEnemy(user, Room[current].enemy[k]);
	if (Room[current].enemy[k].isAlive == 1 && (temporary.i != user.i || temporary.j != user.j))
	{
		Room[current].grid[Room[current].enemy[k].i][Room[current].enemy[k].j] = Room[current].enemy[k].previousDot;
		Room[current].enemy[k] = moveEnemy(user, Room[current].enemy[k]);
		Room[current].enemy[k].previousDot = Room[current].grid[Room[current].enemy[k].i][Room[current].enemy[k].j];
		Room[current].grid[Room[current].enemy[k].i][Room[current].enemy[k].j] = Room[current].enemy[k].sig;
	}
	else if (Room[current].enemy[k].isAlive == 1)
	{
		char* strike = rand()%5 > 0 ? (rand()%2 == 0 ? "hits" : "strikes") : "jabs";
		sprintf(Room[current].messg, "%c%s %s %s %s.", toupper((int)Room[current].enemy[k].name[0]), Room[current].enemy[k].name+1, strike, "you with", Room[current].enemy[k].inventory[0].name);
		if (Room[current].enemy[k].weaponState == 1 && strcmp(Room[current].enemy[k].inventory[0].ego, "flame") == 0)
			sprintf(Room[current].messg,"%s It burns!", Room[current].messg);
		else if (Room[current].enemy[k].weaponState == 1 && strcmp(Room[current].enemy[k].inventory[0].ego, "frost") == 0)
			sprintf(Room[current].messg, "%s It freezes!", Room[current].messg);
		else if (Room[current].enemy[k].weaponState == 1 && strcmp(Room[current].enemy[k].inventory[0].ego, "spark") == 0)
			sprintf(Room[current].messg, "%s It shocks!", Room[current].messg);
		else if (strcmp(Room[current].enemy[k].inventory[0].ego, "venom") == 0)
		{
			sprintf(Room[current].messg, "%s You are poisoned!", Room[current].messg);
			user.duration[0] += 3;
			if (user.condition[0] < Room[current].enemy[k].inventory[0].egoDmg) //only the strongest poison applies, but duration sums for all poisons
				user.condition[0] = Room[current].enemy[k].inventory[0].egoDmg;
		}
		plog(Room[current].messg);
		Log.messg_count++;
		Log.thisTurnInd[Log.messg_count-1] = Log.index;
		user = attack(user, Room[current].enemy[k]);
		if (user.hp <= 0)
		{
			if (Room[current].enemy[k].weaponState == 1)
			{
				sprintf(Room[current].messg, "%s\nAww! You have been slain by a %s wielding %s.", Room[current].messg, Room[current].enemy[k].name, Room[current].enemy[k].inventory[0].name);
				plog(Room[current].messg);
			}
			else
			{
				sprintf(Room[current].messg, "%s\nAww! You have fallen a prey of %s's %s.", Room[current].messg, Room[current].enemy[k].name, Room[current].enemy[k].inventory[0].name);
				plog(Room[current].messg);
			}
			gameOver(Room[current].messg);
			user.isAlive = 0;
			return;
		}
	}
	if (Room[current].enemy[k].isAlive == 1)
	{
		for (int i = 0; i < Room[current].numTraps; i++)
		{
			if (Room[current].enemy[k].i == Room[current].trap[i].i && Room[current].enemy[k].j == Room[current].trap[i].j)
			{
				if (strcmp(Room[current].trap[i].name, "an acid pit") == 0)
				{
					Room[current].enemy[k] = attack(Room[current].enemy[k], Room[current].trap[i]);
					if (Room[current].enemy[k].hp <= 0)
					{
						sprintf(Room[current].messg, "%c%s %s %s.", toupper((int)Room[current].enemy[k].name[0]), Room[current].enemy[k].name+1, Room[current].trap[i].effect2, Room[current].enemy[k].gender);
						enemyDeath(k, Room[current].messg);
					}
					else
					{
						sprintf(Room[current].messg, "%c%s %s %s.", toupper((int)Room[current].enemy[k].name[0]), Room[current].enemy[k].name+1, Room[current].trap[i].effect, Room[current].enemy[k].gender);
						Room[current].enemy[k].previousDot = Room[current].trap[i].sig;
					}
					plog(Room[current].messg);
					Log.messg_count++;
					Log.thisTurnInd[Log.messg_count-1] = Log.index;
				}
				else if (strcmp(Room[current].trap[i].name, "teleportation pad") == 0)
				{
					sprintf(Room[current].messg, "%c%s %s", toupper((int)Room[current].enemy[k].name[0]), Room[current].enemy[k].name+1, Room[current].trap[i].effect2);
					plog(Room[current].messg);
					Log.messg_count++;
					Log.thisTurnInd[Log.messg_count-1] = Log.index;
					Room[current].grid[Room[current].trap[i].i][Room[current].trap[i].j] = Room[current].trap[i].sig;
					while (1)
					{
						Room[current].enemy[k].i = 1 + rand()%(Room[current].DIMENSION_I - 2);
						Room[current].enemy[k].j = 1 + rand()%(Room[current].DIMENSION_J - 2);
						if (strcmp(Room[current].grid[Room[current].enemy[k].i][Room[current].enemy[k].j], YELLOW"."RESET) == 0)
						{
							Room[current].grid[Room[current].enemy[k].i][Room[current].enemy[k].j] = Room[current].enemy[k].sig;
							Room[current].enemy[k].previousDot = YELLOW"."RESET;
							break;
						}
					}
				}
				else if (strcmp(Room[current].trap[i].name, "venom pond") == 0)
				{
					if (Room[current].enemy[k].id != 4 && Room[current].enemy[k].id != 12 && Room[current].enemy[k].id != 20 && Room[current].enemy[k].id != 28 && 
					Room[current].enemy[k].id != 2 && Room[current].enemy[k].id != 10 && Room[current].enemy[k].id != 18 && Room[current].enemy[k].id != 25 && Room[current].enemy[k].id != 29)
					{
						sprintf(Room[current].messg, "%c%s %s", toupper((int)Room[current].enemy[k].name[0]), Room[current].enemy[k].name+1, Room[current].trap[i].effect2);
						int strength;
						if (current/7 <= 1)
							strength = 1;
						else if (current/7 >= 2 && current/7 <= 4)
							strength = 2;
						else if (current/7 >= 5)
							strength = 3;
						Room[current].enemy[k].duration[0] += 4;
						if (Room[current].enemy[k].condition[0] < strength) //only the strongest poison applies, but duration sums for all poisons
							Room[current].enemy[k].condition[0] = strength;
					}
					else //some mobs cannot be poisoned
						sprintf(Room[current].messg, "%c%s is stepped into a venom pond.", toupper((int)Room[current].enemy[k].name[0]), Room[current].enemy[k].name+1);
					Room[current].enemy[k].previousDot = Room[current].trap[i].sig;
					plog(Room[current].messg);
					Log.messg_count++;
					Log.thisTurnInd[Log.messg_count-1] = Log.index;
				}
			}
		}
	}
}

void envir()//careful, every isAlive = 0 should have numAliveEnmey-- and the call to this function
{
	if (Room[current].numAliveEnemy > 1)
		sprintf(Room[current].envir, WHITE"%s There are %d enemies here.", Room[current].descr, Room[current].numAliveEnemy);
	else if (Room[current].numAliveEnemy == 1)
	{
		int i = 0;
		while (Room[current].enemy[i].isAlive != 1)
			i++;
		if (Room[current].enemy[i].weaponState == 1)
			sprintf(Room[current].envir, WHITE"%s %s.", Room[current].enemy[i].descr, Room[current].enemy[i].inventory[0].name);
		else
			sprintf(Room[current].envir, WHITE"%s", Room[current].enemy[i].descr);
	}
	else
		sprintf(Room[current].envir, WHITE"%s", Room[current].descr);
}

void plog(char* toLog)
{
	if ((Log.index + 1) < 20)
		Log.index++;
	else
		Log.index = 0;
	Log.log[Log.index] = malloc((1+strlen(toLog))*sizeof(char));
	strcpy(Log.log[Log.index], toLog);
}

int getEnter(void)
{
	struct termios newt, oldt;
	tcgetattr(STDIN_FILENO, &oldt);
	newt = oldt;
	newt.c_lflag &= ~(ICANON | ECHO);
	tcsetattr(STDIN_FILENO, TCSANOW, &newt); //enable raw mode	
	int input = getchar();	
	tcsetattr(STDIN_FILENO, TCSANOW, &oldt); //return old config
	if (input == 10)
		return 1;
	else
		return 0;
}

wall division(int dimension_1, int dimension_2, int kind, int lowbound_1, int lowbound_2, int n)
{
	int minwall = (int)(2*dimension_1/3);
	int maxwall = dimension_1-3;
	int length; 
	if (maxwall >= minwall)
		length = minwall + rand()%(maxwall-minwall+1);
	else
		length = minwall;
	int maxdelta = (int)(dimension_2/4); //~middle for the wall
	int delta = rand()%maxdelta;
	if (rand()%2 == 0)
		delta *=-1;
	int midd = (int)(dimension_2/2)+delta;
	int shift = rand()%(dimension_1-length);//wall shift
	wall wall;
	if (kind == 1) //vertical division
	{
		wall.j = lowbound_2 + midd;
		wall.jj = wall.j;
		wall.i = lowbound_1 + shift;
		wall.ii = wall.i + length-1;
		for (int k = wall.i; k <= wall.ii; k++)
			Room[n].grid[k][wall.j] = YELLOW"#"RESET;
	}
	else if (kind == 2) //horizontal division
	{
		wall.i = lowbound_2 + midd;
		wall.ii = wall.i;
		wall.j = lowbound_1 + shift;
		wall.jj = wall.j + length-1;
		for (int k = wall.j; k <= wall.jj; k++)
			Room[n].grid[wall.i][k] = YELLOW"#"RESET;
	}
	return wall;
}

void dropItem(int i, int j, item dropped_item, int n)
{
	if (Room[n].drop[i][j] == NULL) //creating linked list for a Room[n].grid point and adding items to it
	{
		Room[n].drop[i][j] = malloc(sizeof(node)); //root node
		Room[n].drop[i][j]->Item = dropped_item;
		Room[n].drop[i][j]->next = NULL;
	}
	else
	{
		node* new_item = malloc(sizeof(node)); //new node
		new_item->Item = dropped_item;
		new_item->next = Room[n].drop[i][j];
		Room[n].drop[i][j] = new_item;
	}
}

void itemMoves(int user_move)
{
	char alphabet[] = {'a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v','w','x','y','z','A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P','Q','R','S','T','U','V','W','X','Y','Z'};
	if (user_move == 'g')
	{
		if (Room[current].drop[user.i][user.j] != NULL)
		{
			sprintf(Room[current].drop_screen, "What do you want to pick?\n\n");	
			node* cursor = Room[current].drop[user.i][user.j]; //traversing linked list
			int itemCount = 0;
			while(cursor != NULL && itemCount < 52)
			{
				if (cursor->Item.type >= 100)
					sprintf(Room[current].drop_screen, "%s%c. %s\n", Room[current].drop_screen, alphabet[itemCount], cursor->Item.name);
				else
				{
					sprintf(Room[current].drop_screen, "%s%c. %s (+%d base +%d bonus", Room[current].drop_screen, alphabet[itemCount], cursor->Item.name, cursor->Item.damage, cursor->Item.bonusDmg);
					if (strcmp(cursor->Item.ego, " ") != 0)
						sprintf(Room[current].drop_screen, "%s +%d %s)\n", Room[current].drop_screen, cursor->Item.egoDmg, cursor->Item.ego);
					else
						sprintf(Room[current].drop_screen, "%s)\n", Room[current].drop_screen);
				}
				cursor = cursor->next;
				itemCount++;
			}
			sprintf(Room[current].drop_screen, "%s\n0. Cancel\n", Room[current].drop_screen);
			if (itemCount == 52)
				sprintf(Room[current].drop_screen, "%sSome items at the bottom of the pile cannot be reached.\n", Room[current].drop_screen);
			clrscr();
			printf("%s", Room[current].drop_screen);
			int user_pick = getInputInv(Room[current].drop_screen, alphabet[itemCount-1], 0);
			if (user_pick != '0')
			{
				int indx, inv_index = 1;
				for (int i = 1; i < INVENTORY; i++)
				{
					if (strcmp(user.inventory[inv_index].descr, "empty") == 0)
						break;
					inv_index++;
					if ((inv_index == INVENTORY - 1) && strcmp(user.inventory[inv_index].descr, "empty") != 0)
						inv_index = -1; //no place in pockets
				}	
				for (indx = 0; indx < itemCount; indx++) //or just < 52
					if (alphabet[indx] == user_pick)
						break;
				if (inv_index == -1)
				{
					sprintf(Room[current].user_messg, "You don't have enough place in your pockets to pick it.");
					plog(Room[current].user_messg);
				}
				else if (user_pick == 'a') //picking items from linked list, from head
				{
					if (user.capacity > Room[current].drop[user.i][user.j]->Item.weight)
					{
						user.capacity -= Room[current].drop[user.i][user.j]->Item.weight;
						node* temp = Room[current].drop[user.i][user.j];
						user.inventory[inv_index] = Room[current].drop[user.i][user.j]->Item;
						Room[current].drop[user.i][user.j] = Room[current].drop[user.i][user.j]->next;
						free(temp);
						sprintf(Room[current].user_messg, "You have picked %s.", user.inventory[inv_index].name);
						plog(Room[current].user_messg);
						if (strcmp(user.previousDot, B_WHITE"<") != 0 && strcmp(user.previousDot, B_WHITE">") != 0) //not to wipe stairs accidentally
						{
							if (Room[current].drop[user.i][user.j] != NULL)
								user.previousDot = Room[current].drop[user.i][user.j]->Item.sig;
							else
							{
								user.previousDot = YELLOW"."RESET;
								for (int i = 0; i < Room[current].numTraps; i++)
									if (user.i == Room[current].trap[i].i && user.j == Room[current].trap[i].j)
										user.previousDot = Room[current].trap[i].sig;
							}
						}
					}
					else
					{
						sprintf(Room[current].user_messg, "You cannot bear such weight.");
						plog(Room[current].user_messg);
					}
				}
				else if (user_pick == alphabet[itemCount]) //from tail
				{
					cursor = Room[current].drop[user.i][user.j];
					while(cursor->next->next != NULL)
						cursor = cursor->next;
					if (user.capacity > cursor->next->Item.weight)
					{
						user.capacity -= cursor->next->Item.weight;
						user.inventory[inv_index] = cursor->next->Item;
						free(cursor->next);
						cursor->next = NULL;
						sprintf(Room[current].user_messg, "You have picked %s.", user.inventory[inv_index].name);
						plog(Room[current].user_messg);
					}
					else
					{
						sprintf(Room[current].user_messg, "You cannot bear such weight.");
						plog(Room[current].user_messg);
					}
				}
				else //from body
				{
					cursor = Room[current].drop[user.i][user.j];
					for(int i = 1; i < indx; i++)
						cursor = cursor->next;
					if (user.capacity > cursor->next->Item.weight)
					{
						user.capacity -= cursor->next->Item.weight;
						user.inventory[inv_index] = cursor->next->Item;
						node* temp = cursor->next;
						cursor->next = cursor->next->next;
						free(temp);
						sprintf(Room[current].user_messg, "You have picked %s.", user.inventory[inv_index].name);
						plog(Room[current].user_messg);
					}
					else
					{
						sprintf(Room[current].user_messg, "You cannot bear such weight.");
						plog(Room[current].user_messg);
					}
				}
			}
			else //user chose to cancel
			{
				sprintf(Room[current].user_messg, "Okay then.");
				plog(Room[current].user_messg);
			}
		}
		else
		{
			sprintf(Room[current].user_messg, "%s", "There is nothing to pick up here.");
			plog(Room[current].user_messg);
		}
	}
	if (user_move == 'd')
	{	
		int inv_count = 0;
		sprintf(Room[current].drop_screen, "What do you want to drop?\n\n");
		for (int i = 1; i < INVENTORY; i++)
			if (strcmp(user.inventory[i].descr, "empty") != 0)
			{
				if (user.inventory[i].type >= 100)
					sprintf(Room[current].drop_screen, "%s%c. %s\n", Room[current].drop_screen, alphabet[inv_count], user.inventory[i].name);
				else
				{
					sprintf(Room[current].drop_screen, "%s%c. %s (+%d base +%d bonus", Room[current].drop_screen, alphabet[inv_count], user.inventory[i].name, user.inventory[i].damage, user.inventory[i].bonusDmg);
					if (strcmp(user.inventory[i].ego, " ") != 0)
						sprintf(Room[current].drop_screen, "%s +%d %s)\n", Room[current].drop_screen, user.inventory[i].egoDmg, user.inventory[i].ego);
					else
						sprintf(Room[current].drop_screen, "%s)\n", Room[current].drop_screen);
				}
				inv_count++;
			}
		sprintf(Room[current].drop_screen, "%s\n0. Cancel.\n", Room[current].drop_screen);
		clrscr();
		printf("%s", Room[current].drop_screen);
		int user_pick;
		if (inv_count > 0) //to set a correct range to pass to the function
			user_pick = getInputInv(Room[current].drop_screen, alphabet[inv_count-1], 0);
		else
			user_pick = getInputInv(Room[current].drop_screen, '0', 0);
		if (user_pick != '0')
		{
			int ind;
			for (ind = 0; ind < inv_count; ind++) 
				if (alphabet[ind] == user_pick)
					break;
			ind++; //number of items in inventory
			int num_item;
			for (num_item = 1; num_item < INVENTORY; num_item++) //to find its index in inventory array
			{
				if (strcmp(user.inventory[num_item].descr, "empty") != 0)
					ind--;
				if (ind == 0)
					break;
			}
			sprintf(Room[current].user_messg, "You have dropped %s.", user.inventory[num_item].name);
			dropItem(user.i, user.j, user.inventory[num_item], current);
			plog(Room[current].user_messg);
			user.capacity += user.inventory[num_item].weight;
			user.inventory[num_item] = zero_item;
			if (strcmp(user.previousDot, B_WHITE"<") != 0 && strcmp(user.previousDot, B_WHITE">") != 0) //not to wipe stairs accidentally
				user.previousDot = Room[current].drop[user.i][user.j]->Item.sig;
		}
		else
		{
			sprintf(Room[current].user_messg, "Okay then.");
			plog(Room[current].user_messg);
		}
	}
	if (user_move == 'w')
	{	
		int inv_count = 0;
		sprintf(Room[current].drop_screen, "What do you want to wield?\n\n");
		for (int i = 1; i < INVENTORY; i++)
			if (strcmp(user.inventory[i].descr, "empty") != 0 && user.inventory[i].type < 100)
			{
				sprintf(Room[current].drop_screen, "%s%c. %s (+%d base +%d bonus", Room[current].drop_screen, alphabet[inv_count], user.inventory[i].name, user.inventory[i].damage, user.inventory[i].bonusDmg);
				if (strcmp(user.inventory[i].ego, " ") != 0)
					sprintf(Room[current].drop_screen, "%s +%d %s)\n", Room[current].drop_screen, user.inventory[i].egoDmg, user.inventory[i].ego);
				else
					sprintf(Room[current].drop_screen, "%s)\n", Room[current].drop_screen);
				inv_count++;
			}
		sprintf(Room[current].drop_screen, "%s\n-. Unwield wielded weapon.\n0. Cancel.\n", Room[current].drop_screen);
		clrscr();
		printf("%s", Room[current].drop_screen);
		int user_pick;
		if (inv_count > 0) //to set a correct range to pass to the function
			user_pick = getInputInv(Room[current].drop_screen, alphabet[inv_count-1], 1);
		else
			user_pick = getInputInv(Room[current].drop_screen, '0', 1);
		if (user_pick != '0' && user_pick != '-')
		{
			int ind;
			for (ind = 0; ind < inv_count; ind++) 
				if (alphabet[ind] == user_pick)
					break;
			ind++; //number of items in inventory
			int num_item;
			for (num_item = 1; num_item < INVENTORY; num_item++) //to find its index in inventory array
			{
				if (strcmp(user.inventory[num_item].descr, "empty") != 0 && user.inventory[num_item].type < 100)
					ind--;
				if (ind == 0)
					break;
			}
			if (strcmp(user.inventory[0].name, "bare hands") != 0)
			{
				user.maxDmg -= user.inventory[0].damage; 
				user.minDmg -= user.inventory[0].damage;
				if (strcmp(user.inventory[0].ego, "venom") != 0) //venom doesn't do damage immidiately but over time
				{
					user.maxDmg -= user.inventory[0].egoDmg; 
					user.minDmg -= user.inventory[0].egoDmg;
				}
				user.maxDmg -= user.inventory[0].bonusDmg; 
				user.minDmg -= user.inventory[0].bonusDmg;
			}
			item temp_item;
			temp_item = user.inventory[0];
			user.inventory[0] = user.inventory[num_item];
			user.inventory[num_item] = temp_item;
			sprintf(Room[current].user_messg, "You wield %s.", user.inventory[0].name);
			plog(Room[current].user_messg);
			user.maxDmg += user.inventory[0].damage; user.minDmg += user.inventory[0].damage;
			if (strcmp(user.inventory[0].ego, "venom") != 0) //venom doesn't do damage immidiately but over time
			{
				user.maxDmg += user.inventory[0].egoDmg;
				user.minDmg += user.inventory[0].egoDmg;
			}
			user.maxDmg += user.inventory[0].bonusDmg; user.minDmg += user.inventory[0].bonusDmg;
		}
		else if (user_pick == '-')
		{
			int remove_index;
			for (remove_index = 1; remove_index < INVENTORY; remove_index++)
			{
				if (strcmp(user.inventory[remove_index].descr, "empty") == 0)
					break;
				if ((remove_index == INVENTORY - 1) && strcmp(user.inventory[remove_index].descr, "empty") != 0)
				{
					sprintf(Room[current].user_messg, "You don't have enough place in your pockets to place removed weapon.");
					plog(Room[current].user_messg);
					remove_index = -1;
				}
			}
			if (remove_index != -1) //potential bug since one of zero_items would have name now?
			{
				if (strcmp(user.inventory[0].descr, "empty") != 0)
				{
					user.maxDmg -= user.inventory[0].damage; user.minDmg -= user.inventory[0].damage;
					if (strcmp(user.inventory[0].ego, "venom") != 0) //venom doesn't do damage immidiately but over time
					{
						user.maxDmg -= user.inventory[0].egoDmg; 
						user.minDmg -= user.inventory[0].egoDmg;
					}
					user.maxDmg -= user.inventory[0].bonusDmg; user.minDmg -= user.inventory[0].bonusDmg;
					user.inventory[remove_index] = user.inventory[0];
					user.inventory[0] = zero_item;
					user.inventory[0].name = "bare hands";
					sprintf(Room[current].user_messg, "You are using your bare hands now.");
					plog(Room[current].user_messg);
				}
				else
				{
					sprintf(Room[current].user_messg, "You cannot remove your hands.");
					plog(Room[current].user_messg);
				}
			}
		}
		else
		{
			sprintf(Room[current].user_messg, "Okay then.");
			plog(Room[current].user_messg);
		}
	}
	if (user_move == 'q')
	{
		int inv_count = 0;
		sprintf(Room[current].drop_screen, "What do you want to quaff?\n\n");
		for (int i = 1; i < INVENTORY; i++)
			if (strcmp(user.inventory[i].descr, "empty") != 0 && user.inventory[i].type >= 100 && user.inventory[i].type < 200)
			{
				sprintf(Room[current].drop_screen, "%s%c. %s\n", Room[current].drop_screen, alphabet[inv_count], user.inventory[i].name);
				inv_count++;
			}
		sprintf(Room[current].drop_screen, "%s\n0. Cancel.\n", Room[current].drop_screen);
		clrscr();
		printf("%s", Room[current].drop_screen);
		int user_pick;
		if (inv_count > 0) //to set a correct range to pass to the function
			user_pick = getInputInv(Room[current].drop_screen, alphabet[inv_count-1], 0);
		else
			user_pick = getInputInv(Room[current].drop_screen, '0', 0);
		if (user_pick != '0')
		{
			int ind;
			for (ind = 0; ind < inv_count; ind++) 
				if (alphabet[ind] == user_pick)
					break;
			ind++; //number of items in inventory
			int num_item;
			for (num_item = 1; num_item < INVENTORY; num_item++) //to find its index in inventory array
			{
				if (strcmp(user.inventory[num_item].descr, "empty") != 0 && user.inventory[num_item].type >= 100 && user.inventory[num_item].type < 200)
					ind--;
				if (ind == 0)
					break;
			}
			sprintf(Room[current].user_messg, "You quaffed %s.", user.inventory[num_item].name);
			plog(Room[current].user_messg);
			if (user.inventory[num_item].type == 100)
			{
				if (strcmp(user.inventory[num_item].name, "healing potion") != 0)
				{
					sprintf(Room[current].messg, "You feel better! It was a healing potion.");
					renameAll(0);
				}
				else
					sprintf(Room[current].messg, "You feel better.");
				if ((user.hp + user.maxhp/4) <= user.maxhp)
					user.hp = user.hp + user.maxhp/4;
				else
					user.hp = user.maxhp;
			}
			else if (user.inventory[num_item].type == 101)
			{
				if (strcmp(user.inventory[num_item].name, "bottle of poison") != 0)
				{
					sprintf(Room[current].messg, "You are poisoned! It was a bottle of poison.");
					renameAll(2);
				}
				else
					sprintf(Room[current].messg, "You are poisoned!");
				int strength;
				if (user.maxhp <= 22)
					strength = 1;
				else if (user.maxhp >= 26 && user.maxhp <= 40)
					strength = 2;
				else if (user.maxhp >= 50)
					strength = 3;
				user.duration[0] += 4;
				if (user.condition[0] < strength) //only the strongest poison applies, but duration sums for all poisons
					user.condition[0] = strength;
			}
			else if (user.inventory[num_item].type == 102)
			{
				if (strcmp(user.inventory[num_item].name, "useless potion") != 0)
				{
					sprintf(Room[current].messg, "It tastes terrible. You feel nothing.");
					renameAll(3);
				}
				else
					sprintf(Room[current].messg, "It tastes terrible.");						
			}
			else if (user.inventory[num_item].type == 103)
			{
				if (strcmp(user.inventory[num_item].name, "potion of experience") != 0)
				{
					sprintf(Room[current].messg, "You feel more experienced. It was a potion of experience.");
					renameAll(4);
				}
				else
					sprintf(Room[current].messg, "You feel more experienced.");
				int strength;
				if (user.maxhp == 20) //i don't like switches
					strength = 10;
				else if (user.maxhp == 22)
					strength = 20;
				else if (user.maxhp == 26)
					strength = 30;
				else if (user.maxhp == 32)
					strength = 40;
				else if (user.maxhp == 40)
					strength = 50;
				else if (user.maxhp == 50)
					strength = 60;				
				else //level cap, but let's add useless exp
					strength = 70;
				user.exp += strength;	
			}
			else if (user.inventory[num_item].type == 104)
			{
				if (strcmp(user.inventory[num_item].name, "vial of power") != 0)
				{
					sprintf(Room[current].messg, "You feel mighty! It was a vial of power.");
					renameAll(5);
				}
				else
					sprintf(Room[current].messg, "You feel mighty.");	
				int strength;
				if (user.maxhp <= 22)
					strength = 1;
				else if (user.maxhp >= 26 && user.maxhp <= 40)
					strength = 2;
				else if (user.maxhp >= 50)
					strength = 3;
				user.duration[1] += 10;
				if (user.condition[1] == 0) //empower applies only once, but duration sums for every potion
				{
					user.condition[1] = strength;
					user.maxDmg += strength;
					user.minDmg += strength;
				}
			}
			plog(Room[current].messg);
			Log.messg_count++;
			Log.thisTurnInd[Log.messg_count-1] = Log.index;
			user.inventory[num_item] = zero_item;
		}
		else
		{
			sprintf(Room[current].user_messg, "Okay then.");
			plog(Room[current].user_messg);
		}
	}
	if (user_move == 'r')
	{
		int inv_count = 0;
		sprintf(Room[current].drop_screen, "What do you want to read?\n\n");
		for (int i = 1; i < INVENTORY; i++)
			if (strcmp(user.inventory[i].descr, "empty") != 0 && user.inventory[i].type >= 200 && user.inventory[i].type < 300)
			{
				sprintf(Room[current].drop_screen, "%s%c. %s\n", Room[current].drop_screen, alphabet[inv_count], user.inventory[i].name);
				inv_count++;
			}
		sprintf(Room[current].drop_screen, "%s\n0. Cancel.\n", Room[current].drop_screen);
		clrscr();
		printf("%s", Room[current].drop_screen);
		int user_pick;
		if (inv_count > 0) //to set a correct range to pass to the function
			user_pick = getInputInv(Room[current].drop_screen, alphabet[inv_count-1], 0);
		else
			user_pick = getInputInv(Room[current].drop_screen, '0', 0);
		if (user_pick != '0')
		{
			int ind;
			for (ind = 0; ind < inv_count; ind++) 
				if (alphabet[ind] == user_pick)
					break;
			ind++; //number of items in inventory
			int num_item;
			for (num_item = 1; num_item < INVENTORY; num_item++) //to find its index in inventory array
			{
				if (strcmp(user.inventory[num_item].descr, "empty") != 0 && user.inventory[num_item].type >= 200 && user.inventory[num_item].type < 300)
					ind--;
				if (ind == 0)
					break;
			}
			sprintf(Room[current].user_messg, "You read %s. It crumbles into dust.", user.inventory[num_item].name);
			plog(Room[current].user_messg);
			if (user.inventory[num_item].type == 200)
			{
				if (strcmp(user.inventory[num_item].name, "scroll of teleportation") != 0)
				{
					sprintf(Room[current].messg, "Surroundings change around you! It was a scroll of teleportation.");
					renameAll(1);
				}
				else
					sprintf(Room[current].messg, "Surroundings change around you.");
				Room[current].grid[user.i][user.j] = user.previousDot;
				while (1)
				{
					user.i = 1 + rand()%(Room[current].DIMENSION_I - 2);
					user.j = 1 + rand()%(Room[current].DIMENSION_J - 2);
					if (strcmp(Room[current].grid[user.i][user.j], YELLOW"."RESET) == 0)
					{
						Room[current].grid[user.i][user.j] = user.sig;
						user.previousDot = YELLOW"."RESET;
						break;
					}
				}
			}
			else if (user.inventory[num_item].type == 201)
			{
				if (strcmp(user.inventory[num_item].name, "useless scroll") != 0)
				{
					sprintf(Room[current].messg, "You read gibberish. Nothing happens."); 
					renameAll(6);
				}
				else
					sprintf(Room[current].messg, "What a gibberish.");
			}
			plog(Room[current].messg);
			Log.messg_count++;
			Log.thisTurnInd[Log.messg_count-1] = Log.index;			
			user.inventory[num_item] = zero_item;
		}
		else
		{
			sprintf(Room[current].user_messg, "Okay then.");
			plog(Room[current].user_messg);
		}
	}
	if (user_move == 'i')
	{
		int inv_count = 0;
		sprintf(Room[current].drop_screen, "Equipment:\n\nweapon - %s (+%d base +%d bonus", user.inventory[0].name, user.inventory[0].damage, user.inventory[0].bonusDmg);
		if (strcmp(user.inventory[0].ego, " ") != 0)
			sprintf(Room[current].drop_screen, "%s +%d %s)\n\nInventory:\n\n", Room[current].drop_screen, user.inventory[0].egoDmg, user.inventory[0].ego);
		else
			sprintf(Room[current].drop_screen, "%s)\n\nInventory:\n\n", Room[current].drop_screen);
		for (int i = 1; i < INVENTORY; i++)
			if (strcmp(user.inventory[i].descr, "empty") != 0)
			{
				if (user.inventory[i].type >= 100)
					sprintf(Room[current].drop_screen, "%s%c. %s\n", Room[current].drop_screen, alphabet[inv_count], user.inventory[i].name);
				else
				{
					sprintf(Room[current].drop_screen, "%s%c. %s (+%d base +%d bonus", Room[current].drop_screen, alphabet[inv_count], user.inventory[i].name, user.inventory[i].damage, user.inventory[i].bonusDmg);
					if (strcmp(user.inventory[i].ego, " ") != 0)
						sprintf(Room[current].drop_screen, "%s +%d %s)\n", Room[current].drop_screen, user.inventory[i].egoDmg, user.inventory[i].ego);
					else
						sprintf(Room[current].drop_screen, "%s)\n", Room[current].drop_screen);
				}
				inv_count++;
			}
		sprintf(Room[current].drop_screen, "%s\nPress any key to continue.\n", Room[current].drop_screen);
		clrscr();
		printf("%s", Room[current].drop_screen);
		getVoidInput();
		clrscr();
	}	
}

void iniItem(int n)
{
	item Item;
	if (rand()%3 > 0) //0.66 chance for generating consumables
	{
		if (rand()%2 == 1) //0.66*0.5 = 0.33 chance for potion of healing
			Item = consumables[0];
		else if (rand()%5 == 1) //(0.66-0.33)*0.2 = 0.066 chance for potion of poison
			Item = consumables[2];
		else if (rand()%7 == 1) //(0.66-0.33-0.066)*0.143 = 0.038 chance for useless potion
			Item = consumables[3];
		else if (rand()%10 == 1) //(0.66-0.33-0.066-0.038)*0.1 = 0.023 chance for exp potion
			Item = consumables[4];
		else if (rand()%7 == 1) //(0.66-0.33-0.066-0.038-0.023)*0.143 = 0.029 chance for potion of power
			Item = consumables[5];
		else //0.66-0.33-0.066-0.038-0.023-0.029 = 0.174 chance for some scroll
		{	
			int r = rand()%2;
			if (r == 0) //0.174*0.5 = 0.087 chance for every scroll, scroll of teleportation
				Item = consumables[1];
			else if (r == 1)
				Item = consumables[6];
		}
	}
	else //0.34 chance for generating weapon
	{
		if (n <= 13) //1-2 floor, tier 1 weapon drop
			Item = egoGeneration(simple_weapon[rand()%8]);
		else if (n >= 14 && n <= 34) //3-5 floor, tier 2 weapon drop
			Item = egoGeneration(simple_weapon[8 + rand()%8]);
		else //6-7 floor, tier 3 weapin drop
			Item = egoGeneration(simple_weapon[16 + rand()%8]);
	}
	int i, j;
	while (1)
	{
		i = 1 + rand()%(Room[n].DIMENSION_I - 2);
		j = 1 + rand()%(Room[n].DIMENSION_J - 2);
		if (strcmp(Room[n].grid[i][j], YELLOW"."RESET) == 0)
		{
			Room[n].grid[i][j] = Item.sig;
			break;
		}
	}
	dropItem(i, j, Item, n);
}

item egoGeneration(item Item)
{
	if (rand()%5 == 2) // bonus damage
		Item.bonusDmg = rand()%(Item.damage + 1);
	if (Item.type == 13) //not to overwrite torch's ego
		return Item;
	int egoDmg; //possible ego damage
	if (Item.type <= 7)
		egoDmg = 1;
	else if (Item.type >= 8 && Item.type <= 15)
		egoDmg = 2;
	else
		egoDmg = 3;
	int len = strlen(Item.name);
	char line[100];
	int roll = rand()%20;
	if (roll == 5)
	{
		Item.ego = "flame";
		Item.egoDmg = egoDmg;
		sprintf(line, "%s of flame", Item.name);
		Item.name = malloc((len + 10)*sizeof(char));
		sprintf(Item.name, "%s", line);
	}
	else if (roll == 4)
	{
		Item.ego = "frost";
		Item.egoDmg = egoDmg;
		sprintf(line, "%s of frost", Item.name);
		Item.name = malloc((len + 10)*sizeof(char));
		sprintf(Item.name, "%s", line);
	}
	else if (roll == 3)
	{
		Item.ego = "spark";
		Item.egoDmg = egoDmg;
		sprintf(line, "%s of spark", Item.name);
		Item.name = malloc((len + 10)*sizeof(char));
		sprintf(Item.name, "%s", line);
	}
	else if (roll == 6)
	{
		Item.ego = "venom";
		Item.egoDmg = egoDmg;
		sprintf(line, "%s of venom", Item.name);
		Item.name = malloc((len + 10)*sizeof(char));
		sprintf(Item.name, "%s", line);
	}		
	return Item;
}

void egoMessg(int i)
{
	if (strcmp(user.inventory[0].ego, "flame") == 0)
		sprintf(Room[current].user_messg, "%s You burn %s.", Room[current].user_messg, Room[current].enemy[i].gender);
	else if (strcmp(user.inventory[0].ego, "frost") == 0)
		sprintf(Room[current].user_messg, "%s You freeze %s.", Room[current].user_messg, Room[current].enemy[i].gender);
	else if (strcmp(user.inventory[0].ego, "spark") == 0)
		sprintf(Room[current].user_messg, "%s You shock %s.", Room[current].user_messg, Room[current].enemy[i].gender);
	else if (strcmp(user.inventory[0].ego, "venom") == 0)	
		sprintf(Room[current].user_messg, "%s You poison %s.", Room[current].user_messg, Room[current].enemy[i].gender);	
}

void renameAll(int s)
{
	for (int k = 0; k < 49; k++)
	{
		for (int i = 0; i < INVENTORY; i++)
			user.inventory[i] = renameCheck(user.inventory[i], s);
		for (int i = 0; i < Room[k].numEnemy; i++)
			for (int j = 0; j < E_INVENTORY; j++)
				Room[k].enemy[i].inventory[j] = renameCheck(Room[k].enemy[i].inventory[j], s);
		for (int i = 0; i < Room[k].DIMENSION_I; i++)
			for (int j = 0; j < Room[k].DIMENSION_J; j++)
			{
				node* cursor = Room[k].drop[i][j];
				while(cursor != NULL)
				{
					cursor->Item = renameCheck(cursor->Item, s);
					cursor = cursor->next;
				}
			}
	}	
}	

item renameCheck(item Item, int s)
{
	if (Item.type == 100 && s == 0)
	{
		Item.name = malloc(15*sizeof(char));
		strcpy(Item.name, "healing potion");
	}
	else if (Item.type == 200 && s == 1)
	{
		Item.name = malloc(24*sizeof(char));
		strcpy(Item.name, "scroll of teleportation");
	}
	else if (Item.type == 101 && s == 2)
	{
		Item.name = malloc(17*sizeof(char));
		strcpy(Item.name, "bottle of poison");
	}
	else if (Item.type == 102 && s == 3)
	{
		Item.name = malloc(15*sizeof(char));
		strcpy(Item.name, "useless potion");
	}
	else if (Item.type == 103 && s == 4)
	{
		Item.name = malloc(21*sizeof(char));
		strcpy(Item.name, "potion of experience");
	}
	else if (Item.type == 104 && s == 5)
	{
		Item.name = malloc(14*sizeof(char));
		strcpy(Item.name, "vial of power");
	}
	else if (Item.type == 201 && s == 6)
	{
		Item.name = malloc(15*sizeof(char));
		strcpy(Item.name, "useless scroll");
	}	
	return Item;
}

void save() //yes, i know that fwrite and fread exist. they are way too glitchy for this task so i'm forced to parse it myself 
{//saves weight 0.5mb
	f = fopen("save", "w");
	char line[5200]; //saving general room data
	sprintf(line, "%d|", current); //to know the current room
	ciph(line);
	sprintf(line, "%d|%d|%d|%d|%d|%d|%d|%d|%d|%s|%s|%s|%s|%s|%s|%s|%d|%d|", user.i, user.j, user.hp, user.maxhp, user.isAlive, 1, user.maxDmg, user.minDmg, user.capacity, "", "", "", "", "", user.sig, user.previousDot, user.exp, 0);
	ciph(line); //saving user
	for (int i = 0; i < INVENTORY; i++) //saving his inventory
	{
		sprintf(line, "%s|%s|%s|%s|%d|%d|%d|%d|%d|", user.inventory[i].name, user.inventory[i].descr, user.inventory[i].sig, user.inventory[i].ego, user.inventory[i].egoDmg, user.inventory[i].weight, user.inventory[i].type, user.inventory[i].damage, user.inventory[i].bonusDmg);
		ciph(line);
	}
	for (int i = 0; i < 2; i++) //saving his condition
	{
		sprintf(line, "%d|%d|", user.condition[i], user.duration[i]);
		ciph(line);
	}
	for (int n = 0; n < 49; n++) //saving the whole array of rooms
	{
		sprintf(line, "%d|%d|%d|%d|%d|%s|", Room[n].DIMENSION_I, Room[n].DIMENSION_J, Room[n].numTraps, Room[n].numEnemy, Room[n].numAliveEnemy, Room[n].descr);
		ciph(line);
		sprintf(line, "%d|%d|%d|%d|%d|%d|%d|%d|%d|%d|%d|%d|%d|%d|%d|%d|%d|%d|", Room[n].keys[0][0], Room[n].keys[0][1], Room[n].keys[0][2], Room[n].keys[1][0], Room[n].keys[1][1], Room[n].keys[1][2], Room[n].keys[2][0], Room[n].keys[2][1], Room[n].keys[2][2], Room[n].keys[3][0], Room[n].keys[3][1], Room[n].keys[3][2], Room[n].keys[4][0], Room[n].keys[4][1], Room[n].keys[4][2], Room[n].keys[5][0], Room[n].keys[5][1], Room[n].keys[5][2]);
		ciph(line);
		for (int i = 0; i < Room[n].DIMENSION_I; i++) //saving Room[n].grid
			for (int j = 0; j < Room[n].DIMENSION_J; j++)
				ciph(Room[n].grid[i][j]);
		for (int i = 0; i < Room[n].DIMENSION_I; i++) //saving room drop
			for (int j = 0; j < Room[n].DIMENSION_J; j++)
			{
				node* cursor = Room[n].drop[i][j];
				while(cursor != NULL)
				{
					sprintf(line, "%s|%s|%s|%s|%d|%d|%d|%d|%d|", cursor->Item.name, cursor->Item.descr, cursor->Item.sig, cursor->Item.ego, cursor->Item.egoDmg, cursor->Item.weight, cursor->Item.type, cursor->Item.damage, cursor->Item.bonusDmg);
					cursor = cursor->next;
					ciph(line);
				}
				sprintf(line, "&");
				ciph(line);
			}
		for (int i = 0; i < Room[n].numEnemy; i++) //saving enemies
		{
			sprintf(line, "%d|%d|%d|%d|%d|%d|%d|%d|%d|%s|%s|%s|%s|%s|%s|%s|%d|%d|", Room[n].enemy[i].i, Room[n].enemy[i].j, Room[n].enemy[i].hp, Room[n].enemy[i].maxhp, Room[n].enemy[i].isAlive, Room[n].enemy[i].weaponState, Room[n].enemy[i].maxDmg, Room[n].enemy[i].minDmg, 0, Room[n].enemy[i].name, Room[n].enemy[i].descr, Room[n].enemy[i].gender, "", "", Room[n].enemy[i].sig, Room[n].enemy[i].previousDot, Room[n].enemy[i].exp, Room[n].enemy[i].id);
			ciph(line);
			for (int j = 0; j < E_INVENTORY; j++) //saving their inventory
			{
				sprintf(line, "%s|%s|%s|%s|%d|%d|%d|%d|%d|", Room[n].enemy[i].inventory[j].name, Room[n].enemy[i].inventory[j].descr, Room[n].enemy[i].inventory[j].sig, Room[n].enemy[i].inventory[j].ego, Room[n].enemy[i].inventory[j].egoDmg, Room[n].enemy[i].inventory[j].weight, Room[n].enemy[i].inventory[j].type, Room[n].enemy[i].inventory[j].damage, Room[n].enemy[i].inventory[j].bonusDmg);
				ciph(line);
			}
			for (int m = 0; m < 2; m++) //saving their condition
			{
				sprintf(line, "%d|%d|", Room[n].enemy[i].condition[m], Room[n].enemy[i].duration[m]);
				ciph(line);
			}	
		}
		for (int i = 0; i < Room[n].numTraps; i++) //saving traps (+ some useless info so loadItem() would work for them too)
		{
			sprintf(line, "%d|%d|%d|%d|%d|%d|%d|%d|%d|%s|%s|%s|%s|%s|%s|%s|%d|%d|", Room[n].trap[i].i, Room[n].trap[i].j, 0, 0, 0, 0, Room[n].trap[i].maxDmg, Room[n].trap[i].minDmg, 0, Room[n].trap[i].name, "", "it", Room[n].trap[i].effect, Room[n].trap[i].effect2, Room[n].trap[i].sig, "", 0, 0);
			ciph(line);		
		}
	}
	fclose(f);
}

void load()
{	
	char cc[5200], temp[250];
	char* tmp;
	int j = 0;
	scan(cc); //loading meta room data
	tmp =  deciph(cc);
	sprintf(cc, "%s", tmp);
	free(tmp);
	parse(cc, j, temp);
	current = atoi(temp);
	scan(cc); //loading user
	tmp =  deciph(cc);
	sprintf(cc, "%s", tmp);
	free(tmp);
	user = loadObject(cc);
	for (int i = 0; i < INVENTORY; i++) //loading user inventory
	{
		scan(cc);
		tmp =  deciph(cc);
		sprintf(cc, "%s", tmp);
		free(tmp);
		item Item = loadItem(cc);
		user.inventory[i] = Item;		
	}
	for (int i = 0; i < 2; i++)
	{
		scan(cc);
		tmp =  deciph(cc);
		sprintf(cc, "%s", tmp);
		free(tmp);
		j = 0;
		parse(cc, j, temp);
		j += strlen(temp) + 1;
		user.condition[i] = atoi(temp);
		parse(cc, j, temp);	
		j += strlen(temp) + 1;
		user.duration[i] = atoi(temp);	
	}
	for (int n = 0; n < 49; n++)
	{
		scan(cc); //loading general room information
		tmp =  deciph(cc);
		sprintf(cc, "%s", tmp); //letter 'k' and 'q' cause crashes for some reason
		free(tmp);
		j = 0;
		parse(cc, j, temp);	
		j += strlen(temp) + 1;
		Room[n].DIMENSION_I = atoi(temp);
		parse(cc, j, temp);	
		j += strlen(temp) + 1;
		Room[n].DIMENSION_J = atoi(temp);		
		parse(cc, j, temp);	
		j += strlen(temp) + 1;
		Room[n].numTraps = atoi(temp);			
		parse(cc, j, temp);	
		j += strlen(temp) + 1;
		Room[n].numEnemy = atoi(temp);	
		parse(cc, j, temp);	
		j += strlen(temp) + 1;
		Room[n].numAliveEnemy = atoi(temp);	
		parse(cc, j, temp);
		j += strlen(temp) + 1;
		Room[n].descr = malloc((1+strlen(temp))*sizeof(char));
		strcpy(Room[n].descr, temp);
		scan(cc); //loading keys
		tmp = deciph(cc);
		sprintf(cc, "%s", tmp);
		free(tmp);
		j = 0;
		for (int i = 0; i < 6; i++)
			for (int k = 0; k < 3; k++)
			{
				parse(cc, j, temp);	
				j += strlen(temp) + 1;
				Room[n].keys[i][k] = atoi(temp);			
			}
		for (int i = 0; i < Room[n].DIMENSION_I; i++) //loading Room[n].grid
			for (int j = 0; j < Room[n].DIMENSION_J; j++)
			{
				scan(cc);
				Room[n].grid[i][j] = deciph(cc);
			}
		for (int i = 0; i < Room[n].DIMENSION_I; i++) //loading room drop
			for (int j = 0; j < Room[n].DIMENSION_J; j++)
				while(1) //loading drop for one dot of the Room[n].grid
				{
					scan(cc);
					tmp =  deciph(cc);
					sprintf(cc, "%s", tmp);
					free(tmp);
					if (cc[0] == '&')
						break;
					item Item = loadItem(cc);
					dropItem(i, j, Item, n);
				}
		for (int i = 0; i < Room[n].numEnemy; i++) //loading enemies
		{
			scan(cc); //loading enemy[i]
			tmp =  deciph(cc);
			sprintf(cc, "%s", tmp);
			free(tmp);
			Room[n].enemy[i] = loadObject(cc);
			for (int j = 0; j < E_INVENTORY; j++) //loading enemy[i] inventory
			{
				scan(cc);
				tmp =  deciph(cc);
				sprintf(cc, "%s", tmp);
				free(tmp);
				item Item = loadItem(cc);
				Room[n].enemy[i].inventory[j] = Item;	
			}
			for (int h = 0; h < 2; h++)
			{
				scan(cc);
				tmp =  deciph(cc);
				sprintf(cc, "%s", tmp);
				free(tmp);
				j = 0;
				parse(cc, j, temp);
				j += strlen(temp) + 1;
				Room[n].enemy[i].condition[h] = atoi(temp);
				parse(cc, j, temp);	
				j += strlen(temp) + 1;
				Room[n].enemy[i].duration[h] = atoi(temp);	
			}
		}
		for (int i = 0; i < Room[n].numTraps; i++) //loading traps
		{
			scan(cc); //loading trap[i]
			tmp =  deciph(cc);
			sprintf(cc, "%s", tmp);
			free(tmp);
			Room[n].trap[i] = loadObject(cc);
		}
	}
	fclose(f);		
}

object loadObject(char* cc)
{
	object Object;
	int j = 0;
	char temp[250];
	parse(cc, j, temp);	
	j += strlen(temp) + 1;
	Object.i = atoi(temp);			
	parse(cc, j, temp);	
	j += strlen(temp) + 1;
	Object.j = atoi(temp);	
	parse(cc, j, temp);	
	j += strlen(temp) + 1;
	Object.hp = atoi(temp);
	parse(cc, j, temp);	
	j += strlen(temp) + 1;
	Object.maxhp = atoi(temp);	
	parse(cc, j, temp);	
	j += strlen(temp) + 1;
	Object.isAlive = atoi(temp);
	parse(cc, j, temp);	
	j += strlen(temp) + 1;
	Object.weaponState = atoi(temp);
	parse(cc, j, temp);	
	j += strlen(temp) + 1;
	Object.maxDmg = atoi(temp);
	parse(cc, j, temp);	
	j += strlen(temp) + 1;
	Object.minDmg = atoi(temp);
	parse(cc, j, temp);	
	j += strlen(temp) + 1;
	Object.capacity = atoi(temp);
	parse(cc, j, temp);
	j += strlen(temp) + 1;
	Object.name = malloc((1+strlen(temp))*sizeof(char));
	strcpy(Object.name, temp);
	parse(cc, j, temp);
	j += strlen(temp) + 1;
	Object.descr = malloc((1+strlen(temp))*sizeof(char));
	strcpy(Object.descr, temp);
	parse(cc, j, temp);
	j += strlen(temp) + 1;
	Object.gender = malloc((1+strlen(temp))*sizeof(char));
	strcpy(Object.gender, temp);		
	parse(cc, j, temp);
	j += strlen(temp) + 1;
	Object.effect = malloc((1+strlen(temp))*sizeof(char));
	strcpy(Object.effect, temp);	
	parse(cc, j, temp);
	j += strlen(temp) + 1;
	Object.effect2 = malloc((1+strlen(temp))*sizeof(char));
	strcpy(Object.effect2, temp);	
	parse(cc, j, temp);
	j += strlen(temp) + 1;
	Object.sig = malloc((1+strlen(temp))*sizeof(char));
	strcpy(Object.sig, temp);	
	parse(cc, j, temp);
	j += strlen(temp) + 1;
	Object.previousDot = malloc((1+strlen(temp))*sizeof(char));
	strcpy(Object.previousDot, temp);
	parse(cc, j, temp);
	j += strlen(temp) + 1;
	Object.exp = atoi(temp);	
	parse(cc, j, temp);
	j += strlen(temp) + 1;
	Object.id = atoi(temp);		
	return Object;
}

item loadItem(char* cc)
{
	item Item;
	int j = 0;
	char temp[250];
	parse(cc, j, temp);
	j += strlen(temp) + 1;
	Item.name = malloc((1+strlen(temp))*sizeof(char));
	strcpy(Item.name, temp);
	parse(cc, j, temp);
	j += strlen(temp) + 1;
	Item.descr = malloc((1+strlen(temp))*sizeof(char));
	strcpy(Item.descr, temp);
	parse(cc, j, temp);	
	j += strlen(temp) + 1;
	Item.sig = malloc((1+strlen(temp))*sizeof(char));
	strcpy(Item.sig, temp);				
	parse(cc, j, temp);	
	j += strlen(temp) + 1;
	Item.ego = malloc((1+strlen(temp))*sizeof(char));
	strcpy(Item.ego, temp);
	parse(cc, j, temp);	
	j += strlen(temp) + 1;
	Item.egoDmg = atoi(temp);			
	parse(cc, j, temp);	
	j += strlen(temp) + 1;
	Item.weight = atoi(temp);	
	parse(cc, j, temp);	
	j += strlen(temp) + 1;
	Item.type = atoi(temp);
	parse(cc, j, temp);	
	j += strlen(temp) + 1;
	Item.damage = atoi(temp);	
	parse(cc, j, temp);	
	j += strlen(temp) + 1;
	Item.bonusDmg = atoi(temp);	
	return Item;
}

void parse(char* cc, int j, char temp[250])
{
	sprintf(temp, "%s", "");
	while (cc[j] != '|')
	{
		sprintf(temp, "%s%c", temp, cc[j]);
		j++;
	}
}

void scan(char cc[2500])
{
	int c = fgetc(f);
	int k = 0;
	if (c == EOF) //broken save check
		return;
	while (c != 126)
	{
		if (c == EOF) //broken save check
			return;				
		cc[k] = c;
		k++;
		c = fgetc(f);
	}
	cc[k] = '\0';	
}

void ciph(char* a)
{
	int len = strlen(a);	
	for (int i = 0; i < len; i++)
		if (a[i] == 'k')
			fputc('~'^'k', f);
		else if (a[i] == 'q')
			fputc('`'^'k', f);
		else
			fputc(a[i]^'k', f);
	fputc(126, f);
}

char* deciph(char* c)
{
	int len = strlen(c);
	char* a = malloc(sizeof(char)*len+1);
	for (int i = 0; i < len; i++)
	{
		a[i] = c[i]^'k';
		if (a[i] == '~') 
			a[i] = 'k'; 
		else if (a[i] == '`') 
			a[i] = 'q';
	}
	a[len] = '\0';
	return a;
}

void freeing()
{
	for (int n = 0; n < 49; n++)
	{
		for (int i = 0; i < Room[n].DIMENSION_I; i++) //freeing room drop
			for (int j = 0; j < Room[n].DIMENSION_J; j++)				
			{
				node* cursor = Room[n].drop[i][j]; //freeing the dot of Room[n].grid
				while(cursor != NULL)
				{
					node* temp = cursor;
					cursor = cursor->next;
					free(temp);
				}
				Room[n].drop[i][j] = NULL;
			}
	}	
}

void iniKeys()
{
	struct meta //dungeon layouts
	{
		int i, j, sKey; //metaroom coordinates and self key
		int keys[6]; //right/left/down/up/down lvl/up lvl - keys to other rooms, -2 if no key
	};
	struct meta Rooms[49];
	int lvl[7][7][7];
	for (int ii = 0; ii < 49; ii += 7)
	{
		for (int i = 0; i < 7; i++)
			for (int j = 0; j < 7; j++)
				lvl[i][j][ii/7] = -1;
		int i = rand()%7, j = rand()%7;
		lvl[i][j][ii/7] = ii;
		Rooms[ii].i = i; Rooms[ii].j = j; Rooms[ii].sKey = ii;
		if (j+1 <= 3)
			lvl[i][j+1][ii/7] = -9;
		if (j- 1 >= 0)		
			lvl[i][j-1][ii/7] = -9;
		if (i+1 <= 3)
			lvl[i+1][j][ii/7] = -9;
		if (i- 1 >= 0)
			lvl[i- 1][j][ii/7] = -9;
		for (int k = 1; k < 7; k++)
		{
			while (lvl[i][j][ii/7] != -9)
			{
				i = rand()%7;
				j = rand()%7;
			}
			Rooms[k+ii].i = i; Rooms[k+ii].j = j; Rooms[k+ii].sKey = k + ii;
			lvl[i][j][ii/7] = k + ii;
			if (j+1 <= 3) //to avoid possible out of range value at dot cheking
				if (lvl[i][j+1][ii/7] == -1)
					lvl[i][j+1][ii/7] = -9;
			if (j- 1 >= 0) 
				if (lvl[i][j- 1][ii/7] == -1)		
					lvl[i][j- 1][ii/7] = -9;
			if (i+1 <= 3) 
				if (lvl[i+1][j][ii/7] == -1)
					lvl[i+1][j][ii/7] = -9;
			if (i- 1 >= 0) 
				if (lvl[i- 1][j][ii/7] == -1)
					lvl[i- 1][j][ii/7]= -9;
		}
		for (int i = 0; i < 7; i++)
			for (int j = 0; j < 6; j++)	
				Rooms[i+ii].keys[j] = -2;
		for (int h = 0, ind; h < 7; h++)
		{
			for (ind = 0; ind < 7; ind++) //right
				if (Rooms[ind+ii].i == Rooms[h+ii].i && Rooms[ind+ii].j == Rooms[h+ii].j+1)
					break;
			if (ind != 7)
				Rooms[h+ii].keys[0] = Rooms[ind+ii].sKey;
				
			for (ind = 0; ind < 7; ind++) //left
				if (Rooms[ind+ii].i == Rooms[h+ii].i && Rooms[ind+ii].j == Rooms[h+ii].j-1)
					break;						
			if (ind != 7)
				Rooms[h+ii].keys[1] = Rooms[ind+ii].sKey;
				
			for (ind = 0; ind < 7; ind++) //down
				if (Rooms[ind+ii].i == Rooms[h+ii].i+1 && Rooms[ind+ii].j == Rooms[h+ii].j)
					break;							
			if (ind != 7)
				Rooms[h+ii].keys[2] = Rooms[ind+ii].sKey;
			for (ind = 0; ind < 7; ind++) //up
				if (Rooms[ind+ii].i == Rooms[h+ii].i-1 && Rooms[ind+ii].j == Rooms[h+ii].j)
					break;				
			if (ind != 7)
				Rooms[h+ii].keys[3] = Rooms[ind+ii].sKey;
		}
		int roll = rand()%6; //marking rooms with down stairs
		if (ii != 42) //no down stairs at the bottom
		{
			Rooms[roll+ii].keys[4] = 50;
			Rooms[roll+1+ii].keys[4] = 50;
		}
		if (ii == 0) //marking rooms with up stairs
		{
			Rooms[0+ii].keys[5] = 50;
			Rooms[1+ii].keys[5] = 50;
		}
		else
		{
			roll = rand()%6;
			Rooms[roll+ii].keys[5] = 50;
			Rooms[roll+1+ii].keys[5] = 50;
		}
	}
	for (int ii = 0, a, b, c, d; ii < 42; ii += 7) //connecting levels
	{
		for (int i = 0; i < 7; i++)
			if (Rooms[i+ii].keys[4] == 50)
			{
				a = Rooms[i+ii].sKey;
				b = Rooms[i+1+ii].sKey;
				break;
			}
		for (int i = 0; i < 7; i++)
			if (Rooms[i+ii+7].keys[5] == 50)
			{
				c = Rooms[i+ii+7].sKey;
				d = Rooms[i+1+ii+7].sKey;
				break;
			}
		Rooms[a].keys[4] = c;
		Rooms[b].keys[4] = d;
		Rooms[c].keys[5] = a;
		Rooms[d].keys[5] = b;
	}
	for (int i = 0; i < 49; i++)
		for (int j = 0; j < 6; j++)
		{
			Room[i].keys[j][0] = Rooms[i].keys[j]; //exit key (number of the room where the exit leads)
			Room[i].keys[j][1] = -1; //exit coordinates, i
			Room[i].keys[j][2] = -1; //j
		}
}

void iniRoom(int n)
{
	int roll = rand()%7;
	if (roll <= 1)
	{
		Room[n].DIMENSION_I = 16 + rand()%3;
		Room[n].DIMENSION_J = 42 + rand()%35;
	}
	else if (roll > 1 && roll <= 4)
	{
		Room[n].DIMENSION_I = 12 + rand()%7;
		Room[n].DIMENSION_J = 18 + rand()%24;
	}
	else if (roll >= 5)
	{
		Room[n].DIMENSION_I = 9 + rand()%4;
		Room[n].DIMENSION_J = 9 + rand()%9;
	}
	if (n != 48) //different descriptions
	{
		roll = rand()%5;
		if (roll == 0)
			Room[n].descr = "You are in a dark and wet dungeon.";
		else if (roll == 1)
			Room[n].descr = "You are in a dim-lit cave.";
		else if (roll == 2)
			Room[n].descr = "You are in an abandoned mine.";
		else if (roll == 3)
			Room[n].descr = "You are in a musty cave.";
		else if (roll == 4)
			Room[n].descr = "You are in a foggy dungeon.";
	}
	else
		Room[n].descr = "You are in the dragon's den!";
	for (int i = 0; i < Room[n].DIMENSION_I; i++)
		for (int j = 0; j < Room[n].DIMENSION_J; j++)
			Room[n].grid[i][j] = YELLOW"."RESET;
	iniWall(n); //wall, then exits, traps, enemy initialization
	iniExits(n); //doors & stairs
	if (Room[n].DIMENSION_J < 18) //traps
		Room[n].numTraps = rand()%3 + 1; //1-3 traps
	else if (Room[n].DIMENSION_J < 42)
		Room[n].numTraps = rand()%4 + 2; //2-5 traps
	else
		Room[n].numTraps = rand()%5 + 4; //4-8 traps
	for (int i = 0; i < Room[n].numTraps; i++)
		Room[n].trap[i] = iniTrap(n);
	if (Room[n].DIMENSION_J < 18) //enemy
		Room[n].numEnemy = rand()%2 + 1; //1-2 enemies
	else if (Room[n].DIMENSION_J < 42)
		Room[n].numEnemy = rand()%2 + 2; //2-3 enemies
	else
		Room[n].numEnemy = rand()%4 + 4; //4-7 enemies
	for (int i =0; i < Room[n].numEnemy; i++)
		if (rand()%2 == 0)
			Room[n].enemy[i] = iniMeleeEnemy(n);
		else
			Room[n].enemy[i] = iniWeaponlessEnemy(n);
	if (n == 48)
	{
		Room[n].enemy[Room[n].numEnemy] = iniDragon();
		Room[n].numEnemy++;
	}
	Room[n].numAliveEnemy = Room[n].numEnemy;
	int num_items; //items on floor
	if (Room[n].DIMENSION_J < 18)
		num_items = rand()%2; //0-1 items
	else if (Room[n].DIMENSION_J < 42)
		num_items = rand()%4; //0-3 items
	else
		num_items = rand()%4 + 1; //1-4 items
	for (int i = 0; i < num_items; i++)
		iniItem(n);
	for (int i = 0; i < Room[n].numTraps; i++) //after ini
		Room[n].grid[Room[n].trap[i].i][Room[n].trap[i].j] = YELLOW"."RESET; //to wipe traps from board where they were for ini checking	
}

void doorStep() //right/left/down/up
{
	if (user.j == Room[current].DIMENSION_J-1) //right move
	{
		Room[current].grid[user.i][user.j] = user.previousDot;
		current = Room[current].keys[0][0];
		user.i = Room[current].keys[1][1]; user.j = Room[current].keys[1][2];
		user.previousDot = Room[current].grid[user.i][user.j];
		Room[current].grid[user.i][user.j] = user.sig;
		clrscr();		
	}
	else if (user.j == 0) //left move
	{
		Room[current].grid[user.i][user.j] = user.previousDot;
		current = Room[current].keys[1][0];
		user.i = Room[current].keys[0][1]; user.j = Room[current].keys[0][2];
		user.previousDot = Room[current].grid[user.i][user.j];
		Room[current].grid[user.i][user.j] = user.sig;
		clrscr();		
	}
	else if (user.i == Room[current].DIMENSION_I-1) //down move
	{
		Room[current].grid[user.i][user.j] = user.previousDot;
		current = Room[current].keys[2][0];
		user.i = Room[current].keys[3][1]; user.j = Room[current].keys[3][2];
		user.previousDot = Room[current].grid[user.i][user.j];
		Room[current].grid[user.i][user.j] = user.sig;
		clrscr();		
	}
	else if (user.i == 0) //up move
	{
		Room[current].grid[user.i][user.j] = user.previousDot;
		current = Room[current].keys[3][0];
		user.i = Room[current].keys[2][1]; user.j = Room[current].keys[2][2];
		user.previousDot = Room[current].grid[user.i][user.j];
		Room[current].grid[user.i][user.j] = user.sig;
		clrscr();		
	}
}

object iniDragon()
{
	object enemy;
	enemy.weaponState = 0;
	enemy.hp = enemy.maxhp = simple_melee[32].maxhp; enemy.isAlive = 1;
	enemy.inventory[0] = zero_item;
	enemy.inventory[0].name = malloc((1+strlen(simple_melee[32].effect))*sizeof(char));
	strcpy(enemy.inventory[0].name, simple_melee[32].effect); //not a real item, nothing but name!
	if (rand()%3 == 1)
		enemy.inventory[1] = consumables[0]; //maybe it will be useful later
	else
		enemy.inventory[1] = zero_item;
	enemy.inventory[2] = zero_item;
	enemy.descr = simple_melee[32].descr; enemy.name = simple_melee[32].name; enemy.gender = simple_melee[32].gender;
	enemy.minDmg = simple_melee[32].minDmg; enemy.maxDmg = simple_melee[32].maxDmg; enemy.sig = simple_melee[32].sig;
	enemy.previousDot = YELLOW"."RESET; enemy.exp = simple_melee[32].exp; enemy.id = simple_melee[32].id;
	for (int i = 0; i < 2; i++)
	{
		enemy.condition[i] = 0;
		enemy.duration[i] = 0;
	}
	while (1)
	{
		enemy.i = 1 + rand()%(Room[48].DIMENSION_I - 2);
		enemy.j = 1 + rand()%(Room[48].DIMENSION_J - 2);
		if (strcmp(Room[48].grid[enemy.i][enemy.j], YELLOW"."RESET) == 0)
		{
			Room[48].grid[enemy.i][enemy.j] = enemy.sig;
			break;
		}
	}
	return enemy;
}

void enemyDeath(int k, char* messg)
{
	Room[current].enemy[k].isAlive = 0;
	if (strcmp(Room[current].enemy[k].name, "cobalt dragon") == 0)
	{
		sprintf(Room[current].messg, "%s You have won!\nThe glory is yours and so are the treasures, if you can find them.", messg);
		gameOver(Room[current].messg);
		user.isAlive = 0;
	}
	Room[current].numAliveEnemy--;
	for (int i = 0; i < E_INVENTORY; i++)
		if (strcmp(Room[current].enemy[k].inventory[i].descr, "empty") != 0) //careful about weaponless ones
			dropItem(Room[current].enemy[k].i, Room[current].enemy[k].j, Room[current].enemy[k].inventory[i], current);
	if (Room[current].enemy[k].id != 29) //nightmars leave no corpse
	{
		item corpse = zero_item;
		char line[200];
		if (Room[current].enemy[k].id == 4 || Room[current].enemy[k].id == 12 || Room[current].enemy[k].id == 20 || Room[current].enemy[k].id == 28)
		{ //clockwork ones
			sprintf(line, "A broken hull of %s.", Room[current].enemy[k].name);
			corpse.descr = malloc(sizeof(char)*(strlen(line)+1));
			strcpy(corpse.descr, line);
			sprintf(line, "hull of %s", Room[current].enemy[k].name);
			corpse.name = malloc(sizeof(char)*(strlen(line)+1));
			strcpy(corpse.name, line);		
		}
		else if (Room[current].enemy[k].id == 2)
		{ //skeleton
			sprintf(line, "A pile of bones lies here.");
			corpse.descr = malloc(sizeof(char)*(strlen(line)+1));
			strcpy(corpse.descr, line);
			sprintf(line, "pile of bones");
			corpse.name = malloc(sizeof(char)*(strlen(line)+1));
			strcpy(corpse.name, line);		
		}
		else
		{ //ordinary corpses
			sprintf(line, "A stinky corpse of %s. Eww.", Room[current].enemy[k].name);
			corpse.descr = malloc(sizeof(char)*(strlen(line)+1));
			strcpy(corpse.descr, line);
			sprintf(line, "corpse of %s", Room[current].enemy[k].name);
			corpse.name = malloc(sizeof(char)*(strlen(line)+1));
			strcpy(corpse.name, line);
		}
		corpse.sig = WHITE"%"RESET;
		corpse.weight = 500; //no reason to make different weights yet
		corpse.type = 300; //same
		dropItem(Room[current].enemy[k].i, Room[current].enemy[k].j, corpse, current);
	}
	if (strcmp(Room[current].enemy[k].previousDot, B_WHITE"<") != 0 && strcmp(Room[current].enemy[k].previousDot, B_WHITE">") != 0 && Room[current].drop[Room[current].enemy[k].i][Room[current].enemy[k].j] != NULL) //not to wipe stairs accidentally
		Room[current].grid[Room[current].enemy[k].i][Room[current].enemy[k].j] = Room[current].drop[Room[current].enemy[k].i][Room[current].enemy[k].j]->Item.sig;
	else
		Room[current].grid[Room[current].enemy[k].i][Room[current].enemy[k].j] = Room[current].enemy[k].previousDot;	
}

void prompt()
{
	int x;
	if (user.exp < 100) x = 100 - user.exp;
	else if (user.exp < 300) x = 300 - user.exp;
	else if (user.exp < 600) x = 600 - user.exp;
	else if (user.exp < 1000) x = 1000 - user.exp;
	else if (user.exp < 1500) x = 1500 - user.exp;
	else if (user.exp < 2100) x = 2100 - user.exp;
	else x = 9999;
	if (x  != 9999) //a bit clumsy but w/e, it's more clear
		sprintf(Room[current].prompt, B_RED"%d/%d"RESET""B_WHITE"hp "B_YELLOW"%d-%d"RESET""B_WHITE"dmg "B_MAGENTA"%d"RESET""B_WHITE"flr "CYAN"%d"RESET""B_WHITE"exp |%s"WHITE, user.hp, user.maxhp, user.minDmg, user.maxDmg, (current/7+1), x, user.inventory[0].name);
	else
		sprintf(Room[current].prompt, B_RED"%d/%d"RESET""B_WHITE"hp "B_YELLOW"%d-%d"RESET""B_WHITE"dmg "B_MAGENTA"%d"RESET""B_WHITE"flr "CYAN"lvl cap"RESET""B_WHITE" |%s"WHITE, user.hp, user.maxhp, user.minDmg, user.maxDmg, (current/7+1), user.inventory[0].name);		
}

void hittingEnemy(int k)
{
	char* strike = rand()%5 > 0 ? (rand()%2 == 0 ? "hit" : "strike") : "jab";
	sprintf(Room[current].user_messg, "You %s %s with %s.", strike, Room[current].enemy[k].name, user.inventory[0].name);
	Room[current].enemy[k] = attack(Room[current].enemy[k], user);
	if (strcmp(user.inventory[0].ego, "venom") != 0)
		egoMessg(k);
	if ((strcmp(user.inventory[0].ego, "venom") == 0) && (Room[current].enemy[k].id != 4 && Room[current].enemy[k].id != 12 && Room[current].enemy[k].id != 20 && Room[current].enemy[k].id != 28 && 
	Room[current].enemy[k].id != 2 && Room[current].enemy[k].id != 10 && Room[current].enemy[k].id != 18 && Room[current].enemy[k].id != 25 && Room[current].enemy[k].id != 29))
	{//you cannot poison constructs and undead
		egoMessg(k);
		Room[current].enemy[k].duration[0] += 3;
		if (Room[current].enemy[k].condition[0] < user.inventory[0].egoDmg) //only the strongest poison applies, but duration sums for all poisons
			Room[current].enemy[k].condition[0] = user.inventory[0].egoDmg;
	}
	plog(Room[current].user_messg);
}

int getYesNo()
{
	while (1)
	{
		struct termios newt, oldt;
		tcgetattr(STDIN_FILENO, &oldt);
		newt = oldt;
		newt.c_lflag &= ~(ICANON | ECHO);
		tcsetattr(STDIN_FILENO, TCSANOW, &newt); //enable raw mode
		int ch = 0, input = 0; //input is set but not used intentionally
		ch = getchar();
		if (ch == 126)
			ch = getchar();
		if (ch == 27)
			ch = getchar();
		if (ch == '[')
			input = getchar();
		tcsetattr(STDIN_FILENO, TCSANOW, &oldt); //return old config
		if (ch == 'y' || ch == 'Y' || ch == 'n' || ch == 'N')
			return ch;
		sprintf(Room[current].user_messg, "[Y]es or [N]o only, please.");
		plog(Room[current].user_messg);
		draw();
	}
	return 1;
}

void levelUp()
{
	if (user.maxhp == 20 && user.exp >= 100) //level up the hero
	{
		user.hp = user.maxhp = 22;
		user.maxDmg += 1; //0-1 at lvl 1, 0-2 now at lvl 2
		sprintf(Room[current].messg, "Congratulations, you have achieved the second level!");
		plog(Room[current].messg);
		Log.messg_count++;
		Log.thisTurnInd[Log.messg_count-1] = Log.index;
	}
	else if (user.maxhp == 22 && user.exp >= 300) //100+200
	{
		user.hp = user.maxhp = 26;
		user.minDmg += 1; //0-2 at lvl 2, 1-2 now at lvl 3
		sprintf(Room[current].messg, "Congratulations, you have achieved the third level!");
		plog(Room[current].messg);
		Log.messg_count++;
		Log.thisTurnInd[Log.messg_count-1] = Log.index;
	}		
	else if (user.maxhp == 26 && user.exp >= 600) //100+200+300
	{
		user.hp = user.maxhp = 32;
		user.maxDmg += 1; //1-2 at lvl 3, 1-3 now at lvl 4
		sprintf(Room[current].messg, "Congratulations, you have achieved the fourth level!");
		plog(Room[current].messg);
		Log.messg_count++;
		Log.thisTurnInd[Log.messg_count-1] = Log.index;
	}		
	else if (user.maxhp == 32 && user.exp >= 1000) //100+200+300+400
	{
		user.hp = user.maxhp = 40;
		user.minDmg += 1; //1-3 at lvl 4, 2-3 now at lvl 5
		sprintf(Room[current].messg, "Congratulations, you have achieved the fifth level!");
		plog(Room[current].messg);
		Log.messg_count++;
		Log.thisTurnInd[Log.messg_count-1] = Log.index;
	}		
	else if (user.maxhp == 40 && user.exp >= 1500) //100+200+300+400+500
	{
		user.hp = user.maxhp = 50;
		user.maxDmg += 1; //2-3 at lvl 5, 2-4 now at lvl 6
		sprintf(Room[current].messg, "Congratulations, you have achieved the sixth level!");
		plog(Room[current].messg);
		Log.messg_count++;
		Log.thisTurnInd[Log.messg_count-1] = Log.index;
	}		
	else if (user.maxhp == 50 && user.exp >= 2100) //100+200+300+400+500+600
	{
		user.hp = user.maxhp = 62;
		user.minDmg += 1; //2-4 at lvl 6, 3-4 now at lvl 7
		sprintf(Room[current].messg, "Congratulations, you have achieved the seventh level! Surprisingly.");
		plog(Room[current].messg);
		Log.messg_count++;
		Log.thisTurnInd[Log.messg_count-1] = Log.index;
	}
}

void conditionCheck()
{
	if (user.condition[0] != 0) //suffer from poison
	{
		user.hp -= user.condition[0];
		sprintf(Room[current].messg, "You feel sick.");
		if (user.hp <= 0)
		{
			sprintf(Room[current].messg, "Alas! You have succumbed to the poison and died.");
			plog(Room[current].messg);
			Log.messg_count++;
			Log.thisTurnInd[Log.messg_count-1] = Log.index;
			gameOver(Room[current].messg);
			user.isAlive = 0;
			return;
		}
		user.duration[0] -= 1;
		if (user.duration[0] == 0)
		{
			user.condition[0] = 0;
			sprintf(Room[current].messg, "%s You have overcome the poison!", Room[current].messg);
		}
		plog(Room[current].messg);
		Log.messg_count++;
		Log.thisTurnInd[Log.messg_count-1] = Log.index;
	}
	else if (user.condition[1] != 0) //empowered
	{
		user.duration[1] -= 1;
		if (user.duration[1] == 0)
		{
			sprintf(Room[current].messg, "You feel less mighty.");
			user.maxDmg -= user.condition[1];
			user.minDmg -= user.condition[1];
			user.condition[1] = 0;
			plog(Room[current].messg);
			Log.messg_count++;
			Log.thisTurnInd[Log.messg_count-1] = Log.index;
		}
	}
}

int differentMoves(int user_move)
{
	if (user_move == 'p') //show log
	{
		clrscr();
		if (Log.index == 19)
			for (int i = 0; i < 20; i++)
				printf("%s\n", Log.log[i]);
		else if (strcmp(Log.log[Log.index+1], " ") == 0)
			for (int i = 0; i < (Log.index + 1); i++)
				printf("%s\n", Log.log[i]);
		else
		{
			for (int i = Log.index+1; i < 20; i++)
				printf("%s\n", Log.log[i]);
			for (int i = 0; i < (Log.index + 1); i++)
				printf("%s\n", Log.log[i]);
		}
		printf("\nPress any key to continue.");
		getVoidInput();
		clrscr();
	}
	if (user_move == 'Q')
	{
		sprintf(Room[current].user_messg, "Are you sure you want to commit suicide? Y/N");
		plog(Room[current].user_messg);
		draw();
		int choice = getYesNo();
		if (choice == 'y' || choice == 'Y')
		{			
			sprintf(Room[current].user_messg, "Exhausted with anguish and fear, you took your own life.");
			plog(Room[current].user_messg);
			gameOver(Room[current].user_messg);
			return 0;
		}
		else
		{
			sprintf(Room[current].user_messg, "Okay then.");
			plog(Room[current].user_messg);
		}
	}
	if (user_move == 'S')
	{
		sprintf(Room[current].user_messg, "See you later!");
		plog(Room[current].user_messg);
		gameOver(Room[current].user_messg);
		save();
		freeing();
		return 0;
	}
	if (user_move == 's')
	{
		sprintf(Room[current].user_messg, "Saving...");
		plog(Room[current].user_messg);
		save();
	}
	return 1;	
}

void placeRelatedMsg()
{
	if (strcmp(user.previousDot, B_WHITE"<") == 0 && current < 7 && Room[current].drop[user.i][user.j] == NULL) //creating place-related messages
	{
		sprintf(Room[current].messg, "There is an exit from the dungeon here.");
		plog(Room[current].messg);
		Log.messg_count++;
		Log.thisTurnInd[Log.messg_count-1] = Log.index;
	}
	else if (strcmp(user.previousDot, B_WHITE"<") == 0 && current >= 7 && Room[current].drop[user.i][user.j] == NULL)
	{
		sprintf(Room[current].messg, "There is a stairs to the previous level here.");
		plog(Room[current].messg);
		Log.messg_count++;
		Log.thisTurnInd[Log.messg_count-1] = Log.index;
	}
	else if (strcmp(user.previousDot, B_WHITE">") == 0 && Room[current].drop[user.i][user.j] == NULL)
	{
		sprintf(Room[current].messg, "There is a stairs to the next level here.");
		plog(Room[current].messg);
		Log.messg_count++;
		Log.thisTurnInd[Log.messg_count-1] = Log.index;
	}
	else if (strcmp(user.previousDot, WHITE"/"RESET) == 0 && Room[current].drop[user.i][user.j] == NULL)
	{
		sprintf(Room[current].messg, "You are on a door step to another room.");
		plog(Room[current].messg);
		Log.messg_count++;
		Log.thisTurnInd[Log.messg_count-1] = Log.index;
	}
	else if ((strcmp(user.previousDot, YELLOW"."RESET) != 0) &&
	(strcmp(user.previousDot, YELLOW"^"RESET) != 0) &&
	(strcmp(user.previousDot, CYAN"^"RESET) != 0) &&
	(strcmp(user.previousDot, B_GREEN"^"RESET) != 0))//not a dot or trap (they have their own messgs) i.e. a corpse or an item
	{
		if (Room[current].drop[user.i][user.j]->next == NULL)
			sprintf(Room[current].messg, "%s", Room[current].drop[user.i][user.j]->Item.descr);
		else
			sprintf(Room[current].messg, "%s [stack]", Room[current].drop[user.i][user.j]->Item.descr);
		plog(Room[current].messg);
		Log.messg_count++;
		Log.thisTurnInd[Log.messg_count-1] = Log.index;
	}
}
