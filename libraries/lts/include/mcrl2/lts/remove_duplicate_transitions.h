// Author(s): Wieger Wesselink
// Copyright: see the accompanying file COPYING or copy at
// https://svn.win.tue.nl/trac/MCRL2/browser/trunk/COPYING
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
/// \file mcrl2/lts/remove_duplicate_transitions.h
/// \brief Algorithm for removing duplicate transitions from an LTS.

#ifndef MCRL2_LTS_REMOVE_DUPLICATE_TRANSITIONS_H
#define MCRL2_LTS_REMOVE_DUPLICATE_TRANSITIONS_H

#include <set>
#include "mcrl2/lts/lts.h"

namespace mcrl2 {

namespace lts {

inline
void remove_duplicate_transitions(labeled_transition_system& ltsspec)
{
  std::set<transition> transitions(ltsspec.transitions.begin(), ltsspec.transitions.end());
  ltsspec.transitions.assign(transitions.begin(), transitions.end());
}

} // namespace lts

} // namespace mcrl2

#endif // MCRL2_LTS_REMOVE_DUPLICATE_TRANSITIONS_H
