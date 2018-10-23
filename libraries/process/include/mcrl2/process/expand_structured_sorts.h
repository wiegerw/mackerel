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

#include <algorithm>
#include "mcrl2/core/detail/print_utility.h"
#include "mcrl2/data/substitutions/mutable_map_substitution.h"
#include "mcrl2/process/builder.h"
#include "mcrl2/process/process_specification.h"

namespace mcrl2 {

namespace process {

namespace detail {

struct expand_structured_sorts_builder: public sort_expression_builder<expand_structured_sorts_builder>
{
  typedef sort_expression_builder<expand_structured_sorts_builder> super;
  using super::enter;
  using super::leave;
  using super::apply;

  const std::map<process_identifier, process_identifier>& process_identifier_map;
  const std::map<data::data_expression, data::data_expression>& sigma;
  const data::basic_sort& psort; // the sort of the structured sort variable p
  const data::variable_list& parameters; // the replacement for the structured sort variable p
  const data::sort_expression_list& parameter_sorts;

  expand_structured_sorts_builder(const std::map<process_identifier, process_identifier>& process_identifier_map_,
                                  const std::map<data::data_expression, data::data_expression>& sigma_,
                                  const data::basic_sort& psort_,
                                  const data::variable_list& parameters_,
                                  const data::sort_expression_list& parameter_sorts_
                                 )
   : process_identifier_map(process_identifier_map_), sigma(sigma_), psort(psort_), parameters(parameters_), parameter_sorts(parameter_sorts_)
  {}

  // replace p in the domain of each function sort
  data::sort_expression apply(const data::function_sort& x)
  {
    if (x.target_sort() == psort)
    {
      std::cout << "x = " << x << std::endl;
      throw mcrl2::runtime_error("expand_structured_sort: the expanded sort cannot be a target sort!");
    }

    std::vector<data::sort_expression> domain_sorts;
    for (const auto& s: x.domain())
    {
      if (s == psort)
      {
        domain_sorts.insert(domain_sorts.end(), parameter_sorts.begin(), parameter_sorts.end());
      }
      else
      {
        domain_sorts.push_back(s);
      }
    }
    return data::function_sort(data::sort_expression_list(domain_sorts.begin(), domain_sorts.end()), apply(x.target_sort()));
  }

  // replace p in the variables of abstractions
  data::data_expression apply(const data::abstraction& x)
  {
    data::variable_list variables = x.variables();
    if (variables.size() == 1 && variables.front().sort() == psort)
    {
      variables = parameters;
    }
    return data::abstraction(x.binding_operator(), variables, apply(x.body()));
  }

  data::data_expression apply(const data::function_symbol& x)
  {
    auto i = sigma.find(x);
    if (i != sigma.end())
    {
      return i->second;
    }
    return data::function_symbol(x.name(), apply(x.sort()));
  }

  data::data_expression apply(const data::application& x)
  {
    auto i = sigma.find(x);
    if (i != sigma.end())
    {
      return i->second;
    }

    data::application x1 = super::apply(x);

    // Replace f(p) by f(parameters)
    if (x1.size() == 1 && x1[0].sort() == psort)
    {
      return data::application(x1.head(), parameters);
    }
    return x1;
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
    auto actual_parameters = x.actual_parameters();

    if (actual_parameters.size() != 1)
    {
      throw mcrl2::runtime_error("expand_structured_sorts: unsupported case 3!");
    }
    const auto& arg = atermpp::down_cast<data::application>(actual_parameters.front());
    actual_parameters = data::data_expression_list(arg.begin(), arg.end());
    actual_parameters = super::apply(actual_parameters);
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
    data::data_expression_list actual_parameters(rhs.begin(), rhs.end());
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
    // TODO: these sorts should be normalized. This looks like a bug.
    lhs = data::normalize_sorts(lhs, procspec.data());
    rhs = data::normalize_sorts(rhs, procspec.data());
    sigma[lhs] = rhs;
  }
  return sigma;
}

inline
std::pair<data::variable_list, data::sort_expression_list> structured_sort_variables(const process_specification& procspec, const data::variable& v, const data::structured_sort& sort)
{
  std::vector<data::variable> parameters;
  std::vector<data::sort_expression> parameter_sorts;
  for (const data::structured_sort_constructor_argument& arg: sort.constructors().front().arguments())
  {
    data::function_symbol f(arg.name(), data::function_sort(data::sort_expression_list({ v.sort() }), arg.sort()));
    data::variable rhs(arg.name(), arg.sort());
    // TODO: these sorts should be normalized. This looks like a bug.
    rhs = data::normalize_sorts(rhs, procspec.data());
    parameters.push_back(rhs);
    parameter_sorts.push_back(rhs.sort());
  }
  return { data::variable_list(parameters.begin(), parameters.end()), data::sort_expression_list(parameter_sorts.begin(), parameter_sorts.end()) };
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

  data::variable p;
  data::structured_sort p_structured_sort;
  std::tie(p, p_structured_sort) = *structured_sort_parameters.begin();

  // find the basic sort corresponding to p_structured_sort
  data::variable p_normalized = data::normalize_sorts(p, procspec.data());
  const data::basic_sort& p_basic_sort = atermpp::down_cast<data::basic_sort>(p_normalized.sort());

  std::map<data::data_expression, data::data_expression> sigma = detail::make_structured_sort_substitution(procspec, p, p_structured_sort);

  data::variable_list parameters;
  data::sort_expression_list parameter_sorts;
  std::tie(parameters, parameter_sorts) = detail::structured_sort_variables(procspec, p, p_structured_sort);

  detail::expand_structured_sorts_builder f(process_identifier_map, sigma, p_basic_sort, parameters, parameter_sorts);
  f.update(procspec);

  // N.B. The data specification needs to be handled separately, because its irregular interface doesn't play
  // well with the traverser framework.
  const data::data_specification& dataspec = procspec.data();
  data::basic_sort_list sorts(dataspec.user_defined_sorts().begin(), dataspec.user_defined_sorts().begin());
  auto a = dataspec.user_defined_aliases();
  a.erase(std::remove_if(std::begin(a), std::end(a), [&](const data::alias& x) { return x.name() == p_basic_sort; }), std::end(a));
  data::alias_list aliases(a.begin(), a.end());
  aliases = f.apply(aliases);
  data::function_symbol_list constructors(dataspec.user_defined_constructors().begin(), dataspec.user_defined_constructors().end());
  constructors = f.apply(constructors);
  data::function_symbol_list mappings(dataspec.user_defined_mappings().begin(), dataspec.user_defined_mappings().end());
  mappings = f.apply(mappings);
  data::data_equation_list equations(dataspec.user_defined_equations().begin(), dataspec.user_defined_equations().end());
  equations = f.apply(equations);
  procspec.data() = data::data_specification(sorts, aliases, constructors, mappings, equations);
  process::normalize_sorts(procspec, procspec.data()); // TODO: find out if this is necessary.
}

} // namespace process

} // namespace mcrl2

#endif // MCRL2_PROCESS_EXPAND_STRUCTURED_SORTS_H
