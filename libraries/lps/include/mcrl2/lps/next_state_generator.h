// Author(s): Ruud Koolen
// Copyright: see the accompanying file COPYING or copy at
// https://svn.win.tue.nl/trac/MCRL2/browser/trunk/COPYING
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
      atermpp::aterm_appl condition_arguments_function_dummy;
      std::map<enumeration_cache_key, enumeration_cache_value> enumeration_cache;
    };

    struct pruning_tree_node
    {
      atermpp::detail::shared_subset<next_state_summand> summand_subset;
      std::map<data::data_expression, pruning_tree_node> children;
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
        summand_subset(next_state_generator* generator, bool use_summand_pruning);

        /// \brief Constructs the summand subset containing the given commands.
        summand_subset(next_state_generator* generator, const action_summand_vector& summands, bool use_summand_pruning);

      private:
        next_state_generator* m_generator;
        bool m_use_summand_pruning;

        std::vector<std::size_t> m_summands;

        pruning_tree_node m_pruning_tree;
        std::vector<std::size_t> m_pruning_parameters;
        rewriter_substitution m_pruning_substitution;

        static bool
        summand_set_contains(const std::set<action_summand>& summand_set, const next_state_summand& summand);

        void build_pruning_parameters(const action_summand_vector& summands);

        bool is_not_false(const next_state_summand& summand);

        atermpp::detail::shared_subset<next_state_summand>::iterator begin(const lps::state& state);
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
        bool m_use_summand_pruning;
        std::vector<std::size_t>::iterator m_summand_iterator;
        std::vector<std::size_t>::iterator m_summand_iterator_end;
        atermpp::detail::shared_subset<next_state_summand>::iterator m_summand_subset_iterator;
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
        void enumerate(const data::variable_list& variables, const data::data_expression& phi,
                       data::mutable_indexed_substitution<>& sigma)
        {
          m_enumeration_queue->clear();
          m_enumeration_queue->push_back(data::enumerator_list_element_with_substitution<>(variables, phi));
          try
          {
            m_enumeration_iterator = m_generator->m_enumerator.begin(sigma, *m_enumeration_queue);
          }
          catch (mcrl2::runtime_error& e)
          {
            throw mcrl2::runtime_error(
                    std::string(e.what()) + "\nProblem occurred when enumerating variables " + data::pp(variables) +
                    " in " + data::pp(phi));
          }
        }


      public:
        iterator() = default;

        iterator(next_state_generator* generator, const lps::state& state, rewriter_substitution* substitution,
                 summand_subset& summand_subset, enumerator_queue* enumeration_queue);

        iterator(next_state_generator* generator, const lps::state& state, rewriter_substitution* substitution,
                 std::size_t summand_index, enumerator_queue* enumeration_queue);

        explicit operator bool() const
        {
          return m_generator != nullptr;
        }

      private:
        friend class boost::iterator_core_access;

        bool equal(const iterator& other) const
        {
          return (!(bool) *this && !(bool) other) || (this == &other);
        }

        const transition& dereference() const
        {
          return m_transition;
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

    summand_subset m_all_summands;

  public:
    /// \brief Constructor
    /// \param spec The process specification
    /// \param rewriter The rewriter used
    /// \param use_enumeration_caching Cache intermediate enumeration results
    /// \param use_summand_pruning Preprocess summands using pruning strategy.
    next_state_generator(const specification& spec,
                         const data::rewriter& rewriter,
                         bool use_enumeration_caching = false,
                         bool use_summand_pruning = false);

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

    /// \brief Returns the rewriter associated with this generator.
    data::rewriter& rewriter()
    {
      return m_rewriter;
    }

    /// \brief Returns a reference to the summand subset containing all summands.
    summand_subset& full_subset()
    {
      return m_all_summands;
    }
};

} // namespace lps

} // namespace mcrl2

#endif // MCRL2_LPS_NEXT_STATE_GENERATOR_H
