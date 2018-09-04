// Author(s): Wieger Wesselink
// Copyright: see the accompanying file COPYING or copy at
// https://svn.win.tue.nl/trac/MCRL2/browser/trunk/COPYING
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
/// \file mcrl2/process/expand_structured_sorts.h
/// \brief add your file description here.

#ifndef MCRL2_PROCESS_EXPAND_STRUCTURED_SORTS_H
#define MCRL2_PROCESS_EXPAND_STRUCTURED_SORTS_H

#include "mcrl2/data/substitutions/mutable_map_substitution.h"
#include "mcrl2/process/builder.h"
#include "mcrl2/process/process_specification.h"

namespace mcrl2 {

namespace process {

namespace detail {

struct expand_structured_sorts_builder: public data_expression_builder<expand_structured_sorts_builder>
{
  typedef data_expression_builder<expand_structured_sorts_builder> super;
  using super::enter;
  using super::leave;
  using super::apply;

  const std::map<process_identifier, process_identifier>& process_identifier_map;
  const std::map<data::data_expression, data::data_expression>& sigma;

  expand_structured_sorts_builder(const std::map<process_identifier, process_identifier>& process_identifier_map_,
                                  const std::map<data::data_expression, data::data_expression>& sigma_)
   : process_identifier_map(process_identifier_map_), sigma(sigma_)
  {}

  data::data_expression apply(const data::application& x)
  {
    auto x1 = super::apply(x); // innermost
    auto i = sigma.find(x1);
    if (i == sigma.end())
    {
      return x1;
    }
    return i->second;
  }

  process_equation apply(const process_equation& x)
  {
    auto identifier = process_identifier_map.at(x.identifier());
    auto expression = super::apply(x.expression());
    return process_equation(identifier, identifier.variables(), expression);
  }

  process_expression apply(const if_then& x)
  {
    return super::apply(x);
  }

  process_expression apply(const process_instance& x)
  {
    // TODO: generalize this code
    auto identifier = process_identifier_map.at(x.identifier());
    auto actual_parameters = super::apply(x.actual_parameters());

    if (actual_parameters.size() != 1)
    {
      throw mcrl2::runtime_error("expand_structured_sorts: unsupported case 3!");
    }
    const auto& arg = atermpp::down_cast<data::application>(actual_parameters.front());
    actual_parameters = data::data_expression_list(arg.begin(), arg.end());

    return process_instance(identifier, actual_parameters);
  }

  process_expression apply(const process_instance_assignment& x)
  {
    // TODO: generalize this code
    auto identifier = process_identifier_map.at(x.identifier());
    if (x.assignments().empty())
    {
      return process_instance_assignment(identifier, data::assignment_list());
    }
    if (x.assignments().size() > 1)
    {
      throw mcrl2::runtime_error("expand_structured_sorts: unsupported case 2!");
    }
    const data::assignment& a = x.assignments().front();
    const auto& rhs = atermpp::down_cast<data::application>(a.rhs());
    auto actual_parameters = data::data_expression_list(rhs.begin(), rhs.end());
    actual_parameters = super::apply(actual_parameters);
    return process_instance(identifier, actual_parameters);
  }
};

inline
std::map<data::variable, data::structured_sort> find_structured_sort_parameters(process_specification& procspec)
{
  std::map<data::sort_expression, data::sort_expression> alias_map;
  for (const auto& p: procspec.data().sort_alias_map())
  {
    alias_map[p.second] = p.first;
  }

  std::map<data::variable, data::structured_sort> result;

  // find structured sorts
  for (const process_equation& eqn: procspec.equations())
  {
    for (const data::variable& v: eqn.formal_parameters())
    {
      auto i = alias_map.find(v.sort());
      if (i == alias_map.end() || !data::is_structured_sort(i->second))
      {
        continue;
      }
      const auto& sort = atermpp::down_cast<data::structured_sort>(i->second);
      if (sort.constructors().size() == 1)
      {
        result[v] = sort;
      }
    }
  }
  return result;
}

inline
std::map<data::sort_expression, data::variable_list> make_structured_sort_map(const std::map<data::variable, data::structured_sort>& structured_sort_parameters)
{
  // create a mapping
  std::map<data::sort_expression, data::variable_list> result;
  for (const auto& p: structured_sort_parameters)
  {
    const data::structured_sort& sort = p.second;
    std::vector<data::variable> v;
    for (const data::structured_sort_constructor_argument& arg: sort.constructors().front().arguments())
    {
      v.emplace_back(arg.name(), arg.sort());
    }
    result[p.first.sort()] = data::variable_list(v.begin(), v.end());
  }
  return result;
}

inline
std::map<process_identifier, process_identifier> make_process_identifier_map(const process_specification& procspec,
                                                                             const std::map<data::sort_expression, data::variable_list>& structured_sort_map)
{
  std::map<process_identifier, process_identifier> result;
  for (const process_equation& eqn: procspec.equations())
  {
    std::vector<data::variable> parameters;
    for (const data::variable& v: eqn.formal_parameters())
    {
      auto i = structured_sort_map.find(v.sort());
      if (i == structured_sort_map.end())
      {
        parameters.push_back(v);
      }
      else
      {
        const auto& variables = i->second;
        parameters.insert(parameters.end(), variables.begin(), variables.end());
      }
    }
    process_identifier P(eqn.identifier().name(), data::variable_list(parameters.begin(), parameters.end()));
    result[eqn.identifier()] = P;
  }
  return result;
}

inline
std::map<data::data_expression, data::data_expression> make_structured_sort_substitution(const process_specification& procspec, const data::variable& v, const data::structured_sort& sort)
{
  std::map<data::data_expression, data::data_expression> sigma;
  for (const data::structured_sort_constructor_argument& arg: sort.constructors().front().arguments())
  {
    data::function_symbol f(arg.name(), data::function_sort(data::sort_expression_list({ v.sort() }), arg.sort()));
    data::application lhs(f, v);
    data::variable rhs(arg.name(), arg.sort());
    // TODO: find out why these sorts are not normalized yet
    lhs = data::normalize_sorts(lhs, procspec.data());
    rhs = data::normalize_sorts(rhs, procspec.data());
    sigma[lhs] = rhs;
  }
  return sigma;
}

}; // namespace detail

// Expands process parameters of type structured_sort (with one constructor)
// For example:
//
// sort S = struct S_(i: Int, b: Bool);
// proc P(s: S) = delta;
//
// will be expanded to
//
// proc P(i: Int, b: Bool) = delta;
inline
void expand_structured_sorts(process_specification& procspec)
{
  std::map<data::variable, data::structured_sort> structured_sort_parameters = detail::find_structured_sort_parameters(procspec);
  std::map<data::sort_expression, data::variable_list> structured_sort_map = detail::make_structured_sort_map(structured_sort_parameters);
  std::map<process_identifier, process_identifier> process_identifier_map = detail::make_process_identifier_map(procspec, structured_sort_map);

  if (structured_sort_parameters.size() != 1)
  {
    throw mcrl2::runtime_error("expand_structured_sorts: unsupported case 1!");
  }

  auto p = *structured_sort_parameters.begin();
  std::map<data::data_expression, data::data_expression> sigma = detail::make_structured_sort_substitution(procspec, p.first, p.second);

  detail::expand_structured_sorts_builder f(process_identifier_map, sigma);
  f.update(procspec);

  // N.B. The data specification needs to be handled separately, because its irregular interface doesn't play
  // well with the traverser framework.
  const data::data_specification& dataspec = procspec.data();
  data::basic_sort_list sorts(dataspec.user_defined_sorts().begin(), dataspec.user_defined_sorts().begin());
  data::alias_list aliases(dataspec.user_defined_aliases().begin(), dataspec.user_defined_aliases().end());
  data::function_symbol_list constructors(dataspec.user_defined_constructors().begin(), dataspec.user_defined_constructors().end());
  data::function_symbol_list mappings(dataspec.user_defined_mappings().begin(), dataspec.user_defined_mappings().end());
  data::data_equation_list equations(dataspec.user_defined_equations().begin(), dataspec.user_defined_equations().end());
  equations = f.apply(equations);
  procspec.data() = data::data_specification(sorts, aliases, constructors, mappings, equations);
}

} // namespace process

} // namespace mcrl2

#endif // MCRL2_PROCESS_EXPAND_STRUCTURED_SORTS_H
