// Author(s): Wieger Wesselink 2010 - 2018
//            Ruud Koolen
// Copyright: see the accompanying file COPYING or copy at
// https://github.com/mCRL2org/mCRL2/blob/master/COPYING
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
/// \file next_state_generator.h

#ifndef MCRL2_LPS_NEXT_STATE_GENERATOR_H
#define MCRL2_LPS_NEXT_STATE_GENERATOR_H

#include <boost/iterator/iterator_facade.hpp>
#include <forward_list>
#include <iterator>
#include <string>
#include <vector>

#include "mcrl2/atermpp/detail/shared_subset.h"
#include "mcrl2/data/enumerator.h"
#include "mcrl2/lps/state.h"
#include "mcrl2/lps/specification.h"

namespace mcrl2 {

namespace lps {

class next_state_generator
{
  public:
    typedef atermpp::term_appl<data::data_expression> enumeration_cache_key;
    typedef std::list<data::data_expression_list> enumeration_cache_value;
    typedef data::enumerator_algorithm_with_iterator<> enumerator;
    typedef std::deque<data::enumerator_list_element_with_substitution<>> enumerator_queue;
    typedef data::rewriter::substitution_type rewriter_substitution;

  protected:
    struct next_state_action_label
    {
      process::action_label label;
      data::data_expression_vector arguments;
    };

    struct next_state_summand
    {
      action_summand* summand;
      data::variable_list variables;
      data::data_expression condition;
      data::data_expression_vector result_state;
      std::vector<next_state_action_label> action_label;
      data::data_expression time;

      std::vector<std::size_t> condition_parameters;
      atermpp::function_symbol condition_arguments_function;
      std::map<enumeration_cache_key, enumeration_cache_value> enumeration_cache;
    };

  public:
    class iterator;

    struct transition
    {
      lps::multi_action action;
      lps::state target_state;
      std::size_t summand_index;
    };

    class iterator: public boost::iterator_facade<iterator, const transition, boost::forward_traversal_tag>
    {
      protected:
        transition m_transition;
        next_state_generator* m_generator = nullptr;
        lps::state m_state;
        rewriter_substitution* m_substitution;

        std::vector<next_state_summand>::iterator m_summands_first;
        std::vector<next_state_summand>::iterator m_summands_last;
        next_state_summand* m_summand;

        bool m_cached;
        enumeration_cache_value::iterator m_enumeration_cache_iterator;
        enumeration_cache_value::iterator m_enumeration_cache_end;
        enumerator::iterator m_enumeration_iterator;
        bool m_caching;
        enumeration_cache_key m_enumeration_cache_key;
        enumeration_cache_value m_enumeration_log;
        enumerator_queue* m_enumeration_queue;

        /// \brief Enumerate <variables, phi> with substitution sigma.
        void enumerate(const data::variable_list& variables, const data::data_expression& phi, data::mutable_indexed_substitution<>& sigma)
        {
          m_enumeration_queue->clear();
          m_enumeration_queue->push_back(data::enumerator_list_element_with_substitution<>(variables, phi));
          try
          {
            m_enumeration_iterator = m_generator->m_enumerator.begin(sigma, *m_enumeration_queue);
          }
          catch (mcrl2::runtime_error &e)
          {
            throw mcrl2::runtime_error(std::string(e.what()) + "\nProblem occurred when enumerating variables " + data::pp(variables) + " in " + data::pp(phi));
          }
        }


      public:
        iterator() = default;

        iterator(next_state_generator* generator,
                 const lps::state& state,
                 std::vector<next_state_summand>::iterator summands_first,
                 std::vector<next_state_summand>::iterator summands_last,
                 rewriter_substitution* substitution,
                 enumerator_queue* enumeration_queue
                )
                : m_generator(generator),
                  m_state(state),
                  m_substitution(substitution),
                  m_summands_first(summands_first),
                  m_summands_last(summands_last),
                  m_summand(nullptr),
                  m_caching(false),
                  m_enumeration_queue(enumeration_queue)
        {
          std::size_t j = 0;
          for (auto i = state.begin(); i != state.end(); ++i, ++j)
          {
            (*m_substitution)[generator->m_process_parameters[j]] = *i;
          }
          increment();
        }

        explicit operator bool() const
        {
          return m_generator != nullptr;
        }

      private:
        friend class boost::iterator_core_access;

        bool equal(const iterator& other) const
        {
          return (!(bool)*this && !(bool)other) || (this == &other);
        }

        const transition& dereference() const
        {
          return m_transition;
        }

        void check_condition_rewrites_to_true() const
        {
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
        }

        void increment()
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

            if (m_summands_first == m_summands_last)
            {
              m_generator = nullptr;
              return;
            }
            m_summand = &(*m_summands_first++);

            if (m_generator->m_use_enumeration_caching)
            {
              m_enumeration_cache_key = enumeration_cache_key(m_summand->condition_arguments_function,
                                                              m_summand->condition_parameters.begin(),
                                                              m_summand->condition_parameters.end(),
                                                              [&](const std::size_t n)
                                                              {
                                                                return m_state.element_at(n, m_generator->m_process_parameters.size());
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
            for (auto i = m_summand->variables.begin(); i != m_summand->variables.end(); i++, v++)
            {
              (*m_substitution)[*i] = *v;
            }
          }
          else
          {
            m_enumeration_iterator->add_assignments(m_summand->variables, *m_substitution, m_generator->m_rewriter);

            check_condition_rewrites_to_true();

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
          m_transition.target_state = lps::state(state_args.begin(), state_args.size(), [&](const data::data_expression& x) { return m_generator->m_rewriter(x, *m_substitution); });

          std::vector<process::action> actions;
          actions.resize(m_summand->action_label.size());
          std::vector<data::data_expression> arguments;
          for (std::size_t i = 0; i < m_summand->action_label.size(); i++)
          {
            arguments.resize(m_summand->action_label[i].arguments.size());
            for (std::size_t j = 0; j < m_summand->action_label[i].arguments.size(); j++)
            {
              arguments[j] = m_generator->m_rewriter(m_summand->action_label[i].arguments[j], *m_substitution);
            }
            actions[i] = process::action(m_summand->action_label[i].label, data::data_expression_list(arguments.begin(), arguments.end()));
          }
          if (m_summand->time == data::data_expression())  // Check whether the time is valid.
          {
            m_transition.action = multi_action(process::action_list(actions.begin(), actions.end()));
          }
          else
          {
            m_transition.action =
              multi_action(process::action_list(actions.begin(), actions.end()), m_generator->m_rewriter(m_summand->time, *m_substitution));
          }

          m_transition.summand_index = m_summand - &m_generator->m_summands[0];

          for (const auto& variable: m_summand->variables)
          {
            (*m_substitution)[variable] = variable;  // Reset the variable.
          }
        }
    };

  protected:
    specification m_specification;
    data::rewriter m_rewriter;
    rewriter_substitution m_substitution;
    data::enumerator_identifier_generator m_id_generator;
    enumerator m_enumerator;

    bool m_use_enumeration_caching;

    data::variable_vector m_process_parameters;
    std::vector<next_state_summand> m_summands;
    lps::state m_initial_state;

  public:
    /// \brief Constructor
    /// \param spec The process specification
    /// \param rewriter The rewriter used
    /// \param use_enumeration_caching Cache intermediate enumeration results
    next_state_generator(const specification& spec,
                         const data::rewriter& rewriter,
                         bool use_enumeration_caching = false
                        )
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

        for (std::size_t j = 0; j < m_process_parameters.size(); j++)
        {
          if (data::search_free_variable(action_summand.condition(), m_process_parameters[j]))
          {
            summand.condition_parameters.push_back(j);
          }
        }
        summand.condition_arguments_function = atermpp::function_symbol("condition_arguments", summand.condition_parameters.size());
        std::vector<atermpp::aterm_int> dummy(summand.condition_arguments_function.arity(), atermpp::aterm_int(static_cast<std::size_t>(0)));

        m_summands.push_back(summand);
      }

      data::data_expression_list initial_state_raw = m_specification.initial_process().state(m_specification.process().process_parameters());
      data::mutable_indexed_substitution<> sigma;
      m_initial_state = state(initial_state_raw.begin(),initial_state_raw.size(), [&](const data::data_expression& x) { return m_rewriter(x, m_substitution); });
    }

    /// \brief Returns an iterator for generating the successors of the given state.
    iterator begin(const state& state, enumerator_queue* enumeration_queue)
    {
      // return iterator(this, state, &m_substitution, enumeration_queue);
      return iterator(this, state, m_summands.begin(), m_summands.end(), &m_substitution, enumeration_queue);
    }

    /// \brief Returns an iterator for generating the successors of the given state.
    /// Only the successors with respect to the summand with the given index are generated.
    iterator begin(const state& state, std::size_t summand_index, enumerator_queue* enumeration_queue)
    {
      // return iterator(this, state, &m_substitution, summand_index, enumeration_queue);
      return iterator(this, state, m_summands.begin() + summand_index, m_summands.begin() + summand_index + 1, &m_substitution, enumeration_queue);
    }

    /// \brief Returns an iterator pointing to the end of a next state list.
    iterator end()
    {
      return iterator();
    }

    /// \brief Gets the initial state.
    lps::state initial_state() const
    {
      return m_initial_state;
    }

    /// \brief Returns the rewriter associated with this generator.
    data::rewriter& rewriter()
    {
      return m_rewriter;
    }
};

} // namespace lps

} // namespace mcrl2

#endif // MCRL2_LPS_NEXT_STATE_GENERATOR_H
