// Plain assert-based tests for the ledger (no framework, runs on the build PC).
#include <cstdio>
#include <cstdlib>

#include "../src/ledger.h"

using xpshare::Ledger;

static int failures = 0;

#define EXPECT_EQ(actual, expected)                                                       \
    do {                                                                                  \
        auto a_ = (actual);                                                               \
        auto e_ = (expected);                                                             \
        if (a_ != e_) {                                                                   \
            std::printf("FAIL %s:%d  %s == %lld, expected %lld\n", __FILE__, __LINE__,    \
                        #actual, static_cast<long long>(a_), static_cast<long long>(e_)); \
            ++failures;                                                                   \
        }                                                                                 \
    } while (0)

constexpr uintptr_t A = 0x1000, B = 0x2000, C = 0x3000;

static void first_visit_gets_no_back_pay() {
    Ledger l;
    l.record(A, 100, 2, 0);
    EXPECT_EQ(l.collect(B, 1.0, 1), 0u);
}

static void other_players_gain_is_shared_once() {
    Ledger l;
    l.collect(A, 1.0, 0);
    l.collect(B, 1.0, 0);
    l.record(A, 120, 2, 1);
    EXPECT_EQ(l.collect(B, 1.0, 2), 120u);
    EXPECT_EQ(l.collect(B, 1.0, 3), 0u);
}

static void own_gain_is_not_returned() {
    Ledger l;
    l.collect(A, 1.0, 0);
    l.record(A, 50, 5, 1);
    EXPECT_EQ(l.collect(A, 1.0, 2), 0u);
}

static void test_mode_returns_own_gain_once() {
    Ledger l;
    l.collect(A, 1.0, 0, true);
    l.record(A, 50, 2, 1);
    EXPECT_EQ(l.collect(A, 1.0, 2, true), 50u);
    EXPECT_EQ(l.collect(A, 1.0, 3, true), 0u);
}

static void combat_and_none_are_never_recorded() {
    Ledger l;
    l.collect(B, 1.0, 0);
    EXPECT_EQ(l.record(A, 300, 4, 1), false);
    EXPECT_EQ(l.record(A, 300, 0, 1), false);
    EXPECT_EQ(l.collect(B, 1.0, 2), 0u);
}

static void every_receiver_gets_its_share() {
    Ledger l;
    l.collect(A, 1.0, 0);
    l.collect(B, 1.0, 0);
    l.collect(C, 1.0, 0);
    l.record(A, 40, 3, 1);
    l.record(B, 10, 1, 1);
    EXPECT_EQ(l.collect(A, 1.0, 2), 10u);
    EXPECT_EQ(l.collect(B, 1.0, 2), 40u);
    EXPECT_EQ(l.collect(C, 1.0, 2), 50u);
}

static void multiplier_is_applied_and_floored() {
    Ledger l;
    l.collect(B, 0.5, 0);
    l.record(A, 25, 2, 1);
    EXPECT_EQ(l.collect(B, 0.5, 2), 12u);
}

static void zero_multiplier_shares_nothing() {
    Ledger l;
    l.collect(B, 0.0, 0);
    l.record(A, 25, 2, 1);
    EXPECT_EQ(l.collect(B, 0.0, 2), 0u);
}

static void prune_drops_old_gains_and_players() {
    Ledger l;
    l.collect(B, 1.0, 0);
    l.record(A, 25, 2, 0);
    l.prune(200'000, 120'000);
    EXPECT_EQ(l.pending_gains(), 0u);
    EXPECT_EQ(l.known_players(), 0u);
}

constexpr uint8_t kQuest = 3, kMining = 5;
constexpr uint64_t kHold = Ledger::kQuestHoldMs;

static void joint_quest_is_double_with_filter_off() {
    Ledger l;
    l.collect(A, 1.0, 0);
    l.collect(B, 1.0, 0);
    l.record(A, 5, kQuest, 1);
    l.record(B, 5, kQuest, 1);
    EXPECT_EQ(l.collect(A, 1.0, 2), 5u);
    EXPECT_EQ(l.collect(B, 1.0, 2), 5u);
}

static void joint_quest_is_skipped_with_filter_on() {
    Ledger l;
    l.set_skip_joint_quests(true);
    l.collect(A, 1.0, 0);
    l.collect(B, 1.0, 0);
    l.record(A, 5, kQuest, 1);
    l.record(B, 5, kQuest, 1);
    uint32_t skipped = 0;
    EXPECT_EQ(l.collect(A, 1.0, 1 + kHold, false, &skipped), 0u);
    EXPECT_EQ(skipped, 5u);
    EXPECT_EQ(l.collect(B, 1.0, 1 + kHold), 0u);
    EXPECT_EQ(l.collect(B, 1.0, 2 + kHold), 0u);
}

static void solo_quest_is_shared_after_the_hold() {
    Ledger l;
    l.set_skip_joint_quests(true);
    l.collect(B, 1.0, 0);
    l.record(A, 5, kQuest, 1);
    EXPECT_EQ(l.collect(B, 1.0, kHold), 0u);  // still held
    EXPECT_EQ(l.collect(B, 1.0, 1 + kHold), 5u);
    EXPECT_EQ(l.collect(B, 1.0, 2 + kHold), 0u);
}

static void receivers_late_quest_gain_still_counts_as_joint() {
    Ledger l;
    l.set_skip_joint_quests(true);
    l.collect(B, 1.0, 0);
    l.record(A, 5, kQuest, 1);
    EXPECT_EQ(l.collect(B, 1.0, 500), 0u);
    l.record(B, 5, kQuest, 1'000);  // B's own quest XP arrives a second later
    EXPECT_EQ(l.collect(B, 1.0, 1 + kHold), 0u);
    EXPECT_EQ(l.collect(B, 1.0, 1'000 + kHold), 0u);
}

static void different_quest_amounts_are_both_shared() {
    Ledger l;
    l.set_skip_joint_quests(true);
    l.collect(A, 1.0, 0);
    l.collect(B, 1.0, 0);
    l.record(A, 5, kQuest, 1);
    l.record(B, 8, kQuest, 1);
    EXPECT_EQ(l.collect(A, 1.0, 1 + kHold), 8u);
    EXPECT_EQ(l.collect(B, 1.0, 1 + kHold), 5u);
}

static void same_amount_mining_is_never_filtered() {
    Ledger l;
    l.set_skip_joint_quests(true);
    l.collect(A, 1.0, 0);
    l.collect(B, 1.0, 0);
    l.record(A, 10, kMining, 1);
    l.record(B, 10, kMining, 1);
    EXPECT_EQ(l.collect(A, 1.0, 2), 10u);
    EXPECT_EQ(l.collect(B, 1.0, 2), 10u);
}

static void gains_after_a_held_quest_wait_and_then_arrive() {
    Ledger l;
    l.set_skip_joint_quests(true);
    l.collect(B, 1.0, 0);
    l.record(A, 5, kQuest, 1);
    l.record(A, 10, kMining, 2);
    EXPECT_EQ(l.collect(B, 1.0, 3), 0u);
    EXPECT_EQ(l.collect(B, 1.0, 1 + kHold), 15u);
}

static void test_mode_keeps_own_quest_with_filter_on() {
    Ledger l;
    l.set_skip_joint_quests(true);
    l.collect(A, 1.0, 0, true);
    l.record(A, 5, kQuest, 1);
    EXPECT_EQ(l.collect(A, 1.0, 1 + kHold, true), 5u);
}

int main() {
    first_visit_gets_no_back_pay();
    other_players_gain_is_shared_once();
    own_gain_is_not_returned();
    test_mode_returns_own_gain_once();
    combat_and_none_are_never_recorded();
    every_receiver_gets_its_share();
    multiplier_is_applied_and_floored();
    zero_multiplier_shares_nothing();
    prune_drops_old_gains_and_players();
    joint_quest_is_double_with_filter_off();
    joint_quest_is_skipped_with_filter_on();
    solo_quest_is_shared_after_the_hold();
    receivers_late_quest_gain_still_counts_as_joint();
    different_quest_amounts_are_both_shared();
    same_amount_mining_is_never_filtered();
    gains_after_a_held_quest_wait_and_then_arrive();
    test_mode_keeps_own_quest_with_filter_on();
    if (failures == 0) {
        std::printf("ledger tests passed\n");
        return EXIT_SUCCESS;
    }
    std::printf("%d ledger test(s) failed\n", failures);
    return EXIT_FAILURE;
}
