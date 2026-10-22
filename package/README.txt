Enshrouded Shared XP 1.1.3
==========================

Server-side mod for Enshrouded dedicated servers. When one player earns non-combat XP
(exploration, books, quests, mining, item sets), every other online player gets the same
amount, at any distance. Players need nothing; only the server gets the mod.

Works with server builds 25988414 and 26083076 (Enshrouded 1.0, October 2026). On other
builds the mod stays inactive, the server runs as usual and the chat says so when a player
joins.

WINDOWS SERVER
1. Stop the server.
2. Copy powrprof.dll and xpshare.ini next to enshrouded_server.exe.
3. Start the server.

LINUX / DOCKER (WINE)
1. Stop the container.
2. Copy powrprof.dll and xpshare.ini next to enshrouded_server.exe.
3. Set the environment variable WINEDLLOVERRIDES=powrprof=n,b for the container
   (with the Live Map too: WINEDLLOVERRIDES=powrprof=n,b;iphlpapi=n,b).
4. Start the container.

CHECK
Join the server: right after "Player <name> joined." the chat shows
"Shared XP 1.1.3 is active ...". "is NOT active: the server was updated" means a game update
this version does not support yet. xpshare.log next to the server exe says more; set
LogEveryShare=1 in xpshare.ini to log every share. AnnounceInChat=0 turns the chat line off.

WORKS WITH THE LIVE MAP
Both mods add a line after "Player <name> joined." and run side by side.

QUESTS DONE TOGETHER
DoubleQuestXp=0 (default): each player gets the quest XP once.
DoubleQuestXp=1: each player also gets the other players' share (double quest XP).

COMBAT XP
ShareCombatXp=0 (default): left to the game, only players near a kill get XP.
ShareCombatXp=1: players far from a kill get the XP the players at the kill got.

UNINSTALL
Stop the server and delete powrprof.dll, xpshare.ini and xpshare.log.

Settings, details and source code: https://github.com/oggii/enshrouded-shared-xp
