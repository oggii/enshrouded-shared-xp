// Plain assert-based tests for the join watch (no framework, runs on the build PC).
#include <cstdio>
#include <cstdlib>

#include "../src/join_watch.h"

using xpshare::JoinWatch;
using xpshare::kPlayerSlots;
using xpshare::SlotState;
using Slots = std::array<SlotState, kPlayerSlots>;

static int failures = 0;

#define EXPECT(cond)                                                                              \
    do {                                                                                          \
        if (!(cond)) { std::printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond); ++failures; } \
    } while (0)

static void players_already_in_game_are_not_greeted() {
    JoinWatch w(8000);
    Slots s{};
    s[0] = {64, 1, 0xabc};
    w.update(s, 0);
    EXPECT(w.pending() == 0);
    EXPECT(w.take_due(s, 100000).empty());
}

static void a_join_is_announced_after_the_delay() {
    JoinWatch w(8000);
    Slots s{};
    w.update(s, 0);
    s[2] = {65, 7, 0xdef};
    w.update(s, 1000);
    EXPECT(w.pending() == 1);
    EXPECT(w.take_due(s, 8999).empty());
    auto due = w.take_due(s, 9000);
    EXPECT(due.size() == 1 && due[0].handle == 65 && due[0].account == 0xdef);
    EXPECT(w.pending() == 0);
}

static void a_player_who_left_again_is_not_announced() {
    JoinWatch w(8000);
    Slots s{};
    w.update(s, 0);
    s[1] = {66, 3, 1};
    w.update(s, 1000);
    s[1] = {};
    w.update(s, 2000);
    EXPECT(w.take_due(s, 9000).empty());
    EXPECT(w.pending() == 0);
}

static void loading_screen_then_in_game_counts_once() {
    JoinWatch w(8000);
    Slots s{};
    w.update(s, 0);
    s[0] = {64, 0, 1};  // connected, entity not yet there
    w.update(s, 500);
    EXPECT(w.pending() == 0);
    s[0].entity = 1;
    w.update(s, 1000);
    w.update(s, 1250);
    EXPECT(w.pending() == 1);
}

static void rejoin_with_a_new_entity_is_a_new_join() {
    JoinWatch w(8000);
    Slots s{};
    s[0] = {64, 1, 1};
    w.update(s, 0);
    s[0] = {64, 2, 1};  // same slot, new entity after a reconnect between two snapshots
    w.update(s, 250);
    EXPECT(w.pending() == 1);
}

int main() {
    players_already_in_game_are_not_greeted();
    a_join_is_announced_after_the_delay();
    a_player_who_left_again_is_not_announced();
    loading_screen_then_in_game_counts_once();
    rejoin_with_a_new_entity_is_a_new_join();
    if (failures == 0) {
        std::printf("join watch tests passed\n");
        return EXIT_SUCCESS;
    }
    std::printf("%d join watch test(s) failed\n", failures);
    return EXIT_FAILURE;
}
