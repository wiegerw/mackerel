// Author(s): Ruud Koolen
// Copyright: see the accompanying file COPYING or copy at
// https://svn.win.tue.nl/trac/MCRL2/browser/trunk/COPYING
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
/// \file next_state_generator.cpp

#include <set>
#include <algorithm>

#include "mcrl2/lps/next_state_generator.h"

using namespace mcrl2;
using namespace mcrl2::lps;

namespace mcrl2 {

namespace lps {

next_state_generator::next_state_generator(
        const specification& spec,
        const data::rewriter& rewriter,
        bool use_enumeration_caching,
        bool use_summand_pruning)
        : m_specification(spec),
          m_rewriter(rewriter),
          m_enumerator(m_rewriter, m_specification.data(), m_rewriter, m_id_generator,
                       (std::numeric_limits<std::size_t>::max)(), true),  // Generate exceptions.
          m_use_enumeration_caching(use_enumeration_caching)
{
  m_process_parameters = data::variable_vector(m_specification.process().process_parameters().begin(),
                                               m_specification.process().process_parameters().end());

  if (m_specification.process().has_time())
  {
    mCRL2log(log::warning) << "Specification uses time, which is (currently) only partly supported." << std::endl;
  }

  for (auto& action_summand : m_specification.process().action_summands())
  {
    next_state_summand summand;
    summand.summand = &action_summand;
    summand.variables = order_variables_to_optimise_enumeration(action_summand.summation_variables(), spec.data());
    summand.condition = action_summand.condition();
    const data::data_expression_list& l = action_summand.next_state(m_specification.process().process_parameters());
    summand.result_state = data::data_expression_vector(l.begin(), l.end());
    if (action_summand.multi_action().has_time())
    {
      summand.time = action_summand.multi_action().time();
    }

    for (const auto& a: action_summand.multi_action().actions())
    {
      next_state_action_label action_label;
      action_label.label = a.label();

      for (const auto& arg: a.arguments())
      {
        action_label.arguments.push_back(arg);
      }
      summand.action_label.push_back(action_label);
    }

    for (std::size_t j = 0;
         j < m_process_parameters.size();
         j++)
    {
      if (data::search_free_variable(action_summand.condition(), m_process_parameters[j]))
      {
        summand.condition_parameters.push_back(j);
      }
    }
    summand.condition_arguments_function = atermpp::function_symbol("condition_arguments",
                                                                    summand.condition_parameters.size());
    std::vector<atermpp::aterm_int> dummy(summand.condition_arguments_function.arity(),
                                          atermpp::aterm_int(static_cast<std::size_t>(0)));
    summand.condition_arguments_function_dummy = atermpp::aterm_appl(summand.condition_arguments_function,
                                                                     dummy.begin(), dummy.end());

    m_summands.push_back(summand);
  }

  data::data_expression_list initial_state_raw = m_specification.initial_process().state(
          m_specification.process().process_parameters());

  data::mutable_indexed_substitution<> sigma;
  data::data_expression_vector initial_symbolic_state(initial_state_raw.begin(), initial_state_raw.end());
  m_all_summands = summand_subset(this, use_summand_pruning);
}

next_state_generator::summand_subset::summand_subset(next_state_generator* generator, bool use_summand_pruning)
        : m_generator(generator),
          m_use_summand_pruning(use_summand_pruning)
{
  if (m_use_summand_pruning)
  {
    m_pruning_tree.summand_subset = atermpp::detail::shared_subset<next_state_summand>(generator->m_summands);
    build_pruning_parameters(generator->m_specification.process().action_summands());
  }
  else
  {
    for (std::size_t i = 0;
         i < generator->m_summands.size();
         i++)
    {
      m_summands.push_back(i);
    }
  }
}

bool next_state_generator::summand_subset::summand_set_contains(
        const std::set<action_summand>& summand_set,
        const next_state_generator::next_state_summand& summand)
{
  return summand_set.count(*summand.summand) > 0;
}

next_state_generator::summand_subset::summand_subset(
        next_state_generator* generator,
        const action_summand_vector& summands,
        bool use_summand_pruning)
        : m_generator(generator),
          m_use_summand_pruning(use_summand_pruning)
{
  std::set<action_summand> summand_set;
  for (const action_summand& i: summands)
  {
    summand_set.insert(i);
  }

  if (m_use_summand_pruning)
  {
    atermpp::detail::shared_subset<next_state_summand> full_set(generator->m_summands);
    m_pruning_tree.summand_subset = atermpp::detail::shared_subset<next_state_summand>(full_set, std::bind(
            next_state_generator::summand_subset::summand_set_contains, summand_set, std::placeholders::_1));
    build_pruning_parameters(summands);
  }
  else
  {
    for (std::size_t i = 0;
         i < generator->m_summands.size();
         i++)
    {
      if (summand_set.count(*generator->m_summands[i].summand) > 0)
      {
        m_summands.push_back(i);
      }
    }
  }
}

static float condition_selectivity(const data::data_expression& e, const data::variable& v)
{
  if (data::sort_bool::is_and_application(e))
  {
    return condition_selectivity(data::binary_left(atermpp::down_cast<data::application>(e)), v)
           + condition_selectivity(data::binary_right(atermpp::down_cast<data::application>(e)), v);
  }
  else if (data::sort_bool::is_or_application(e))
  {
    float sum = 0;
    std::size_t count = 0;
    std::list<data::data_expression> terms;
    terms.push_back(e);
    while (!terms.empty())
    {
      data::data_expression expression = terms.front();
      terms.pop_front();
      if (data::sort_bool::is_or_application(expression))
      {
        terms.push_back(data::binary_left(atermpp::down_cast<data::application>(e)));
        terms.push_back(data::binary_right(atermpp::down_cast<data::application>(e)));
      }
      else
      {
        sum += condition_selectivity(expression, v);
        count++;
      }
    }
    return sum / count;
  }
  else if (is_equal_to_application(e))
  {
    const data::data_expression& left = data::binary_left(atermpp::down_cast<data::application>(e));
    const data::data_expression& right = data::binary_right(atermpp::down_cast<data::application>(e));

    if (data::is_variable(left) && data::variable(left) == v)
    {
      return 1;
    }
    else if (data::is_variable(right) && data::variable(right) == v)
    {
      return 1;
    }
    else
    {
      return 0;
    }
  }
  else
  {
    return 0;
  }
}

struct parameter_score
{
  std::size_t parameter_id;
  float score;

  parameter_score() = default;

  parameter_score(std::size_t id, float score_)
          : parameter_id(id), score(score_)
  {
  }
};

static bool parameter_score_compare(const parameter_score& left, const parameter_score& right)
{
  return left.score > right.score;
}

void next_state_generator::summand_subset::build_pruning_parameters(const action_summand_vector& summands)
{
  std::vector<parameter_score> parameters;

  for (std::size_t i = 0;
       i < m_generator->m_process_parameters.size();
       i++)
  {
    parameters.emplace_back(i, 0);
    for (const auto& summand : summands)
    {
      parameters[i].score += condition_selectivity(summand.condition(), m_generator->m_process_parameters[i]);
    }
  }

  std::sort(parameters.begin(), parameters.end(), parameter_score_compare);

  for (std::size_t i = 0;
       i < m_generator->m_process_parameters.size();
       i++)
  {
    if (parameters[i].score > 0)
    {
      m_pruning_parameters.push_back(parameters[i].parameter_id);
      mCRL2log(log::verbose) << "using pruning parameter "
                             << m_generator->m_process_parameters[parameters[i].parameter_id].name() << std::endl;
    }
  }
}

bool next_state_generator::summand_subset::is_not_false(const next_state_generator::next_state_summand& summand)
{
  return m_generator->m_rewriter(summand.condition, m_pruning_substitution) != data::sort_bool::false_();
}

atermpp::detail::shared_subset<next_state_generator::next_state_summand>::iterator
next_state_generator::summand_subset::begin(const state& state)
{
  assert(m_use_summand_pruning);

  for (unsigned long m_pruning_parameter: m_pruning_parameters)
  {
    const data::variable& v = m_generator->m_process_parameters[m_pruning_parameter];
    m_pruning_substitution[v] = v;
  }

  pruning_tree_node* node = &m_pruning_tree;
  for (unsigned long parameter: m_pruning_parameters)
  {
    const data::data_expression& argument = state.element_at(parameter, m_generator->m_process_parameters.size());
    m_pruning_substitution[m_generator->m_process_parameters[parameter]] = argument;
    auto position = node->children.find(argument);
    if (position == node->children.end())
    {
      pruning_tree_node child;
      child.summand_subset = atermpp::detail::shared_subset<next_state_summand>(node->summand_subset, std::bind(
              &next_state_generator::summand_subset::is_not_false, this, std::placeholders::_1));
      node->children[argument] = child;
      node = &node->children[argument];
    }
    else
    {
      node = &position->second;
    }
  }

  return node->summand_subset.begin();
}


next_state_generator::iterator::iterator(next_state_generator* generator, const state& state,
                                         next_state_generator::rewriter_substitution* substitution,
                                         summand_subset& summand_subset, enumerator_queue* enumeration_queue)
        : m_generator(generator),
          m_state(state),
          m_substitution(substitution),
          m_single_summand(false),
          m_use_summand_pruning(summand_subset.m_use_summand_pruning),
          m_summand(nullptr),
          m_caching(false),
          m_enumeration_queue(enumeration_queue)
{
  if (m_use_summand_pruning)
  {
    m_summand_subset_iterator = summand_subset.begin(state);
  }
  else
  {
    m_summand_iterator = summand_subset.m_summands.begin();
    m_summand_iterator_end = summand_subset.m_summands.end();
  }

  std::size_t j = 0;
  for (auto i = state.begin();
       i != state.end();
       ++i, ++j)
  {
    (*m_substitution)[generator->m_process_parameters[j]] = *i;
  }

  increment();
}

next_state_generator::iterator::iterator(next_state_generator* generator, const state& state,
                                         next_state_generator::rewriter_substitution* substitution,
                                         std::size_t summand_index,
                                         enumerator_queue* enumeration_queue)
        : m_generator(generator),
          m_state(state),
          m_substitution(substitution),
          m_single_summand(true),
          m_single_summand_index(summand_index),
          m_use_summand_pruning(false),
          m_summand(nullptr),
          m_caching(false),
          m_enumeration_queue(enumeration_queue)
{
  std::size_t j = 0;
  for (auto i = state.begin();
       i != state.end();
       ++i, ++j)
  {
    (*m_substitution)[generator->m_process_parameters[j]] = *i;
  }

  increment();
}

void next_state_generator::iterator::increment()
{
  while (!m_summand ||
         (m_cached && m_enumeration_cache_iterator == m_enumeration_cache_end) ||
         (!m_cached && m_enumeration_iterator == m_generator->m_enumerator.end())
          )
  {
    // Here we have to get a new summand. Search through the summands until one is
    // found of which the condition is not equal to false. As a new summand is started
    // we can reset the identifier_generator as no local variables are in use. 
    m_generator->m_id_generator.clear();
    if (m_caching)
    {
      m_summand->enumeration_cache[m_enumeration_cache_key] = m_enumeration_log;
    }

    if (m_single_summand)
    {
      if (m_summand)
      {
        m_generator = nullptr;
        return;
      }
      m_summand = &(m_generator->m_summands[m_single_summand_index]);
    }
    else if (m_use_summand_pruning)
    {
      if (!m_summand_subset_iterator)
      {
        m_generator = nullptr;
        return;
      }
      m_summand = &(*m_summand_subset_iterator++);
    }
    else
    {
      if (m_summand_iterator == m_summand_iterator_end)
      {
        m_generator = nullptr;
        return;
      }
      m_summand = &(m_generator->m_summands[*m_summand_iterator++]);
    }

    if (m_generator->m_use_enumeration_caching)
    {
      m_enumeration_cache_key = enumeration_cache_key(m_summand->condition_arguments_function,
                                                      m_summand->condition_parameters.begin(),
                                                      m_summand->condition_parameters.end(),
                                                      [&](const std::size_t n) {
                                                          return m_state.element_at(n,
                                                                                    m_generator->m_process_parameters.size());
                                                      });

      auto position = m_summand->enumeration_cache.find(m_enumeration_cache_key);
      if (position == m_summand->enumeration_cache.end())
      {
        m_cached = false;
        m_caching = true;
        m_enumeration_log.clear();
      }
      else
      {
        m_cached = true;
        m_caching = false;
        m_enumeration_cache_iterator = position->second.begin();
        m_enumeration_cache_end = position->second.end();
      }
    }
    else
    {
      m_cached = false;
      m_caching = false;
    }
    if (!m_cached)
    {
      for (const auto& variable: m_summand->variables)
      {
        (*m_substitution)[variable] = variable;  // Reset the variable.
      }
      enumerate(m_summand->variables, m_summand->condition, *m_substitution);
    }
  }

  data::data_expression_list valuation;
  if (m_cached)
  {
    valuation = *m_enumeration_cache_iterator;
    m_enumeration_cache_iterator++;
    assert(valuation.size() == m_summand->variables.size());
    auto v = valuation.begin();
    for (auto i = m_summand->variables.begin();
         i != m_summand->variables.end();
         i++, v++)
    {
      (*m_substitution)[*i] = *v;
    }
  }
  else
  {
    m_enumeration_iterator->add_assignments(m_summand->variables, *m_substitution, m_generator->m_rewriter);

    // If we failed to exactly rewrite the condition to true, nextstate generation fails.
    if (m_enumeration_iterator->expression() != data::sort_bool::true_())
    {
      assert(m_enumeration_iterator->expression() != data::sort_bool::false_());

      // Reduce condition as much as possible, and give a hint of the original condition in the error message.
      data::data_expression reduced_condition(m_generator->m_rewriter(m_summand->condition, *m_substitution));
      std::string printed_condition(data::pp(m_summand->condition).substr(0, 300));

      throw mcrl2::runtime_error("Expression " + data::pp(reduced_condition) +
                                 " does not rewrite to true or false in the condition "
                                 + printed_condition
                                 + (printed_condition.size() >= 300 ? "..." : ""));
    }

    m_enumeration_iterator++;
    if (m_caching)
    {
      valuation = data::data_expression_list(m_summand->variables.begin(), m_summand->variables.end(), *m_substitution);
      assert(valuation.size() == m_summand->variables.size());
    }
  }

  if (m_caching)
  {
    m_enumeration_log.push_back(valuation);
  }

  const data::data_expression_vector& state_args = m_summand->result_state;
  m_transition.target_state = lps::state(state_args.begin(), state_args.size(), [&](const data::data_expression& x) {
      return m_generator->m_rewriter(x, *m_substitution);
  });

  std::vector<process::action> actions;
  actions.resize(m_summand->action_label.size());
  std::vector<data::data_expression> arguments;
  for (std::size_t i = 0;
       i < m_summand->action_label.size();
       i++)
  {
    arguments.resize(m_summand->action_label[i].arguments.size());
    for (std::size_t j = 0;
         j < m_summand->action_label[i].arguments.size();
         j++)
    {
      arguments[j] = m_generator->m_rewriter(m_summand->action_label[i].arguments[j], *m_substitution);
    }
    actions[i] = process::action(m_summand->action_label[i].label,
                                 data::data_expression_list(arguments.begin(), arguments.end()));
  }
  if (m_summand->time == data::data_expression())  // Check whether the time is valid.
  {
    m_transition.action = multi_action(process::action_list(actions.begin(), actions.end()));
  }
  else
  {
    m_transition.action = multi_action(process::action_list(actions.begin(), actions.end()),
                                       m_generator->m_rewriter(m_summand->time, *m_substitution));
  }

  m_transition.summand_index = m_summand - &m_generator->m_summands[0];

  for (const auto& variable: m_summand->variables)
  {
    (*m_substitution)[variable] = variable;  // Reset the variable.
  }
}

} // namespace lps

} // namespace mcrl2