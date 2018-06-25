// Author(s): Ruud Koolen
//            Wieger Wesselink 2018
// Copyright: see the accompanying file COPYING or copy at
// https://github.com/mCRL2org/mCRL2/blob/master/COPYING
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <ctime>
#include <iomanip>
#include <memory>

#include "mcrl2/utilities/logger.h"
#include "mcrl2/lps/resolve_name_clashes.h"
#include "mcrl2/lps/detail/instantiate_global_variables.h"
#include "mcrl2/lps/probabilistic_data_expression.h"
#include "mcrl2/lps/one_point_rule_rewrite.h"
#include "mcrl2/lts/detail/liblts_swap_to_from_probabilistic_lts.h"
#include "mcrl2/lts/detail/lts_convert.h"
#include "mcrl2/lts/detail/exploration.h"
#include "mcrl2/lts/detail/counter_example.h"
#include "mcrl2/lts/probabilistic_lts.h"

namespace mcrl2 {

namespace lts {

void lps2lts_algorithm::on_start_exploration()
{
  if (m_options.outformat == lts_aut)
  {
    mCRL2log(log::verbose) << "writing state space in AUT format to '" << m_options.filename << "'." << std::endl;
    m_aut_file.open(m_options.filename.c_str());
    if (!m_aut_file.is_open())
    {
      mCRL2log(log::error) << "cannot open '" << m_options.filename << "' for writing" << std::endl;
      std::exit(EXIT_FAILURE);
    }
  }
  else if (m_options.outformat == lts_none)
  {
    mCRL2log(log::verbose) << "not saving state space." << std::endl;
  }
  else
  {
    mCRL2log(log::verbose) << "writing state space in "
                           << "unknown" // mcrl2::lts::detail::string_for_type(m_options.outformat)
                           << " format to '" << m_options.filename << "'." << std::endl;
    m_output_lts.set_data(m_options.specification.data());
    m_output_lts.set_process_parameters(m_options.specification.process().process_parameters());
    m_output_lts.set_action_label_declarations(m_options.specification.action_labels());
  }

  if (m_options.outformat == lts_aut)
  {
    // HACK: this line will be overwritten once generation is finished.
    m_aut_file << "                                                             " << std::endl;
  }
  else if (m_options.outformat != lts_none)
  {
    auto initial_state_number = m_output_lts.add_state(state_label_lts(m_generator->initial_state()));
    m_output_lts.set_initial_state(initial_state_number);
  }
}

void lps2lts_algorithm::on_new_state(const lps::state& target_state)
{
  if (m_options.outformat != lts_none && m_options.outformat != lts_aut)
  {
    m_output_lts.add_state(state_label_lts(target_state));
  }
}

void lps2lts_algorithm::on_transition(std::size_t source_state_number, const lps::multi_action& action, std::size_t target_state_number)
{
  if (m_options.outformat == lts_aut)
  {
    m_aut_file << "(" << source_state_number << ",\"" << lps::pp(action) << "\"," << target_state_number << ")" << std::endl;
  }
  else if (m_options.outformat != lts_none)
  {
    std::pair<size_t, bool> action_label_number = m_action_label_numbers.put(action.actions());
    if (action_label_number.second)
    {
      std::size_t action_number = m_output_lts.add_action(action_label_lts(action));
      assert(action_number == action_label_number.first);
      static_cast <void>(action_number); // Avoid a warning when compiling in non debug mode.
    }
    m_output_lts.add_transition(mcrl2::lts::transition(source_state_number, action_label_number.first, target_state_number));
  }
}

void lps2lts_algorithm::on_end_exploration()
{
  if (m_options.outformat == lts_aut)
  {
    m_aut_file.flush();
    m_aut_file.seekp(0);
    m_aut_file << "des (0," << m_number_of_transitions << "," << m_number_of_states << ")";
    m_aut_file.close();
  }
  else if (m_options.outformat != lts_none)
  {
    if (!m_options.outinfo)
    {
      m_output_lts.clear_state_labels();
    }

    switch (m_options.outformat)
    {
      case lts_lts:
      {
        m_output_lts.save(m_options.filename);
        break;
      }
      case lts_fsm:
      {
        // TODO: get rid of this ugly code
        probabilistic_lts_lts_t output_lts;
        probabilistic_lts_fsm_t fsm;
        detail::translate_to_probabilistic_lts(m_output_lts, output_lts);
        detail::lts_convert(output_lts, fsm);
        fsm.save(m_options.filename);
        break;
      }
      case lts_dot:
      {
        // TODO: get rid of this ugly code
        probabilistic_lts_lts_t output_lts;
        probabilistic_lts_dot_t dot;
        detail::translate_to_probabilistic_lts(m_output_lts, output_lts);
        detail::lts_convert(output_lts, dot);
        dot.save(m_options.filename);
        break;
      }
      default:
        assert(0);
    }
  }
}

bool lps2lts_algorithm::initialise_lts_generation(const lts_generation_options& options)
{
  m_options = options;
  m_state_numbers = atermpp::indexed_set<lps::state>(m_options.initial_table_size, 50);
  m_number_of_states = 0;
  m_number_of_transitions = 0;
  m_level = 1;

  // preprocess the LPS
  lps::specification& lpsspec = m_options.specification;
  lps::resolve_summand_variable_name_clashes(lpsspec);
  if (m_options.instantiate_global_variables)
  {
    lps::detail::instantiate_global_variables(lpsspec);
  }
  lps::one_point_rule_rewrite(lpsspec);

  data::rewriter rewriter;
  if (m_options.remove_unused_rewrite_rules)
  {
    mCRL2log(log::verbose) << "removing unused parts of the data specification." << std::endl;
    std::set<data::function_symbol> extra_function_symbols = lps::find_function_symbols(lpsspec);
    extra_function_symbols.insert(data::sort_real::minus(data::sort_real::real_(), data::sort_real::real_()));

    rewriter = data::rewriter(lpsspec.data(),
                              data::used_data_equation_selector(lpsspec.data(), extra_function_symbols,
                                                                lpsspec.global_variables()), m_options.strat);
  }
  else
  {
    rewriter = data::rewriter(lpsspec.data(), m_options.strat);
  }

  bool compute_actions = m_options.outformat != lts_none;
  if (!compute_actions)
  {
    for (auto& summand: lpsspec.process().action_summands())
    {
      summand.multi_action().actions() = process::action_list();
    }
  }
  m_generator = std::make_unique<lps::next_state_generator>(lpsspec, rewriter, m_options.use_enumeration_caching);

  if (m_options.detect_deadlock)
  {
    mCRL2log(log::verbose) << "Detect deadlocks.\n";
  }

  if (m_options.detect_nondeterminism)
  {
    mCRL2log(log::verbose) << "Detect nondeterministic states.\n";
  }
  return true;
}

bool lps2lts_algorithm::generate_lts(const lts_generation_options& options)
{
  if (!initialise_lts_generation(options))
  {
    return false;
  }

  on_start_exploration();

  m_state_numbers.put(m_generator->initial_state());
  m_number_of_states = 1;

  mCRL2log(log::verbose) << "generating state space with '" << es_breadth << "' strategy...\n";

  if (m_options.max_states == 0)
  {
    return true;
  }

  generate_lts_breadth_first();

  mCRL2log(log::verbose) << "done with state space generation ("
                    << m_level - 1 << " level" << ((m_level == 2) ? "" : "s") << ", "
                    << m_number_of_states << " state" << ((m_number_of_states == 1) ? "" : "s")
                    << " and " << m_number_of_transitions << " transition"
                    << ((m_number_of_transitions == 1) ? "" : "s") << ")"
                    << std::endl;

  on_end_exploration();

  return true;
}

// Add the target state to the transition system, and if necessary store it to be investigated later.
// Return the number of the target state.
std::pair<std::size_t, bool>
lps2lts_algorithm::add_target_state(const lps::state& source_state, const lps::state& target_state)
{
  std::pair<std::size_t, bool> target_state_number = m_state_numbers.put(target_state);
  if (target_state_number.second) // The state is new.
  {
    m_number_of_states++;
    on_new_state(target_state);
  }
  return target_state_number;
}

bool
lps2lts_algorithm::add_transition(const lps::state& source_state, const lps::next_state_generator::transition& transition)
{

  std::size_t source_state_number = m_state_numbers[source_state];
  const std::pair<std::size_t, bool> target_state_number = add_target_state(source_state, transition.target_state);

  on_transition(source_state_number, transition.action, target_state_number.first);

  m_number_of_transitions++;
  return target_state_number.second;
}

// The function below checks whether in the set of outgoing transitions,
// there are two transitions with the same label, going to different states.
// If this is the case, true is delivered and one nondeterministic transition 
// is returned in the variable nondeterministic_transition.
bool
lps2lts_algorithm::is_nondeterministic(std::vector<lps::next_state_generator::transition>& transitions,
                                       lps::next_state_generator::transition& nondeterministic_transition)
{
  // Below a mapping from transition labels to target states is made. 
  static std::map<lps::multi_action, lps::state> sorted_transitions; // The set is static to avoid repeated construction.
  assert(sorted_transitions.empty());
  for (const lps::next_state_generator::transition& tr: transitions)
  {
    auto i = sorted_transitions.find(tr.action);
    if (i != sorted_transitions.end())
    {
      if (i->second != tr.target_state)
      {
        // Two transitions with the same label and different targets states have been found. This state is nondeterministic.
        sorted_transitions.clear();
        nondeterministic_transition = tr;
        return true;
      }
    }
    else
    {
      sorted_transitions[tr.action] = tr.target_state;
    }
  }
  sorted_transitions.clear();
  return false;
}

void lps2lts_algorithm::generate_transitions(const lps::state& state,
                                             std::vector<lps::next_state_generator::transition>& transitions,
                                             lps::next_state_generator::enumerator_queue& enumeration_queue
)
{
  assert(transitions.empty());
  try
  {
    enumeration_queue.clear();
    lps::next_state_generator::iterator it(m_generator->begin(state, &enumeration_queue));
    while (it)
    {
      transitions.push_back(*it++);
    }
  }
  catch (mcrl2::runtime_error& e)
  {
    mCRL2log(log::error) << "Error while exploring state space: " << e.what() << "\n";
    if (m_options.outformat == lts_aut)
    {
      m_aut_file.flush();
    }
    std::exit(EXIT_FAILURE);
  }

  if (m_options.detect_deadlock && transitions.empty())
  {
    mCRL2log(log::info) << "deadlock-detect: deadlock found (state index: " << m_state_numbers.index(state) << ").\n";
  }

  if (m_options.detect_nondeterminism)
  {
    lps::next_state_generator::transition nondeterministic_transition;
    if (is_nondeterministic(transitions, nondeterministic_transition))
    {
      mCRL2log(log::info) << "Nondeterministic state found (state index: " << m_state_numbers.index(state) << ").\n";
    }
  }
}

void lps2lts_algorithm::generate_lts_breadth_first()
{
  std::size_t current_state = 0;
  std::size_t start_level_seen = 1;
  std::size_t start_level_transitions = 0;
  std::vector<lps::next_state_generator::transition> transitions;
  time_t last_log_time = time(nullptr) - 1, new_log_time;
  lps::next_state_generator::enumerator_queue enumeration_queue;

  while (!m_must_abort && (current_state < m_state_numbers.size()) && (current_state < m_options.max_states))
  {
    lps::state state = m_state_numbers.get(current_state);
    generate_transitions(state, transitions, enumeration_queue);
    for (const lps::next_state_generator::transition& t: transitions)
    {
      add_transition(state, t);
    }
    transitions.clear();

    current_state++;
    if (current_state == start_level_seen)
    {
      mCRL2log(log::debug) << "Number of states at level " << m_level << " is " << m_number_of_states - start_level_seen << "\n";
      m_level++;
      start_level_seen = m_number_of_states;
      start_level_transitions = m_number_of_transitions;
    }

    if (!m_options.suppress_progress_messages && time(&new_log_time) > last_log_time)
    {
      last_log_time = new_log_time;
      std::size_t lvl_states = m_number_of_states - start_level_seen;
      std::size_t lvl_transitions = m_number_of_transitions - start_level_transitions;
      mCRL2log(log::status) << std::fixed << std::setprecision(2)
                       << m_number_of_states << "st, " << m_number_of_transitions << "tr"
                       << ", explored " << 100.0 * ((float) current_state / m_number_of_states)
                       << "%. Last level: " << m_level << ", " << lvl_states << "st, " << lvl_transitions
                       << "tr.\n";
    }
  }

  if (current_state == m_options.max_states)
  {
    mCRL2log(log::verbose) << "explored the maximum number (" << m_options.max_states << ") of states, terminating." << std::endl;
  }
}

} // namespace lts

} // namespace mcrl2
