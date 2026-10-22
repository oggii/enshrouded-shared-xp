# Changelog

## 1.1.2 (2026-10-22)

- Chat line on join: when a player joins, the server chat shows
  "Shared XP 1.1.2 is active: non-combat XP is shared with everyone online." (or "all XP"
  with `ShareCombatXp=1`). No line after joining means the mod is off, for example after a
  game update your host installed. Asked for on Nexus by a player on a rented server.
- `AnnounceInChat` (default 1): set 0 to turn the line off.
- The line goes to everyone online, once per player who joins. It needs text chat to be
  enabled on the server; with text chat off the mod logs that and stays quiet.
- If the chat sites are not found, only the line is off; XP sharing works as before.
- The selftest checks the two new sites in both server builds.

## 1.1.1 (2026-10-21)

- Works on server build 26083076 (the 20 October update) as well as 25988414. Three hook
  patterns contained call addresses that moved with the update; they now use wildcards, and
  the add helper is taken from the call the two XP systems make. The log names the build.
- The selftest knows the hook sites of both builds.

## 1.1.0 (2026-10-19)

- `ShareCombatXp` (default 0): combat XP is shared too. Players far from a kill get the XP the
  players at the kill got (the largest amount, once per kill); players at the kill get nothing
  extra. Asked for on Nexus, so a player who mines does not fall behind a friend who fights.
- The selftest also checks the combat hook site.

## 1.0.0 (2026-10-19)

First public release, for Enshrouded 1.0 server build 25988414.

- Shares exploration, lore, quest, mining and item-set XP with every online player, at any
  distance. Combat XP is left to the game.
- `Multiplier` for the shared amount, logging switches, solo test mode.
- `DoubleQuestXp`: quests finished together give the quest XP once (default) or also the
  other players' share.
- Inactive on any other server build; the server then runs unmodded.
- `xpshare_selftest.exe` checks a server exe on disk.
