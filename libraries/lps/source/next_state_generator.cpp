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
