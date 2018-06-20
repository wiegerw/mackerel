// Author(s): Ruud Koolen
//            Wieger Wesselink 2018
// Copyright: see the accompanying file COPYING or copy at
// https://github.com/mCRL2org/mCRL2/blob/master/COPYING
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MCRL2_LTS_DETAIL_EXPLORATION_NEW_H
#define MCRL2_LTS_DETAIL_EXPLORATION_NEW_H

#include <string>
#include <limits>
#include <memory>
#include <unordered_set>

#include "mcrl2/atermpp/indexed_set.h"
#include "mcrl2/lps/next_state_generator.h"
#include "mcrl2/lts/lts_lts.h"
#include "mcrl2/lts/detail/queue.h"
#include "mcrl2/lts/detail/lts_generation_options.h"
#include "mcrl2/lts/detail/exploration_strategy.h"

namespace mcrl2 {

namespace lts {

class lps2lts_algorithm
{
  private:
    lts_generation_options m_options;

    // TODO: this generator should not be stored as a pointer
    std::unique_ptr<lps::next_state_generator> m_generator;

    // TODO: this summand_subset should be an implementation detail of the next state generator.
    lps::next_state_generator::summand_subset m_main_subset;

    atermpp::indexed_set<lps::state> m_state_numbers;
    atermpp::indexed_set<process::action_list> m_action_label_numbers;
    std::size_t m_number_of_states = 0;
    std::size_t m_number_of_transitions = 0;
    std::size_t m_level = 0;

    // TODO: the details of writing the computed LTS (in two different formats!?) should not be hard coded like this
    lts_lts_t m_output_lts;
    std::ofstream m_aut_file;

    volatile bool m_must_abort = false;

  public:
    lps2lts_algorithm()
    {
      m_action_label_numbers.put(action_label_lts::tau_action().actions());  // The action tau has index 0 by default.
    }

    bool generate_lts(const lts_generation_options& options);

    void abort()
    {
      // Stops the exploration algorithm if it is running by making sure
      // not a single state can be generated anymore.
      if (!m_must_abort)
      {
        m_must_abort = true;
        mCRL2log(log::warning) << "state space generation was aborted prematurely" << std::endl;
      }
    }

    virtual void on_start_exploration();
    virtual void on_new_state(const lps::state& s);
    virtual void on_transition(std::size_t source_state_number, const lps::multi_action& action, std::size_t target_state_number);
    virtual void on_end_exploration();

  private:
    bool initialise_lts_generation(const lts_generation_options& options);

    bool is_nondeterministic(std::vector<lps::next_state_generator::transition>& transitions, lps::next_state_generator::transition& nondeterminist_transition);

    std::pair<std::size_t, bool> add_target_state(const lps::state& source_state, const lps::state& target_state);

    bool add_transition(const lps::state& source_state, const lps::next_state_generator::transition& transition);

    void generate_transitions(const lps::state& state,
                              std::vector<lps::next_state_generator::transition>& transitions,
                              lps::next_state_generator::enumerator_queue& enumeration_queue
    );

    void generate_lts_breadth_first();
};

} // namespace lps

} // namespace mcrl2

#endif // MCRL2_LTS_DETAIL_EXPLORATION_H
