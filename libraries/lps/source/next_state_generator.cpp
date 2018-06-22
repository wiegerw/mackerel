// Author(s): Wieger Wesselink 2010 - 2018
//            Ruud Koolen
// Copyright: see the accompanying file COPYING or copy at
// https://github.com/mCRL2org/mCRL2/blob/master/COPYING
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
using namespace mcrl2::data;
using namespace mcrl2::lps;
using namespace mcrl2::lps::detail;

next_state_generator::summand_subset::summand_subset(next_state_generator* generator)
  : m_generator(generator)
{
  for (std::size_t i = 0; i < generator->m_summands.size(); i++)
  {
    m_summands.push_back(i);
  }
}

next_state_generator::summand_subset::summand_subset(next_state_generator* generator, const action_summand_vector& summands)
  : m_generator(generator)
{
  std::set<action_summand> summand_set(summands.begin(), summands.end());
  for (std::size_t i = 0; i < generator->m_summands.size(); i++)
  {
    if (summand_set.find(*generator->m_summands[i].summand) != summand_set.end())
    {
      m_summands.push_back(i);
    }
  }
}

void next_state_generator::iterator::check_condition_rewrites_to_true() const
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
