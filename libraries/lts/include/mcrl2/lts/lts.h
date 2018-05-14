// Author(s): Wieger Wesselink
// Copyright: see the accompanying file COPYING or copy at
// https://svn.win.tue.nl/trac/MCRL2/browser/trunk/COPYING
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
/// \file mcrl2/lts/lts.h
/// \brief add your file description here.

#ifndef MCRL2_LTS_LTS_H
#define MCRL2_LTS_LTS_H

#include <cstddef>
#include <iostream>
#include <utility>
#include <vector>

namespace mcrl2
{

namespace lts
{

struct transition
{
  std::size_t from;
  std::size_t label;
  std::size_t to;

  transition(std::size_t from_, std::size_t label_, std::size_t to_)
    : from(from_), label(label_), to(to_)
  {}

  bool operator<(const transition& other) const
  {
    return std::tie(from, label, to) < std::tie(other.from, other.label, other.to);
  }
};

struct labeled_transition_system
{
  std::vector<transition> transitions;
  std::vector<std::string> action_labels;
  std::size_t initial_state;
  std::size_t number_of_states;
};

inline
std::ostream& operator<<(std::ostream& out, const labeled_transition_system& x)
{
  out << "des (" << x.initial_state << ',' << x.transitions.size() << ',' << x.number_of_states << ")\n";
  for (const transition& t: x.transitions)
  {
    out << '(' << t.from << ",\"" << x.action_labels[t.label] << "\"," << t.to << ")\n";
  }
  return out;
}

} // namespace lts

} // namespace mcrl2

#endif // MCRL2_LTS_LTS_H
