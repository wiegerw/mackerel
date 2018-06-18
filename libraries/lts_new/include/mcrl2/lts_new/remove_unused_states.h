// Author(s): Wieger Wesselink
// Copyright: see the accompanying file COPYING or copy at
// https://svn.win.tue.nl/trac/MCRL2/browser/trunk/COPYING
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
/// \file mcrl2/lts/remove_unused_states.h
/// \brief add your file description here.

#ifndef MCRL2_LTS_REMOVE_UNUSED_STATES_H
#define MCRL2_LTS_REMOVE_UNUSED_STATES_H

#include "mcrl2/lts_new/lts.h"

namespace mcrl2 {

namespace lts {

// Renames states such that they are in a contiguous interval [0, ..., N)
inline
void remove_unused_states(labeled_transition_system& ltsspec, std::size_t tau_label = 0)
{
  std::vector<std::size_t> replace(ltsspec.number_of_states, 0);

  // put a 1 in each position of replace that is the source or target of a transition, and the initial state
  for (const transition& t: ltsspec.transitions)
  {
    replace[t.from] = 1;
    replace[t.to] = 1;
  }
  replace[ltsspec.initial_state] = 1;

  // assign a new value to all elements 1 in replace
  std::size_t index = 0;
  for (auto i = replace.begin(); i != replace.end(); ++i)
  {
    if (*i == 1)
    {
      *i = index++;
    }
  }

  // apply replace to ltsspec
  ltsspec.initial_state = replace[ltsspec.initial_state];
  for (transition& t: ltsspec.transitions)
  {
    t.from = replace[t.from];
    t.to = replace[t.to];
  }
  ltsspec.number_of_states = index;
}

} // namespace lts

} // namespace mcrl2

#endif // MCRL2_LTS_REMOVE_UNUSED_STATES_H
