// Author(s): Wieger Wesselink
// Copyright: see the accompanying file COPYING or copy at
// https://svn.win.tue.nl/trac/MCRL2/browser/trunk/COPYING
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
/// \file mcrl2/process/process_equation_index.h
/// \brief add your file description here.

#ifndef MCRL2_PROCESS_PROCESS_EQUATION_INDEX_H
#define MCRL2_PROCESS_PROCESS_EQUATION_INDEX_H

#include "mcrl2/process/process_specification.h"

namespace mcrl2 {

namespace process {

struct process_equation_index
{
  // maps the name of an equation to the corresponding index of the equation
  std::unordered_map<core::identifier_string, std::size_t> equation_index;
  process_specification& procspec;

  process_equation_index() = default;

  explicit process_equation_index(process_specification& procspec_)
    : procspec(procspec_)
  {
    auto const& equations = procspec_.equations();
    for (std::size_t i = 0; i < equations.size(); i++)
    {
      const auto& eqn = equations[i];
      equation_index.insert({ eqn.identifier().name(), i });
    }
  }

  /// \brief Returns the index of the equation of the variable with the given name
  std::size_t index(const core::identifier_string& name) const
  {
    auto i = equation_index.find(name);
    assert (i != equation_index.end());
    return i->second;
  }

  const process_equation& equation(const core::identifier_string& name) const
  {
    auto i = index(name);
    return procspec.equations()[i];
  }

  process_equation& equation(const core::identifier_string& name)
  {
    auto i = index(name);
    return procspec.equations()[i];
  }
};

} // namespace process

} // namespace mcrl2

#endif // MCRL2_PROCESS_PROCESS_EQUATION_INDEX_H
