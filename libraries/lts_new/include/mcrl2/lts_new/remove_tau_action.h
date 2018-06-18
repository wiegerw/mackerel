// Author(s): Wieger Wesselink
// Copyright: see the accompanying file COPYING or copy at
// https://svn.win.tue.nl/trac/MCRL2/browser/trunk/COPYING
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
/// \file mcrl2/lts/remove_tau_action.h
/// \brief Algorithm for removing tau actions from an LTS.

#ifndef MCRL2_LTS_REMOVE_TAU_ACTION_H
#define MCRL2_LTS_REMOVE_TAU_ACTION_H

#include <map>
#include "mcrl2/lts_new/lts.h"
#include "mcrl2/lts_new/remove_duplicate_transitions.h"
#include "mcrl2/lts_new/remove_unused_states.h"

namespace mcrl2 {

namespace lts {

// Joins states that are connected using a transition with label tau_label.
inline
void remove_tau_action(labeled_transition_system& ltsspec, std::size_t tau_label = 0)
{
  // compute a map of replacements
  std::map<std::size_t, std::size_t> replacements;
  for (const transition& t: ltsspec.transitions)
  {
    if (t.label != tau_label)
    {
      continue;
    }
    auto i = replacements.find(t.to);
    if (i == replacements.end())
    {
      replacements[t.to] = t.from;
    }
    else
    {
      replacements[t.to] = i->second;
    }
  }

  auto replace = [&](std::size_t n)
    {
      auto i = replacements.find(n);
      if (i == replacements.end())
      {
        return n;
      }
      return i->second;
    };

  // compute new transitions
  std::vector<transition> transitions;
  for (const transition& t: ltsspec.transitions)
  {
    if (t.label == tau_label)
    {
      continue;
    }
    transitions.emplace_back(replace(t.from), t.label, replace(t.to));
  }
  std::swap(ltsspec.transitions, transitions);

  // update initial state
  ltsspec.initial_state = replace(ltsspec.initial_state);

  // there may be duplicate transitions, so remove them.
  remove_duplicate_transitions(ltsspec);

  // make sure the states are in a contiguous interval
  remove_unused_states(ltsspec);
}

} // namespace lts

} // namespace mcrl2

#endif // MCRL2_LTS_REMOVE_TAU_ACTION_H
