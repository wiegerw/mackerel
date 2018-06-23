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

    class summand_subset
    {
      friend class next_state_generator;
      friend class next_state_generator::iterator;

      public:
        /// \brief Trivial constructor. Constructs an invalid command subset.
        summand_subset() = default;

        /// \brief Constructs the full summand subset for the given generator.
        explicit summand_subset(next_state_generator *generator);

        /// \brief Constructs the summand subset containing the given commands.
        summand_subset(next_state_generator* generator, const action_summand_vector& summands);

      private:
        next_state_generator *m_generator = nullptr;
        std::vector<std::size_t> m_summands;
    };

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

        bool m_single_summand;
        std::size_t m_single_summand_index;
        std::vector<std::size_t>::iterator m_summand_iterator;
        std::vector<std::size_t>::iterator m_summand_iterator_end;
        next_state_summand *m_summand;

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

        iterator(next_state_generator* generator, const lps::state& state, rewriter_substitution* substitution, summand_subset& summand_subset, enumerator_queue* enumeration_queue)
          : m_generator(generator),
            m_state(state),
            m_substitution(substitution),
            m_single_summand(false),
            m_summand(nullptr),
            m_caching(false),
            m_enumeration_queue(enumeration_queue)
        {
          m_summand_iterator = summand_subset.m_summands.begin();
          m_summand_iterator_end = summand_subset.m_summands.end();

          std::size_t j = 0;
          for (auto i = state.begin(); i != state.end(); ++i, ++j)
          {
            (*m_substitution)[generator->m_process_parameters[j]] = *i;
          }

          increment();
        }

        iterator(next_state_generator* generator, const lps::state& state, rewriter_substitution* substitution, std::size_t summand_index, enumerator_queue* enumeration_queue)
          : m_generator(generator),
            m_state(state),
            m_substitution(substitution),
            m_single_summand(true),
            m_single_summand_index(summand_index),
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

        void increment();
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

    summand_subset m_all_summands;

  public:
    /// \brief Constructor
    /// \param spec The process specification
    /// \param rewriter The rewriter used
    /// \param use_enumeration_caching Cache intermediate enumeration results
    next_state_generator(const specification& spec,
                         const data::rewriter& rewriter,
                         bool use_enumeration_caching = false
                        );

    /// \brief Returns an iterator for generating the successors of the given state.
    iterator begin(const state& state, enumerator_queue* enumeration_queue)
    {
      return iterator(this, state, &m_substitution, m_all_summands, enumeration_queue);
    }

    /// \brief Returns an iterator for generating the successors of the given state.
    iterator begin(const state& state, summand_subset& summand_subset, enumerator_queue* enumeration_queue)
    {
      return iterator(this, state, &m_substitution, summand_subset, enumeration_queue);
    }

    /// \brief Returns an iterator for generating the successors of the given state.
    /// Only the successors with respect to the summand with the given index are generated.
    iterator begin(const state& state, std::size_t summand_index, enumerator_queue* enumeration_queue)
    {
      return iterator(this, state, &m_substitution, summand_index, enumeration_queue);
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

    /// \brief Returns a reference to the summand subset containing all summands.
    summand_subset& all_summands()
    {
      return m_all_summands;
    }
};

} // namespace lps

} // namespace mcrl2

#endif // MCRL2_LPS_NEXT_STATE_GENERATOR_H
