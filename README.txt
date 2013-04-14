In order to compile the game for Linux, you need to download the source
file drawt.c and to use the following command:

gcc drawt.c -o drawt -std=c99

The game has no dependencies and should work on every 80x24 terminal 
which supports ANSI color codes. 

In order to compile the game for Windows, you should use cygwin with 
the same command as above. 
Windows binary (when it's used outside of cygwin) needs cygwin1.dll to 
be placed in the game folder to be able to work. The game is licensed 
under GNU/GPL so cygwin's license allows that. The archive with 
windows binary consists cygwin1.dll.

The goal of the game is to slay the dragon which awaits you on the 
deepest level. Your character has no natural health regeneration and 
your only means to survive is healing potions which you can find 
everywhere. As any roguelike, this game has random level layouts,
permadeath, randomized weapon, many kinds of monsters, several kinds of
consumables and traps. Since it's alpha, different races, classes, 
skills, magic (apart from consumables) and ranged weapon aren't 
implemented yet, your character can only use melee weapon or bare 
hands. The turn system is quite straightforward when every your action 
takes one turn and gives the turn to enemy. Sight system is basic, 
every room is considered to be fully lit, monster's pathfinding is 
basic as well, but the ability to cheat mobs easily can be considered 
as an element of balance.

The controls are following:

Movement:
Arrows, num pad or vi keys.

 7 8 9               y k u               > - move downstairs
  \|/                 \|/                < - move upstairs
 4-5-6 - num pad     h-.-l - vi keys     . or 5 - pass a turn 
  /|\                 /|\
 1 2 3               b j n

Commands:
g - pick item       i - show inventory & equipment
d - drop item       p - show log of recent messages
w - wield weapon    s - save
r - read scroll     S - save&quit
q - quaff potion    Q - quit the game (commit suicide)

(c) Evgeny Dergachev, ead1@gmx.us
