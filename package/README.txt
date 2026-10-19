Enshrouded Shared XP 1.0.0
==========================

Server-side mod for Enshrouded dedicated servers. When one player earns non-combat XP
(exploration, books, quests, mining, item sets), every other online player gets the same
amount, at any distance. Players need nothing; only the server gets the mod.

Works with server build 25988414 (Enshrouded 1.0, October 2026). On other builds the mod
stays inactive and the server runs as usual.

WINDOWS SERVER
1. Stop the server.
2. Copy powrprof.dll and xpshare.ini next to enshrouded_server.exe.
3. Start the server.

LINUX / DOCKER (WINE)
1. Stop the container.
2. Copy powrprof.dll and xpshare.ini next to enshrouded_server.exe.
3. Set the environment variable WINEDLLOVERRIDES=powrprof=n,b for the container.
4. Start the container.

CHECK
xpshare.log next to the server exe should end with
"active: non-combat XP is shared with every online player".
Set LogEveryShare=1 in xpshare.ini to log every share.

UNINSTALL
Stop the server and delete powrprof.dll, xpshare.ini and xpshare.log.

Settings, details and source code: https://github.com/oggii/enshrouded-shared-xp
