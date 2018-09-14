// Author(s): Wieger Wesselink
// Copyright: see the accompanying file COPYING or copy at
// https://svn.win.tue.nl/trac/MCRL2/browser/trunk/COPYING
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
/// \file mcrl2/process/linearize.h
/// \brief add your file description here.

#ifndef MCRL2_PROCESS_LINEARIZE_H
#define MCRL2_PROCESS_LINEARIZE_H

#include "mcrl2/data/representative_generator.h"
#include "mcrl2/data/rewriter.h"
#include "mcrl2/data/set_identifier_generator.h"
#include "mcrl2/lps/specification.h"
#include "mcrl2/lps/linearise.h"
#include "mcrl2/process/builder.h"
#include "mcrl2/process/eliminate_single_usage_equations.h"
#include "mcrl2/process/expand_structured_sorts.h"
#include "mcrl2/process/join.h"
#include "mcrl2/process/process_specification.h"
#include "mcrl2/process/remove_data_parameters_restricted.h"
#include "mcrl2/process/rewrite.h"
#include "mcrl2/utilities/execution_timer.h"

namespace mcrl2 {

namespace process {

namespace detail {

inline
process_expression make_if_then(const data::data_expression& b, const process_expression& x)
{
  if (is_if_then(x))
  {
    const auto& x_ = atermpp::down_cast<if_then>(x);
    return if_then(data::and_(b, x_.condition()), x_.then_case());
  }
  else if (is_choice(x))
  {
    std::vector<process_expression> summands = split_summands(x);
    for (auto& summand: summands)
    {
      summand = make_if_then(b, summand);
    }
    return join_summands(summands.begin(), summands.end());
  }
  else if (is_sum(x))
  {
    const auto& x_ = atermpp::down_cast<sum>(x);
    auto operand = make_if_then(b, x_.operand());
    return sum(x_.variables(), operand);
  }
  return if_then(b, x);
}

inline
process_expression make_sum(const data::variable_list& variables, const process_expression& x)
{
  if (is_choice(x))
  {
    std::vector<process_expression> summands = split_summands(x);
    for (auto& summand: summands)
    {
      summand = sum(variables, summand);
    }
    return join_summands(summands.begin(), summands.end());
  }
  return sum(variables, x);
}

struct balance_process_parameters_builder: public process_expression_builder<balance_process_parameters_builder>
{
  typedef process_expression_builder<balance_process_parameters_builder> super;
  using super::enter;
  using super::leave;
  using super::apply;

  std::map<process_identifier, process_identifier> identifiers;
  bool updationg_initial_state = false;
  std::map<data::sort_expression, data::data_expression> default_values;

  process_expression apply(const process::process_instance& x)
  {
    if (!updationg_initial_state)
    {
      throw mcrl2::runtime_error("Unexpected process instance");
    }

    std::map<data::variable, data::data_expression> m;
    const data::variable_list& d = x.identifier().variables();
    const data::data_expression_list& e = x.actual_parameters();
    auto di = d.begin();
    auto ei = e.begin();
    for (; di != d.end(); ++di, ++ei)
    {
      m[*di] = *ei;
    }

    std::vector<data::data_expression> e1;
    const process_identifier& identifier = identifiers[x.identifier()];
    for (const data::variable& v: identifier.variables())
    {
      auto i = m.find(v);
      if (i == m.end())
      {
        e1.push_back(default_values[v.sort()]);
      }
      else
      {
        e1.push_back(i->second);
      }
    }
    return process_instance(identifier, data::data_expression_list(e1.begin(), e1.end()));
  }

  process_expression apply(const process::process_instance_assignment& x)
  {
    const process_identifier& identifier = identifiers[x.identifier()];
    return process_instance_assignment(identifier, x.assignments());
  }

  process_equation apply(const process::process_equation& x)
  {
    const process_identifier& identifier = identifiers[x.identifier()];
    return process_equation(identifier, identifier.variables(), apply(x.expression()));
  }

  void initialize(const process::process_specification& x)
  {
    // initialize identifiers
    std::set<data::variable> all_variables;
    data::variable_list longest_list;
    for (const process_equation& eqn: x.equations())
    {
      if (eqn.formal_parameters().size() > longest_list.size())
      {
        longest_list = eqn.formal_parameters();
      }
      all_variables.insert(eqn.formal_parameters().begin(), eqn.formal_parameters().end());
    }
    for (const data::variable& v: longest_list)
    {
      all_variables.erase(v);
    }
    for (const data::variable& v: all_variables)
    {
      longest_list.push_front(v);
    }
    for (const process_equation& eqn: x.equations())
    {
      identifiers[eqn.identifier()] = process_identifier(eqn.identifier().name(), longest_list);
    }

    // initialize default_values
    data::representative_generator generator(x.data());
    for (const data::variable& v: longest_list)
    {
      if (default_values.find(v.sort()) == default_values.end())
      {
        default_values[v.sort()] = generator(v.sort());
      }
    }
  }

  void update(process::process_specification& x)
  {
    initialize(x);

    updationg_initial_state = true;
    x.init() = apply(x.init());

    updationg_initial_state = false;
    for (process_equation& eqn: x.equations())
    {
      eqn = apply(eqn);
    }
  }
};

// Applies the rules
//     a.(x + y)  -> a.Q, with Q = x + y a new equation.
//     a.(b -> x) -> a.Q, with Q = b -> x a new equation.
// Precondition: all process equations have the same process parameters.
// Precondition: the list of equations is not empty.
struct remove_sequential_composition_builder: public process_expression_builder<remove_sequential_composition_builder>
{
  typedef process_expression_builder<remove_sequential_composition_builder> super;
  using super::enter;
  using super::leave;
  using super::apply;

  data::set_identifier_generator generator;
  std::vector<process_equation> additional_equations;
  data::variable_list process_parameters;

  process_expression apply(const seq& x)
  {
    auto left = apply(x.left());
    auto right = apply(x.right());
    if (is_action(left) && (is_choice(right) || is_if_then(right)))
    {
      process_identifier Q(generator("Q"), process_parameters);
      additional_equations.emplace_back(Q, process_parameters, right);
      return seq(left, process_instance_assignment(Q, {}));
    }
    return seq(left, right);
  }

  void update(process::process_specification& x)
  {
    for (const auto& eqn: x.equations())
    {
      generator.add_identifier(eqn.identifier().name());
    }
    process_parameters = x.equations().front().identifier().variables();

    x.init() = apply(x.init());
    for (process_equation& eqn: x.equations())
    {
      eqn = apply(eqn);
    }

    for (const auto& eqn: additional_equations)
    {
      x.equations().push_back(eqn);
    }
  }
};

struct convert_process_instances_builder: public process_expression_builder<convert_process_instances_builder>
{
  typedef process_expression_builder<convert_process_instances_builder> super;
  using super::apply;

  std::map<process_identifier, const process_equation*> equation_index;

  explicit convert_process_instances_builder(process_specification& procspec)
  {
    for (const process_equation& eqn: procspec.equations())
    {
      equation_index[eqn.identifier()] = &eqn;
    }
  }

  process_expression apply(const process::process_instance& x)
  {
    const data::variable_list& d = equation_index.find(x.identifier())->second->formal_parameters();
    const data::data_expression_list& e = x.actual_parameters();
    auto di = d.begin();
    auto ei = e.begin();
    std::vector<data::assignment> assignments;
    for (; di != d.end(); ++di, ++ei)
    {
      if (*di != *ei)
      {
        assignments.emplace_back(*di, *ei);
      }
    }
    return process_instance_assignment(x.identifier(), data::assignment_list(assignments.begin(), assignments.end()));
  }
};

struct join_processes_builder: public process_expression_builder<join_processes_builder>
{
  typedef process_expression_builder<join_processes_builder> super;
  using super::apply;

  std::map<process_identifier, std::size_t> process_index;
  process_identifier P;
  data::variable phase_variable;

  process_expression apply(const process::process_instance& x)
  {
    data::data_expression phase = data::sort_nat::nat(std::to_string(process_index[x.identifier()]));
    data::data_expression_list e = x.actual_parameters();
    e.push_front(phase);
    return process_instance(P, e);
  }

  process_expression apply(const process::process_instance_assignment& x)
  {
    data::data_expression phase = data::sort_nat::nat(std::to_string(process_index[x.identifier()]));
    data::assignment_list a = x.assignments();
    a.push_front(data::assignment(phase_variable, phase));
    return process_instance_assignment(P, a);
  }

  void update(process::process_specification& procspec)
  {
    // initialize P, phase_variable, process_index
    std::size_t i = 0;
    for (const process_equation& eqn: procspec.equations())
    {
      process_index[eqn.identifier()] = i++;
    }
    phase_variable = data::variable(core::identifier_string("phase"), data::sort_nat::nat());
    data::variable_list variables = procspec.equations().front().formal_parameters();
    variables.push_front(phase_variable);
    P = process_identifier(core::identifier_string("P"), variables);

    std::vector<process_expression> summands;
    for (const process_equation& eqn: procspec.equations())
    {
      data::data_expression phase = data::sort_nat::nat(std::to_string(process_index[eqn.identifier()]));
      data::data_expression condition = data::equal_to(phase_variable, phase);
      summands.push_back(detail::make_if_then(condition, apply(eqn.expression())));
    }
    process_equation equation(P, P.variables(), join_summands(summands.begin(), summands.end()));
    procspec.init() = apply(procspec.init());
    procspec.equations() = { equation };
  }
};

// b -> x <> y   =>   (b -> x) + (!b -> y)
// b -> (c -> x) =>   (b && c) -> x
// b -> (x + y)  =>   (b -> x) + (b -> y)
struct expand_if_then_else_builder: public process_expression_builder<expand_if_then_else_builder>
{
  typedef process_expression_builder<expand_if_then_else_builder> super;
  using super::apply;

  process_expression apply(const process::if_then_else& x)
  {
    process_expression then_case = apply(x.then_case());
    process_expression else_case = apply(x.else_case());
    auto result = choice(make_if_then(x.condition(), then_case), make_if_then(data::not_(x.condition()), else_case));
    return result;
  }

  process_expression apply(const process::if_then& x)
  {
    process_expression then_case = apply(x.then_case());
    return make_if_then(x.condition(), then_case);
  }
};

// sum v. (x + y) => (sum v. x) + (sum v. y)
struct expand_sum_builder: public process_expression_builder<expand_sum_builder>
{
  typedef process_expression_builder<expand_sum_builder> super;
  using super::apply;

  process_expression apply(const process::sum& x)
  {
    process_expression operand = apply(x.operand());
    return make_sum(x.variables(), operand);
  }
};

inline
process_expression make_guarded(const process_expression& x, const process_expression& dummy = tau())
{
  if (process::is_action(x))
  {
    return x;
  }
  else if (process::is_process_instance(x))
  {
    return seq(dummy, x);
  }
  else if (process::is_process_instance_assignment(x))
  {
    return seq(dummy, x);
  }
  else if (process::is_delta(x))
  {
    return x;
  }
  else if (process::is_tau(x))
  {
    return x;
  }
  else if (process::is_sum(x))
  {
    const auto& x_ = atermpp::down_cast<process::sum>(x);
    return sum(x_.variables(), make_guarded(x_.operand(), dummy));
  }
  else if (process::is_block(x))
  {
    const auto& x_ = atermpp::down_cast<process::block>(x);
    return block(x_.block_set(), make_guarded(x_.operand(), dummy));
  }
  else if (process::is_hide(x))
  {
    const auto& x_ = atermpp::down_cast<process::hide>(x);
    return hide(x_.hide_set(), make_guarded(x_.operand(), dummy));
  }
  else if (process::is_rename(x))
  {
    const auto& x_ = atermpp::down_cast<process::rename>(x);
    return rename(x_.rename_set(), make_guarded(x_.operand(), dummy));
  }
  else if (process::is_comm(x))
  {
    const auto& x_ = atermpp::down_cast<process::comm>(x);
    return comm(x_.comm_set(), make_guarded(x_.operand(), dummy));
  }
  else if (process::is_allow(x))
  {
    const auto& x_ = atermpp::down_cast<process::allow>(x);
    return allow(x_.allow_set(), make_guarded(x_.operand(), dummy));
  }
  else if (process::is_sync(x))
  {
    const auto& x_ = atermpp::down_cast<process::sync>(x);
    return sync(make_guarded(x_.left(), dummy), make_guarded(x_.right(), dummy));
  }
  else if (process::is_at(x))
  {
    const auto& x_ = atermpp::down_cast<process::at>(x);
    return at(make_guarded(x_.operand(), dummy), x_.time_stamp());
  }
  else if (process::is_seq(x))
  {
    const auto& x_ = atermpp::down_cast<process::seq>(x);
    if (is_action(x_.left()))
    {
      return x;
    }
    return seq(x_.left(), make_guarded(x_.right(), dummy));
  }
  else if (process::is_if_then(x))
  {
    const auto& x_ = atermpp::down_cast<process::if_then>(x);
    return if_then(x_.condition(), make_guarded(x_.then_case(), dummy));
  }
  else if (process::is_if_then_else(x))
  {
    const auto& x_ = atermpp::down_cast<process::if_then_else>(x);
    return if_then_else(x_.condition(), make_guarded(x_.then_case(), dummy), make_guarded(x_.else_case(), dummy));
  }
  else if (process::is_merge(x))
  {
    const auto& x_ = atermpp::down_cast<process::merge>(x);
    return merge(make_guarded(x_.left(), dummy), make_guarded(x_.right(), dummy));
  }
  else if (process::is_left_merge(x))
  {
    const auto& x_ = atermpp::down_cast<process::left_merge>(x);
    return left_merge(make_guarded(x_.left(), dummy), make_guarded(x_.right(), dummy));
  }
  else if (process::is_choice(x))
  {
    const auto& x_ = atermpp::down_cast<process::choice>(x);
    return choice(make_guarded(x_.left(), dummy), make_guarded(x_.right(), dummy));
  }
  throw mcrl2::runtime_error("make_guarded: unsupported case");
}

} // namespace detail

inline
void balance_process_parameters(process_specification& procspec)
{
  detail::balance_process_parameters_builder f;
  f.update(procspec);
}

inline
void convert_process_instances(process_specification& procspec)
{
  detail::convert_process_instances_builder f(procspec);
  f.update(procspec);
}

inline
void expand_if_then_else(process_specification& procspec)
{
  detail::expand_if_then_else_builder f;
  f.update(procspec);
}

inline
void expand_sum(process_specification& procspec)
{
  detail::expand_sum_builder f;
  f.update(procspec);
}

inline
void join_processes(process_specification& procspec)
{
  detail::join_processes_builder f;
  f.update(procspec);
}

inline
void make_guarded(process_specification& procspec)
{
  for (process_equation& eqn: procspec.equations())
  {
    eqn = process_equation(eqn.identifier(), eqn.formal_parameters(), detail::make_guarded(eqn.expression(), tau()));
  }
}

inline
void remove_sequential_composition(process_specification& procspec)
{
  detail::remove_sequential_composition_builder f;
  f.update(procspec);
}

inline
void log_process_specification(const process_specification& procspec, const std::string& msg)
{
  mCRL2log(log::debug) << "\n--- " << msg << " ---\n" << remove_data_parameters_restricted(procspec) << std::endl;
  // mCRL2log(log::debug) << "\n--- " << msg << " ---\n" << procspec << std::endl;
}

inline
lps::specification linearize(process_specification procspec, bool expand_structured_sorts = false, int max_equation_usage = 1)
{
  utilities::execution_timer timer;

  log_process_specification(procspec, "linearize");

  mCRL2log(log::verbose) << "Rewrite process specification" << std::endl;
  data::rewriter R(procspec.data());
  timer.start("rewriting");
  process::rewrite(procspec, R);
  timer.finish("rewriting");
  log_process_specification(procspec, "rewrite");

  if (expand_structured_sorts)
  {
    mCRL2log(log::verbose) << "Expand structured sorts" << std::endl;
    timer.start("expand structured sorts");
    process::expand_structured_sorts(procspec);
    timer.finish("expand structured sorts");
    log_process_specification(procspec, "expand_structured_sorts");
  }

  mCRL2log(log::verbose) << "Eliminate equations" << std::endl;
  timer.start("eliminate equations");
  eliminate_multiple_usage_equations(procspec, max_equation_usage);
  timer.finish("eliminate equations");
  log_process_specification(procspec, "eliminate_equations");

  mCRL2log(log::verbose) << "Expand if/then/else" << std::endl;
  timer.start("expand if/then/else");
  expand_if_then_else(procspec);
  timer.finish("expand if/then/else");
  log_process_specification(procspec, "expand_if_then_else");

  mCRL2log(log::verbose) << "Expand sum" << std::endl;
  timer.start("expand sum");
  expand_sum(procspec);
  timer.finish("expand sum");
  log_process_specification(procspec, "expand_sum");

  mCRL2log(log::verbose) << "Convert process instances" << std::endl;
  timer.start("convert process instances");
  convert_process_instances(procspec);
  timer.finish("convert process instances");

  mCRL2log(log::verbose) << "Balance process parameters" << std::endl;
  timer.start("balance process parameters");
  balance_process_parameters(procspec);
  timer.finish("balance process parameters");

  mCRL2log(log::verbose) << "Remove sequential composition" << std::endl;
  timer.start("remove sequential composition");
  remove_sequential_composition(procspec);
  timer.finish("remove sequential composition");

  mCRL2log(log::verbose) << "Make process guarded" << std::endl;
  timer.start("make process guarded");
  make_guarded(procspec);
  timer.finish("make process guarded");
  log_process_specification(procspec, "make_guarded");

  mCRL2log(log::verbose) << "Join processes" << std::endl;
  timer.start("join processes");
  join_processes(procspec);
  timer.finish("join processes");
  log_process_specification(procspec, "join_processes");

  mCRL2log(log::verbose) << "Create LPS" << std::endl;
  timer.start("create lps");
  lps::specification lpsspec;
  lps::linear_process& process = lpsspec.process();
  process.process_parameters() = procspec.equations().front().formal_parameters();

  std::vector<lps::action_summand> action_summands;
  for (process_expression& summand: split_summands(procspec.equations().front().expression()))
  {
    process_expression x = summand;

    data::variable_list summation_variables;
    data::data_expression condition;
    lps::multi_action a;
    data::assignment_list assignments;

    if (is_sum(x))
    {
      const auto& x_ = atermpp::down_cast<sum>(x);
      summation_variables = x_.variables();
      x = x_.operand();
    }
    if (is_if_then(x))
    {
      const auto& x_ = atermpp::down_cast<if_then>(x);
      condition = x_.condition();
      x = x_.then_case();
    }
    if (is_seq(x))
    {
      const auto& x_ = atermpp::down_cast<seq>(x);
      if (is_tau(x_.left()))
      {
        a = lps::multi_action(action_list());
      }
      else if (is_action(x_.left()))
      {
        a = lps::multi_action(action_list({ atermpp::down_cast<action>(x_.left()) }));
      }
      else
      {
        throw mcrl2::runtime_error("mcrl3linearize: unexpected action " + process::pp(remove_data_parameters_restricted(summand)));
      }
      x = x_.right();
    }
    if (is_process_instance_assignment(x))
    {
      const auto& x_ = atermpp::down_cast<process_instance_assignment>(x);
      assignments = x_.assignments();
    }
    else
    {
      throw mcrl2::runtime_error("mcrl3linearize: unexpected expression " + process::pp(remove_data_parameters_restricted(summand)));
    }
    action_summands.emplace_back(summation_variables, condition, a, assignments);
  }
  process.action_summands() = action_summands;

  lpsspec.data() = procspec.data();
  lpsspec.action_labels() = procspec.action_labels();

  process_expression p_init = procspec.init();
  if (is_hide(p_init))
  {
    p_init = atermpp::down_cast<hide>(p_init).operand();
  }
  const auto& init = atermpp::down_cast<process_instance_assignment>(p_init);
  lpsspec.initial_process() = lps::process_initializer(init.assignments());
  timer.finish("create lps");

  timer.report();

  return lpsspec;
}

inline
lps::specification mcrl32lps(process_specification procspec, bool expand_structured_sorts = false, int max_equation_usage = 1)
{
  utilities::execution_timer timer;

  log_process_specification(procspec, "mcrl32lps");

  mCRL2log(log::verbose) << "Rewrite process specification" << std::endl;
  data::rewriter R(procspec.data());
  timer.start("rewriting");
  process::rewrite(procspec, R);
  timer.finish("rewriting");
  log_process_specification(procspec, "rewrite");

  if (expand_structured_sorts)
  {
    mCRL2log(log::verbose) << "Expand structured sorts" << std::endl;
    timer.start("expand structured sorts");
    process::expand_structured_sorts(procspec);
    timer.finish("expand structured sorts");
    log_process_specification(procspec, "expand_structured_sorts");
  }

  mCRL2log(log::verbose) << "Eliminate equations" << std::endl;
  timer.start("eliminate equations");
  eliminate_multiple_usage_equations(procspec, max_equation_usage);
  timer.finish("eliminate equations");
  log_process_specification(procspec, "eliminate_equations");

  mCRL2log(log::verbose) << "Make process guarded" << std::endl;
  timer.start("make process guarded");
  make_guarded(procspec);
  timer.finish("make process guarded");
  log_process_specification(procspec, "make_guarded");

  lps::t_lin_options options;
  return lps::remove_stochastic_operators(lps::linearise(procspec, options));
}

} // namespace process

} // namespace mcrl2

#endif // MCRL2_PROCESS_LINEARIZE_H
