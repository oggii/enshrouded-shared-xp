# Changelog

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
