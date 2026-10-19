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
    if (failures == 0) {
        std::printf("ledger tests passed\n");
        return EXIT_SUCCESS;
    }
    std::printf("%d ledger test(s) failed\n", failures);
    return EXIT_FAILURE;
}
