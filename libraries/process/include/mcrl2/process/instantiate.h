// Author(s): Wieger Wesselink
// Copyright: see the accompanying file COPYING or copy at
// https://svn.win.tue.nl/trac/MCRL2/browser/trunk/COPYING
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
/// \file mcrl2/process/instantiate.h
/// \brief add your file description here.

#ifndef MCRL2_PROCESS_INSTANTIATE_H
#define MCRL2_PROCESS_INSTANTIATE_H

#include "mcrl2/data/substitutions/mutable_map_substitution.h"
#include "mcrl2/process/replace.h"
#include "mcrl2/process/rewrite.h"

namespace mcrl2 {

namespace process {

inline
process_expression instantiate(const process_instance& x, const process_equation& eqn)
{
  data::mutable_map_substitution<> sigma;
  const data::variable_list& d = eqn.formal_parameters();
  const data::data_expression_list& e = x.actual_parameters();
  auto di = d.begin();
  auto ei = e.begin();
  for (; di != d.end(); ++di, ++ei)
  {
    sigma[*di] = *ei;
  }
  return process::replace_variables_capture_avoiding(eqn.expression(), sigma, data::substitution_variables(sigma));
}

inline
process_expression instantiate(const process_instance& x, const process_equation& eqn, const data::rewriter& R)
{
  return process::rewrite(instantiate(x, eqn), R);
}

inline
process_expression instantiate(const process_instance_assignment& x, const process_equation& eqn)
{
  data::mutable_map_substitution<> sigma;
  for (const auto& a: x.assignments())
  {
    sigma[a.lhs()] = a.rhs();
  }
  return process::replace_variables_capture_avoiding(eqn.expression(), sigma, data::substitution_variables(sigma));
}

inline
process_expression instantiate(const process_instance_assignment& x, const process_equation& eqn, const data::rewriter& R)
{
  return process::rewrite(instantiate(x, eqn), R);
}

} // namespace process

} // namespace mcrl2

#endif // MCRL2_PROCESS_INSTANTIATE_H
