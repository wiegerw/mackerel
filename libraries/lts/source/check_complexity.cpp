// Author(s): David N. Jansen, Radboud Universiteit, Nijmegen, The Netherlands
//
// Copyright: see the accompanying file COPYING or copy at
// https://github.com/mCRL2org/mCRL2/blob/master/COPYING
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

/// \file lts/source/check_complexity.cpp
///
/// \brief helper class for time complexity checks during test runs
///
/// \details We use the class in this file to check whether the overall time
/// complexity fits in O(m log n).  Although it is difficult to test this in
/// general because of the constant factor in the definition of the O()
/// notation, it is often possible to give a (rather tight) upper bound on the
/// number of iterations of most loops.
///
/// The principle of time measurement with this file is:  the work done in
/// every loop body is assigned to a state or a transition.  Every state and
/// every transition gets a counter for every loop body which is assigned
/// to it.  When the loop body is executed, the corresponding counter is
/// increased.  When increasing a counter, a new value is assigned, based on
/// the logarithm of the size of the corresponding block (or constellation).
/// If the new value is not actually larger than the old one, an error is
/// raised.  The new value never is allowed to become larger than log2(n), so
/// we have:  For every counter, its value is increased at most log2(n) times,
/// and therefore no more than log2(n) steps (of a certain kind) can be
/// assigned to any single state or transition.
///
/// Note that an ``increase'' is by at least 1, but it may be more than 1.  If
/// one always increases to the maximal allowed value, it is ensured that a
/// very small block found early will only incur work that corresponds to its
/// own size (and not to the size of the block from which it was split off).
///
/// To assign work to some unit, write `mCRL2complexity(unit, add_work(counter
/// type, new counter value));`  The `unit` is a state or transition;  the
/// `counter type` is a value of `enum check_complexity::counter_type` defined
/// below; the `new counter value` typically is `check_complexity::log_n -
/// check_complexity::ilog2(block size)`.
///
/// For coroutines, there is an additional provision: work can temporarily be
/// recorded in some special counters.  As soon as it becomes clear which
/// coroutine (the one handling red states or the one handling blue states, in
/// our case) is faster, its counters are transferred to normal counters and
/// the counters of the other coroutine are cancelled.  It is checked that not
/// too many counters are cancelled.
///
/// To transfer work from a temporary to a normal counter, one uses
/// `finalise_work()`.  To cancel counters, use `cancel_work()`.  These
/// functions are not normally called directly, but through `red_is_smaller()`
/// or `blue_is_smaller()`:  If the red subblock is smaller, call
/// `red_is_smaller(temporary counter, normal counter, new counter value)` for
/// each state and each transition of the block that was refined;  if the blue
/// subblock is smaller, call `blue_is_smaller(...)`.
/// After all temporary work has been handled, call `check_temporary_work()` to
/// compare the amount of sensible work with the amount of cancelled work.
///
/// If the work could be assigned to one of several counters, I recommend to
/// assign it to all of them;  otherwise, it may happen that a later excess of
/// the time budget goes unnoticed because too few counters were advanced.
/// This, however, poses some difficulties when using temporary counters: a
/// single unit of work should be assigned to multiple counters, but added to
/// the balance between sensible and superfluous work only once.  A variant of
/// `add_work()`, namely `add_work_notemporary()`, can be called in that case:
/// it assigns a special value `DONT_COUNT_TEMPORARY` to a temporary counter
/// meaning that it should be disregarded in the calculation of the balance.
///
/// \author David N. Jansen, Radboud Universiteit, Nijmegen, The Netherlands


#include "mcrl2/lts/detail/check_complexity.h"

namespace mcrl2
{
namespace lts
{
namespace detail
{
namespace bisim_gjkw
{

#ifndef NDEBUG

/// \brief binary logarithm of the state space size, rounded down
unsigned char check_complexity::log_n = '\0';

/// \brief the number of useful steps in the last refinement
signed_state_type check_complexity::sensible_work = 0;

/// \brief names for complexity counters
/// \details Every complexity counter (defined in check_complexity.h)
/// corresponds to a loop in one of the places of the algorithm.  Here, an
/// English description of that place is given, together with the line number
/// in the pseudocode in the article [Groote/Jansen/Keiren/Wijs: An O(m log n)
/// algorithm for computing stuttering equivalence and branching bisimulation.
/// Accepted for publication in ACM TOCL 2017].
const char *check_complexity::work_names[TRANS_MAX - BLOCK_MIN + 1] =
{
    // block counters
    "2.4: while C contains a nontrivial constellation",
    "2.10: for all s in SpB",
    "3.29: Move Blue/Red to a new block NewB (set pointers to new block)",
    "3.29: Move Blue/Red to a new block NewB (for all states s in NewB)",
    "3.31: for all s in NewB",

    // state counters
    "3.29: Move Blue/Red to a new block NewB (swap states)",

    "3.6l: refine bottom state",
    "3.15: refine visited state",

    // temporary state counters (blue):
    "3.6l: while Test is not empty (3.11l: s is blue)",
    "3.15l: while Blue contains unvisited states",

    // temporary state counters (red):
    "3.15r: while Red contains unvisited states",

    // new bottom state counters: every state is visited once
    "4.8: for all bottom states s in RfnB",
    "4.15: for all old bottom states s in RedB (self-loop)",
    
    // B_to_C_descriptor counters
    "2.20: for all refinable blocks RfnB",
    "2.17: Register that inert transitions from s go to NewC (B_to_C)",
    "4.4: for all constellations C not in R reachable from RfnB",

    // transition counters: every transition is visited O(log n) times
    "2.11: for all s_prime in in(s)",
    "2.17: Register that inert transitions from s go to NewC (out)",
    "2.17: Register that inert transitions from s go to NewC (swap "
                                                                "transitions)",
    "3.6l: refine outgoing transition to marked state",

    "3.6/3.23l: refine outgoing transition",
    "3.29: Move Blue/Red to a new block NewB (for all transitions in out(s))",
    "3.32r: for all s_prime in out(s)",

    "3.18: refine incoming transition",
    "3.32l: for all s_prime in in(s)",

    // temporary transition counters (blue):
    "3.6l: while Test is not empty (3.9l: s is red)",
    "3.6l: while Test is not empty (3.9l: s is red) during postprocessing",
    "3.18l: for all s_prime in in(s) \\ Red",
    "3.23l: if ... s_prime has no transition to SpC",

    // temporary transition counters (red):
    "3.6r: while FromRed is not empty",
    "3.18r: for all s_prime in in(s)",

    // new bottom transition counters: every transition is visited once
    "3.6l: refine outgoing transition (called from PostprocessNewBottom)",
    "3.23l: refine outgoing transition (state is new bottom state)",
    "4.4: for all transitions from new bottom states (a priori)",
    "4.4: for all transitions from new bottom states (a posteriori)",
    "4.12: for all blocks B with transitions to SpC that need postprocessing "
                                                                  "(a priori)",
    "4.12: for all blocks B with transitions to SpC that need postprocessing "
                                                              "(a posteriori)",
    "4.15: for all old bottom states s in RedB"
};


#if 0
/// \brief helper macro for check_complexity::test_work_names()
#define test_work_name(var,ctr)                                               \
        do                                                                    \
        {                                                                     \
            assert((var) == ctr);                                             \
            mCRL2log(log::debug, "bisim_gjkw") << "work_names[" #ctr "] = \"" \
                                                << work_names[ctr] << "\".\n";\
            (var) = (enum counter_type) ((var) + 1);                          \
        }                                                                     \
        while (0)


/// \brief prints a message for each counter, for debugging purposes
/// \details The function prints, for each counter, the enum constant and the
/// string associated with these counters.  The function is intended to be used
/// for debugging:  a programmer can check the output of the function and see
/// whether each enum constant is similar to the string stored in that
/// position.  If no, confusing error messages about time budget overruns may
/// be printed.
///
/// The function is not currently called.  A typical place where a call could
/// be inserted is in check_complexity::init() in check_complexity.h.
void check_complexity::test_work_names()
{
    enum check_complexity::counter_type i = BLOCK_MIN;
    // block counters
    assert(check_complexity::BLOCK_MIN == i);
    test_work_name(i, while_C_contains_a_nontrivial_constellation_2_4);
    test_work_name(i, for_all_s_in_SpB_2_10);
    test_work_name(i, Move_Blue_or_Red_to_a_new_block_NewB_pointer_3_29);
    test_work_name(i, Move_Blue_or_Red_to_a_new_block_states_3_29);
    test_work_name(i, for_all_s_in_NewB_3_31);
    assert(check_complexity::BLOCK_MAX + 1 == i);

    // state counters
    assert(check_complexity::STATE_MIN == i);
    test_work_name(i, Move_Blue_or_Red_to_a_new_block_NewB_swap_3_29);
    test_work_name(i, refine_bottom_state_3_6l);
    test_work_name(i, refine_visited_state_3_15);

    // temporary state counters (blue):
    test_work_name(i, while_Test_is_not_empty_3_6l_s_is_blue_3_11l);
    test_work_name(i, while_Blue_contains_unvisited_states_3_15l);

    // temporary state counters (red):
    test_work_name(i, while_Red_contains_unvisited_states_3_15r);

    // new bottom state counters
    test_work_name(i, for_all_bottom_states_s_in_RfnB_4_8);
    test_work_name(i, for_all_old_bottom_states_s_in_RedB_selfloop_4_15);
    assert(check_complexity::STATE_MAX + 1 == i);

    // B_to_C_descriptor counters
    assert(check_complexity::B_TO_C_MIN == i);
    test_work_name(i, for_all_refinable_blocks_RfnB_2_20);
    test_work_name(i,
                Register_that_inert_transitions_from_s_go_to_NewC_B_to_C_2_17);
    // temporary B_to_C_descriptor counters
    test_work_name(i, for_all_constellations_C_not_in_R_from_RfnB_4_4);
    assert(check_complexity::B_TO_C_MAX + 1 == i);

    // transition counters
    assert(check_complexity::TRANS_MIN == i);
    test_work_name(i, for_all_s_prime_in_pred_s_2_11);
    test_work_name(i,
                  Register_that_inert_transitions_from_s_go_to_NewC_succ_2_17);
    test_work_name(i,
                  Register_that_inert_transitions_from_s_go_to_NewC_swap_2_17);
    test_work_name(i, refine_outgoing_transition_to_marked_state_3_6l);

    test_work_name(i, refine_outgoing_transition_3_6_or_23l);
    test_work_name(i, Move_Blue_or_Red_to_a_new_block_succ_3_29);
    test_work_name(i, for_all_s_prime_in_succ_s_3_32r);

    test_work_name(i, refine_incoming_transition_3_18);
    test_work_name(i, for_all_s_prime_in_pred_s_3_32l);

    // temporary transition counters (blue):
    assert(check_complexity::TRANS_MIN_TEMPORARY == i);
    test_work_name(i, while_Test_is_not_empty_3_6l_s_is_red_3_9l);
    test_work_name(i,
                    while_Test_is_not_empty_3_6l_s_is_red_3_9l_postprocessing);
    test_work_name(i, for_all_s_prime_in_pred_s_setminus_Red_3_18l);
    test_work_name(i, if___s_prime_has_transition_to_SpC_3_23l);

    // temporary transition counters (red):
    test_work_name(i, while_FromRed_is_not_empty_3_6r);
    test_work_name(i, for_all_s_prime_in_pred_s_3_18r);
    assert(check_complexity::TRANS_MAX_TEMPORARY + 1 == i);

    // new bottom transition counters
    test_work_name(i, refine_outgoing_transition_postprocess_new_bottom_3_6l);
    test_work_name(i, refine_outgoing_transition_from_new_bottom_3_23l);
    test_work_name(i, for_all_transitions_from_bottom_states_a_priori_4_4);
    test_work_name(i, for_all_transitions_from_bottom_states_a_posteriori_4_4);
    test_work_name(i, for_all_transitions_that_need_postproc_a_priori_4_12);
    test_work_name(i,for_all_transitions_that_need_postproc_a_posteriori_4_12);
    test_work_name(i, for_all_old_bottom_states_s_in_RedB_4_15);
    assert(check_complexity::TRANS_MAX + 1 == i);

    exit(EXIT_SUCCESS);
}
#endif // #if 0

#endif // #ifndef NDEBUG

} // end namespace bisim_gjkw
} // end namespace detail
} // end namespace lts
} // end namespace mcrl2
