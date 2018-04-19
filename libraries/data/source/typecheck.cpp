// Author(s): Yaroslav Usenko, Jan Friso Groote, Wieger Wesselink (2015)
// Copyright: see the accompanying file COPYING or copy at
// https://svn.win.tue.nl/trac/MCRL2/browser/trunk/COPYING
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// Author(s): Yaroslav Usenko, Jan Friso Groote, Wieger Wesselink (2015)
// Copyright: see the accompanying file COPYING or copy at
// https://svn.win.tue.nl/trac/MCRL2/browser/trunk/COPYING
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <algorithm>
#include <cstring>
#include <cassert>

#include "mcrl2/utilities/logger.h"
#include "mcrl2/data/basic_sort.h"
#include "mcrl2/data/bool.h"
#include "mcrl2/data/pos.h"
#include "mcrl2/data/nat.h"
#include "mcrl2/data/int.h"
#include "mcrl2/data/real.h"
#include "mcrl2/data/list.h"
#include "mcrl2/data/set.h"
#include "mcrl2/data/bag.h"
#include "mcrl2/data/print.h"
#include "mcrl2/data/typecheck.h"
#include "mcrl2/data/untyped_variable_assignment.h"

using namespace mcrl2::log;
using namespace mcrl2::core::detail;

namespace mcrl2 {

namespace data {

namespace detail {

void variable_context::typecheck_variable(const data_type_checker& typechecker, const variable& v) const
{
  typechecker(v, *this);
}

// This function checks whether the set s1 is included in s2. If not the variable culprit
// is the variable occurring in s1 but not in s2.
inline
bool includes(const std::set<variable>& s1, const std::set<variable>& s2, variable& culprit)
{
  for (const variable& v: s1)
  {
    if (s2.count(v) == 0)
    {
      culprit = v;
      return false;
    }
  }
  return true;
}

inline
bool IsPos(const core::identifier_string& x)
{
  char c = x.function().name()[0];
  return isdigit(c) && c > '0';
}

inline
bool IsNat(const core::identifier_string& x)
{
  return isdigit(x.function().name()[0]) != 0;
}

inline
sort_expression_list variable_list_sorts(const variable_list& variables)
{
  sort_expression_list result;
  for (const variable& v: variables)
  {
    result.push_front(v.sort());
  }
  return atermpp::reverse(result);
}

inline
bool has_unknown(const sort_expression& x)
{
  if (data::is_untyped_sort(x))
  {
    return true;
  }
  if (is_basic_sort(x))
  {
    return false;
  }
  if (is_container_sort(x))
  {
    return has_unknown(atermpp::down_cast<container_sort>(x).element_sort());
  }
  if (is_structured_sort(x))
  {
    return false;
  }

  if (is_function_sort(x))
  {
    const auto& s = atermpp::down_cast<function_sort>(x);
    for (const auto& TypeList : s.domain())
    {
      if (has_unknown(TypeList))
      {
        return true;
      }
    }
    return has_unknown(s.codomain());
  }

  return true;
}

inline
bool IsNumericType(const sort_expression& x)
{
  //returns true if x is Bool,Pos,Nat,Int or Real
  //otherwise return fase
  if (data::is_untyped_sort(x))
  {
    return false;
  }
  return sort_bool::is_bool(x) ||
         sort_pos::is_pos(x) ||
         sort_nat::is_nat(x) ||
         sort_int::is_int(x) ||
         sort_real::is_real(x);
}

// Replace occurrences of untyped_possible_sorts([s1,...,sn]) by selecting
// one of the possible sorts from s1,...,sn. Currently, the first is chosen.
inline
sort_expression replace_possible_sorts(const sort_expression& x)
{
  if (is_untyped_possible_sorts(x))
  {
    return atermpp::down_cast<untyped_possible_sorts>(x).sorts().front();
  }
  if (data::is_untyped_sort(x))
  {
    return data::untyped_sort();
  }
  if (is_basic_sort(x))
  {
    return x;
  }
  if (is_container_sort(x))
  {
    const auto& s = atermpp::down_cast<container_sort>(x);
    return container_sort(s.container_name(), replace_possible_sorts(s.element_sort()));
  }

  if (is_structured_sort(x))
  {
    return x;  // I assume that there are no possible sorts in sort constructors. JFG.
  }

  if (is_function_sort(x))
  {
    const auto& x_ = atermpp::down_cast<function_sort>(x);
    sort_expression_list sorts;
    for (const auto& TypeList : x_.domain())
    {
      sorts.push_front(replace_possible_sorts(TypeList));
    }
    const sort_expression& codomain = x_.codomain();
    return function_sort(atermpp::reverse(sorts), replace_possible_sorts(codomain));
  }
  assert(0); // All cases are dealt with above.
  return x; // Avoid compiler warnings.
}

// Insert an element in the list provided, it did not already occur in the list.
template<class S>
inline atermpp::term_list<S> insert_sort_unique(const atermpp::term_list<S>& list, const S& el)
{
  if (std::find(list.begin(), list.end(), el) == list.end())
  {
    atermpp::term_list<S> result = list;
    result.push_front(el);
    return result;
  }
  return list;
}

} // namespace detail

// ------------------------------  Here starts the new class based data expression checker -----------------------

// The function below is used to check whether a term is well typed.
// It always yields true, but if the dataterm is not properly typed, using the types
// that are included inside the term it calls an assert. This function is useful to check
// whether typing was succesful, using assert(strict_type_check(d)).

bool data_type_checker::strict_type_check(const data_expression& x) const
{
  if (is_abstraction(x))
  {
    const auto& x_ = atermpp::down_cast<const abstraction>(x);
    assert(!x_.variables().empty());
    const binder_type& binding_operator = x_.binding_operator();

    if (is_forall_binder(binding_operator) || is_exists_binder(binding_operator))
    {
      assert(x.sort() == sort_bool::bool_());
      strict_type_check(x_.body());
    }

    if (is_lambda_binder(binding_operator))
    {
      strict_type_check(x_.body());
    }
    return true;
  }

  if (is_where_clause(x))
  {
    const auto& where = atermpp::down_cast<const where_clause>(x);
    const assignment_expression_list& where_asss = where.declarations();
    for (const auto& WhereElem: where_asss)
    {
      const auto& t = atermpp::down_cast<const assignment>(WhereElem);
      strict_type_check(t.rhs());
    }
    strict_type_check(where.body());
    return true;
  }

  if (is_application(x))
  {
    const auto& x_ = atermpp::down_cast<application>(x);
    const data_expression& head = x_.head();

    if (data::is_function_symbol(head))
    {
      core::identifier_string name = function_symbol(head).name();
      if (name == sort_list::list_enumeration_name())
      {
        const sort_expression s = x.sort();
        assert(sort_list::is_list(s));
        const sort_expression s1 = container_sort(s).element_sort();

        for (const data_expression& x_i: x_)
        {
          strict_type_check(x_i);
          assert(x_i.sort() == s1);
        }
        return true;
      }
      if (name == sort_set::set_enumeration_name())
      {
        const sort_expression s = x.sort();
        assert(sort_fset::is_fset(s));
        const sort_expression s1 = container_sort(s).element_sort();

        for (const data_expression& x_i: x_)
        {
          strict_type_check(x_i);
          assert(x_i.sort() == s1);
        }
        return true;
      }
      if (name == sort_bag::bag_enumeration_name())
      {
        const sort_expression s = x.sort();
        assert(sort_fbag::is_fbag(s));
        const sort_expression s1 = container_sort(s).element_sort();

        for (auto i = x_.begin();
             i != x_.end();
             ++i)
        {
          strict_type_check(*i);
          assert(i->sort() == s1);
          // Every second element in a bag enumeration should be of type Nat.
          ++i;
          strict_type_check(*i);
          assert(i->sort() == sort_nat::nat());

        }
        return true;

      }
    }
    strict_type_check(head);
    const sort_expression& s = head.sort();
    assert(is_function_sort(s));
    assert(x.sort() == function_sort(s).codomain());
    sort_expression_list argument_sorts = function_sort(s).domain();
    assert(x_.size() == argument_sorts.size());
    auto j = x_.begin();
    for (auto i = argument_sorts.begin();
         i != argument_sorts.end();
         ++i, ++j)
    {
      assert(normalize_sort(j->sort()) == normalize_sort(*i));
      strict_type_check(*j);
    }
    return true;
  }

  if (data::is_function_symbol(x) || is_variable(x))
  {
    return true;
  }

  assert(0); // Unexpected data_expression.
  return true;
}

data_expression data_type_checker::upcast_numeric_type(const data_expression& x, sort_expression sort, sort_expression expected_sort,
                                                       const detail::variable_context& declared_variables, bool strictly_ambiguous,
                                                       bool warn_upcasting, bool print_cast_error) const
{
  data_expression x1 = x;
  if (data::is_untyped_sort(sort))
  {
    return x1;
  }

  if (data::is_untyped_sort(expected_sort))
  {
    return x1;
  }

  // Added to make sure that the types are sufficiently unrolled, because this function is not always called
  // with unrolled types.
  expected_sort = normalize_sort(expected_sort);
  sort = normalize_sort(sort);

  if (equal_sorts(expected_sort, sort))
  {
    return x1;
  }

  if (data::is_untyped_possible_sorts(expected_sort))
  {
    untyped_possible_sorts mps(expected_sort);
    for (const auto& sort1: mps.sorts())
    {
      try
      {
        return upcast_numeric_type(x1, sort, sort1, declared_variables, strictly_ambiguous, warn_upcasting, print_cast_error);
      }
      catch (mcrl2::runtime_error&)
      {
      }
    }
    throw mcrl2::runtime_error("Cannot transform " + data::pp(sort) + " to a number.");
  }

  if (warn_upcasting && data::is_function_symbol(x1) && utilities::is_numeric_string(atermpp::down_cast<function_symbol>(x1).name().function().name()))
  {
    warn_upcasting = false;
  }

  // Try Upcasting to Pos
  sort_expression temp;
  if (match_sorts(expected_sort, sort_pos::pos(), temp))
  {
    if (match_sorts(sort, sort_pos::pos(), temp))
    {
      return x1;
    }
  }

  // Try Upcasting to Nat
  if (match_sorts(expected_sort, sort_nat::nat(), temp))
  {
    if (match_sorts(sort, sort_pos::pos(), temp))
    {
      data_expression OldPar = x1;
      x1 = application(sort_nat::cnat(), x1);
      if (warn_upcasting)
      {
        was_warning_upcasting = true;
        mCRL2log(warning) << "Upcasting " << OldPar << " to sort Nat by applying Pos2Nat to it." << std::endl;
      }
      return x1;
    }
    if (match_sorts(sort, sort_nat::nat(), temp))
    {
      return x1;
    }
  }

  // Try Upcasting to Int
  if (match_sorts(expected_sort, sort_int::int_(), temp))
  {
    if (match_sorts(sort, sort_pos::pos(), temp))
    {
      data_expression OldPar = x1;
      x1 = application(sort_int::cint(), application(sort_nat::cnat(), x1));
      if (warn_upcasting)
      {
        was_warning_upcasting = true;
        mCRL2log(warning) << "Upcasting " << OldPar << " to sort Int by applying Pos2Int to it." << std::endl;
      }
      return x1;
    }
    if (match_sorts(sort, sort_nat::nat(), temp))
    {
      data_expression OldPar = x1;
      x1 = application(sort_int::cint(), x1);
      if (warn_upcasting)
      {
        was_warning_upcasting = true;
        mCRL2log(warning) << "Upcasting " << OldPar << " to sort Int by applying Nat2Int to it." << std::endl;
      }
      return x1;
    }
    if (match_sorts(sort, sort_int::int_(), temp))
    {
      return x1;
    }
  }

  // Try Upcasting to Real
  if (match_sorts(expected_sort, sort_real::real_(), temp))
  {
    if (match_sorts(sort, sort_pos::pos(), temp))
    {
      data_expression OldPar = x1;
      x1 = application(sort_real::creal(), application(sort_int::cint(), application(sort_nat::cnat(), x1)), sort_pos::c1());
      if (warn_upcasting)
      {
        was_warning_upcasting = true;
        mCRL2log(warning) << "Upcasting " << OldPar << " to sort Real by applying Pos2Real to it." << std::endl;
      }
      return x1;
    }
    if (match_sorts(sort, sort_nat::nat(), temp))
    {
      data_expression OldPar = x1;
      x1 = application(sort_real::creal(), application(sort_int::cint(), x1), sort_pos::c1());
      if (warn_upcasting)
      {
        was_warning_upcasting = true;
        mCRL2log(warning) << "Upcasting " << OldPar << " to sort Real by applying Nat2Real to it." << std::endl;
      }
      return x1;
    }
    if (match_sorts(sort, sort_int::int_(), temp))
    {
      data_expression OldPar = x1;
      x1 = application(sort_real::creal(), x1, sort_pos::c1());
      if (warn_upcasting)
      {
        was_warning_upcasting = true;
        mCRL2log(warning) << "Upcasting " << OldPar << " to sort Real by applying Int2Real to it." << std::endl;
      }
      return x1;
    }
    if (match_sorts(sort, sort_real::real_(), temp))
    {
      return x1;
    }
  }

  // If expected_sort and Type are both container types, try to upcast the argument.
  if (is_container_sort(expected_sort) && is_container_sort(sort))
  {
    const container_sort needed_container_type(expected_sort);
    const container_sort container_type(sort);
    sort_expression needed_argument_type = needed_container_type.element_sort();
    const sort_expression& argument_type = container_type.element_sort();
    if (is_untyped_sort(needed_argument_type))
    {
      needed_argument_type = argument_type;
    }
    const sort_expression needed_similar_container_type = container_sort(container_type.container_name(),
                                                                         needed_argument_type);
    if (needed_similar_container_type == expected_sort)
    {
      throw mcrl2::runtime_error("Cannot typecast " + data::pp(sort) + " into " + data::pp(expected_sort) + " for data expression " + data::pp(x1) + ".");
    }
    try
    {
      x1 = typecheck(x1, needed_similar_container_type, declared_variables, strictly_ambiguous, warn_upcasting, print_cast_error);
      sort = x1.sort();
      assert(normalize_sort(sort) == normalize_sort(needed_similar_container_type));

    }
    catch (mcrl2::runtime_error& e)
    {
      throw mcrl2::runtime_error(
              std::string(e.what()) + "\nError occurred while trying to match argument types of " +
              data::pp(expected_sort) + " and " +
              data::pp(sort) + " in data expression " + data::pp(x1) + ".");
    }
  }


  // If is is an fset, try upcasting to a set.
  if (is_container_sort(expected_sort) && is_set_container(container_sort(expected_sort).container_name()))
  {
    if (is_container_sort(sort) && is_fset_container(container_sort(sort).container_name()))
    {
      x1 = sort_set::constructor(container_sort(expected_sort).element_sort(),
                                  sort_set::false_function(container_sort(expected_sort).element_sort()), x1);
      // Do this again to lift argument types if needed. TODO.
      return x1;
    }
    else if (is_container_sort(sort) && is_set_container(container_sort(sort).container_name()))
    {
      if (sort == expected_sort)
      {
        return x1;
      }
      else
      {
        throw mcrl2::runtime_error("Upcasting " + data::pp(sort) + " to " + data::pp(expected_sort) + " fails (1).");
      }
    }
  }

  // If is is an fbag, try upcasting to a bag.
  if (is_container_sort(expected_sort) && is_bag_container(container_sort(expected_sort).container_name()))
  {
    if (is_container_sort(sort) && is_fbag_container(container_sort(sort).container_name()))
    {
      x1 = sort_bag::constructor(container_sort(expected_sort).element_sort(), sort_bag::zero_function(container_sort(expected_sort).element_sort()), x1);
      // Do this again to lift argument types if needed. TODO.
      return x1;
    }
    else if (is_container_sort(sort) && is_bag_container(container_sort(sort).container_name()))
    {
      if (sort == expected_sort)
      {
        return x1;
      }
      else
      {
        throw mcrl2::runtime_error("Upcasting " + data::pp(sort) + " to " + data::pp(expected_sort) + " fails (1).");
      }
    }
  }

  if (is_function_sort(expected_sort))
  {
    const function_sort needed_function_type(expected_sort);
    if (is_function_sort(sort))
    {
      // we only deal here with @false_ and @zero (false_function and zero_function).
      if (x1 == sort_set::false_function(data::untyped_sort()))
      {
        assert(needed_function_type.domain().size() == 1);
        x1 = sort_set::false_function(needed_function_type.domain().front());
        return x1;
      }
      else if (x1 == sort_bag::zero_function(data::untyped_sort()))
      {
        assert(needed_function_type.domain().size() == 1);
        x1 = sort_bag::zero_function(needed_function_type.domain().front());
        return x1;
      }
    }
  }

  throw mcrl2::runtime_error("Upcasting " + data::pp(sort) + " to " + data::pp(expected_sort) + " fails (3).");
}

bool data::data_type_checker::UnFSet(const sort_expression& x, sort_expression& result) const
{
  if (sort_fset::is_fset(x) || sort_set::is_set(x))
  {
    result = atermpp::down_cast<container_sort>(x).element_sort();
    return true;
  }
  else if (data::is_untyped_sort(x))
  {
    result = x;
    return true;
  }
  else if (is_untyped_possible_sorts(x))
  {
    sort_expression_list sorts;
    const auto& mps = atermpp::down_cast<untyped_possible_sorts>(x);
    for (const sort_expression& sort: mps.sorts())
    {
      if (sort_fset::is_fset(sort) || (sort_set::is_set(sort)))
      {
        sorts.push_front(atermpp::down_cast<const container_sort>(sort).element_sort());
      }
      else if (data::is_untyped_sort(sort))
      {
        sorts.push_front(sort);
      }
    }
    sorts = atermpp::reverse(sorts);
    result = untyped_possible_sorts(sort_expression_list(sorts));
    return true;
  }
  return false;
}

bool data::data_type_checker::UnFBag(const sort_expression& x, sort_expression& result) const
{
  if (sort_fbag::is_fbag(x) || (sort_bag::is_bag(x)))
  {
    result = atermpp::down_cast<const container_sort>(x).element_sort();
    return true;
  }
  else if (data::is_untyped_sort(x))
  {
    result = x;
    return true;
  }
  else if (is_untyped_possible_sorts(x))
  {
    sort_expression_list sorts;
    const auto& x_ = atermpp::down_cast<untyped_possible_sorts>(x);
    for (const sort_expression& sort: x_.sorts())
    {
      // if (sort_fbag::is_fbag(sort) || (sort_fbag::is_fbag(sort))) // Is this some kind of typo???
      if (sort_fbag::is_fbag(sort))
      {
        sorts.push_front(atermpp::down_cast<container_sort>(sort).element_sort());
      }
      else if (data::is_untyped_sort(sort))
      {
        sorts.push_front(sort);
      }
    }
    sorts = atermpp::reverse(sorts);
    result = untyped_possible_sorts(sorts);
    return true;
  }
  return false;
}

bool data_type_checker::UnList(const sort_expression& x, sort_expression& result) const
{
  if (sort_list::is_list(x))
  {
    result = atermpp::down_cast<const container_sort>(x).element_sort();
    return true;
  }
  else if (data::is_untyped_sort(x))
  {
    result = x;
    return true;
  }
  else if (is_untyped_possible_sorts(x))
  {
    sort_expression_list sorts;
    const auto& x_ = atermpp::down_cast<untyped_possible_sorts>(x);
    for (const sort_expression& sort: x_.sorts())
    {
      if (is_basic_sort(sort))
      {
        sorts.push_front(normalize_sort(sort));
      }
      else if (sort_list::is_list(sort))
      {
        sorts.push_front(atermpp::down_cast<const container_sort>(sort).element_sort());
      }
      else if (data::is_untyped_sort(sort))
      {
        sorts.push_front(sort);
      }
    }
    sorts = atermpp::reverse(sorts);
    result = untyped_possible_sorts(sorts);
    return true;
  }
  return false;
}


bool data_type_checker::UnArrowProd(const sort_expression_list& ArgTypes, sort_expression PosType, sort_expression& result) const
{
  if (is_basic_sort(PosType))
  {
    PosType = normalize_sort(PosType);
  }
  if (is_function_sort(PosType))
  {
    const auto& s = atermpp::down_cast<const function_sort>(PosType);
    const sort_expression_list& PosArgTypes = s.domain();

    if (PosArgTypes.size() != ArgTypes.size())
    {
      return false;
    }
    sort_expression_list temp;
    if (match_sort_lists(PosArgTypes, ArgTypes, temp))
    {
      result = s.codomain();
      return true;
    }
    else
    {
      // Lift the argument of PosType.
      match_sort_lists(ArgTypes, expand_numeric_types_up(PosArgTypes), temp);
      result = s.codomain();
      return true;
    }
  }
  if (data::is_untyped_sort(PosType))
  {
    result = PosType;
    return true;
  }

  sort_expression_list NewPosTypes;
  if (is_untyped_possible_sorts(PosType))
  {
    const auto& mps = atermpp::down_cast<untyped_possible_sorts>(PosType);
    for (sort_expression_list PosTypes = mps.sorts();
         !PosTypes.empty();
         PosTypes = PosTypes.tail())
    {
      sort_expression NewPosType = PosTypes.front();
      if (is_basic_sort(NewPosType))
      {
        NewPosType = normalize_sort(NewPosType);
      }
      if (is_function_sort(NewPosType))
      {
        const auto& s = atermpp::down_cast<const function_sort>(NewPosType);
        const sort_expression_list& PosArgTypes = s.domain();
        if (PosArgTypes.size() != ArgTypes.size())
        {
          continue;
        }
        sort_expression_list temp_list;
        if (match_sort_lists(PosArgTypes, ArgTypes, temp_list))
        {
          NewPosType = s.codomain();
        }
      }
      else if (!data::is_untyped_sort(NewPosType))
      {
        continue;
      }
      NewPosTypes = detail::insert_sort_unique(NewPosTypes, NewPosType);
    }
    NewPosTypes = atermpp::reverse(NewPosTypes);
    result = untyped_possible_sorts(sort_expression_list(NewPosTypes));
    return true;
  }
  return false;
}

//Find the minimal type that Unifies the 2. If not possible, return false.
bool data_type_checker::unify_minimum_type(const sort_expression& x1, const sort_expression& x2,
                                           sort_expression& result) const
{
  if (!match_sorts(x1, x2, result))
  {
    if (!match_sorts(x1, expand_numeric_types_up(x2), result))
    {
      if (!match_sorts(x2, expand_numeric_types_up(x1), result))
      {
        return false;
      }
    }
  }

  if (is_untyped_possible_sorts(result))
  {
    result = atermpp::down_cast<untyped_possible_sorts>(result).sorts().front();
  }
  return true;
}

bool data_type_checker::match_if(const function_sort& type, sort_expression& result) const
{
  sort_expression_list domain = type.domain();
  sort_expression codomain = type.codomain();
  if (domain.size() != 3)
  {
    return false;
  }
  domain = domain.tail();

  if (!unify_minimum_type(codomain, domain.front(), codomain))
  {
    return false;
  }
  domain = domain.tail();
  if (!unify_minimum_type(codomain, domain.front(), codomain))
  {
    return false;
  }

  result = function_sort({sort_bool::bool_(), codomain, codomain}, codomain);
  return true;
}

bool data_type_checker::match_relational_operators(const function_sort& type, sort_expression& result) const
{
  sort_expression_list domain = type.domain();
  if (domain.size() != 2)
  {
    return false;
  }
  sort_expression arg1 = domain.front();
  domain = domain.tail();
  sort_expression arg2 = domain.front();

  sort_expression arg;
  if (!unify_minimum_type(arg1, arg2, arg))
  {
    return false;
  }

  result = function_sort({ arg, arg }, sort_bool::bool_());
  return true;
}

bool data_type_checker::match_sqrt(const function_sort& type, sort_expression& result) const
{
  const sort_expression_list& domain = type.domain();
  if (domain.size() != 1)
  {
    return false;
  }
  if (domain.front() == sort_nat::nat())
  {
    result = function_sort(domain, sort_nat::nat());
    return true;
  }
  return false;
}

bool data_type_checker::match_cons(const function_sort& type, sort_expression& result) const
{
  sort_expression codomain = type.codomain();
  if (is_basic_sort(codomain))
  {
    codomain = normalize_sort(codomain);
  }
  if (!sort_list::is_list(normalize_sort(codomain)))
  {
    return false;
  }
  codomain = atermpp::down_cast<container_sort>(codomain).element_sort();
  sort_expression_list domain = type.domain();
  if (domain.size() != 2)
  {
    return false;
  }
  sort_expression arg1 = domain.front();
  domain = domain.tail();
  sort_expression arg2 = domain.front();
  if (is_basic_sort(arg2))
  {
    arg2 = normalize_sort(arg2);
  }
  if (!sort_list::is_list(arg2))
  {
    return false;
  }
  arg2 = atermpp::down_cast<container_sort>(arg2).element_sort();

  sort_expression new_result;
  if (!unify_minimum_type(codomain, arg1, new_result))
  {
    return false;
  }

  if (!unify_minimum_type(new_result, arg2, codomain))
  {
    return false;
  }

  result = function_sort({ codomain, sort_list::list(codomain) }, sort_list::list(codomain));
  return true;
}

bool data_type_checker::match_snoc(const function_sort& type, sort_expression& result) const
{
  sort_expression codomain = type.codomain();
  if (is_basic_sort(codomain))
  {
    codomain = normalize_sort(codomain);
  }
  if (!sort_list::is_list(codomain))
  {
    return false;
  }
  codomain = atermpp::down_cast<container_sort>(codomain).element_sort();
  sort_expression_list domain = type.domain();
  if (domain.size() != 2)
  {
    return false;
  }
  sort_expression arg1 = domain.front();
  if (is_basic_sort(arg1))
  {
    arg1 = normalize_sort(arg1);
  }
  if (!sort_list::is_list(arg1))
  {
    return false;
  }
  arg1 = atermpp::down_cast<container_sort>(arg1).element_sort();

  domain = domain.tail();
  sort_expression arg2 = domain.front();

  sort_expression new_result;
  if (!unify_minimum_type(codomain, arg1, new_result))
  {
    return false;
  }

  if (!unify_minimum_type(new_result, arg2, codomain))
  {
    return false;
  }

  result = function_sort({sort_list::list(codomain), codomain}, sort_list::list(codomain));
  return true;
}

bool data_type_checker::match_concat(const function_sort& type, sort_expression& result) const
{
  sort_expression codomain = type.codomain();
  if (is_basic_sort(codomain))
  {
    codomain = normalize_sort(codomain);
  }
  if (!sort_list::is_list(codomain))
  {
    return false;
  }
  codomain = atermpp::down_cast<container_sort>(codomain).element_sort();
  sort_expression_list domain = type.domain();
  if (domain.size() != 2)
  {
    return false;
  }

  sort_expression arg1 = domain.front();
  if (is_basic_sort(arg1))
  {
    arg1 = normalize_sort(arg1);
  }
  if (!sort_list::is_list(arg1))
  {
    return false;
  }
  arg1 = atermpp::down_cast<container_sort>(arg1).element_sort();

  domain = domain.tail();

  sort_expression arg2 = domain.front();
  if (is_basic_sort(arg2))
  {
    arg2 = normalize_sort(arg2);
  }
  if (!sort_list::is_list(arg2))
  {
    return false;
  }
  arg2 = atermpp::down_cast<container_sort>(arg2).element_sort();

  sort_expression new_result;
  if (!unify_minimum_type(codomain, arg1, new_result))
  {
    return false;
  }

  if (!unify_minimum_type(new_result, arg2, codomain))
  {
    return false;
  }

  result = function_sort({sort_list::list(codomain), sort_list::list(codomain)}, sort_list::list(codomain));
  return true;
}

bool data_type_checker::match_element_at(const function_sort& type, sort_expression& result) const
{
  sort_expression codomain = type.codomain();
  const sort_expression_list& domain = type.domain();
  if (domain.size() != 2)
  {
    return false;
  }

  sort_expression arg1 = domain.front();
  if (is_basic_sort(arg1))
  {
    arg1 = normalize_sort(arg1);
  }
  if (!sort_list::is_list(arg1))
  {
    return false;
  }
  arg1 = atermpp::down_cast<container_sort>(arg1).element_sort();

  sort_expression new_result;
  if (!unify_minimum_type(codomain, arg1, new_result))
  {
    return false;
  }
  codomain = new_result;

  result = function_sort({sort_list::list(codomain), sort_nat::nat()}, codomain);
  return true;
}

bool data_type_checker::match_head(const function_sort& type, sort_expression& result) const
{
  sort_expression codomain = type.codomain();
  const sort_expression_list& domain = type.domain();
  if (domain.size() != 1)
  {
    return false;
  }
  sort_expression arg = domain.front();
  if (is_basic_sort(arg))
  {
    arg = normalize_sort(arg);
  }
  if (!sort_list::is_list(arg))
  {
    return false;
  }
  arg = atermpp::down_cast<container_sort>(arg).element_sort();

  sort_expression new_result;
  if (!unify_minimum_type(codomain, arg, new_result))
  {
    return false;
  }
  codomain = new_result;

  result = function_sort({ sort_list::list(codomain) }, codomain);
  return true;
}

bool data_type_checker::match_tail(const function_sort& type, sort_expression& result) const
{
  sort_expression codomain = type.codomain();
  if (is_basic_sort(codomain))
  {
    codomain = normalize_sort(codomain);
  }
  if (!sort_list::is_list(codomain))
  {
    return false;
  }
  codomain = atermpp::down_cast<container_sort>(codomain).element_sort();
  const sort_expression_list& domain = type.domain();
  if (domain.size() != 1)
  {
    return false;
  }
  sort_expression arg = domain.front();
  if (is_basic_sort(arg))
  {
    arg = normalize_sort(arg);
  }
  if (!sort_list::is_list(arg))
  {
    return false;
  }
  arg = atermpp::down_cast<container_sort>(arg).element_sort();

  sort_expression new_result;
  if (!unify_minimum_type(codomain, arg, new_result))
  {
    return false;
  }
  codomain = new_result;

  result = function_sort(sort_expression_list({ sort_list::list(codomain) }), sort_list::list(codomain));
  return true;
}

//Sets
bool data_type_checker::match_set2bag(const function_sort& type, sort_expression& result) const
{
  sort_expression codomain = type.codomain();
  if (is_basic_sort(codomain))
  {
    codomain = normalize_sort(codomain);
  }
  if (!sort_bag::is_bag(codomain))
  {
    return false;
  }
  codomain = atermpp::down_cast<container_sort>(codomain).element_sort();

  const sort_expression_list& domain = type.domain();
  if (domain.size() != 1)
  {
    return false;
  }

  sort_expression front = domain.front();
  if (is_basic_sort(front))
  {
    front = normalize_sort(front);
  }
  if (!sort_set::is_set(front))
  {
    return false;
  }
  front = atermpp::down_cast<container_sort>(front).element_sort();

  sort_expression new_front;
  if (!unify_minimum_type(front, codomain, new_front))
  {
    return false;
  }
  front = new_front;

  result = function_sort(sort_expression_list({sort_set::set_(front)}), sort_bag::bag(front));
  return true;
}

bool data_type_checker::match_set_constructor(const function_sort& type, sort_expression& result) const
{
  sort_expression codomain = type.codomain();
  if (is_basic_sort(codomain))
  {
    codomain = normalize_sort(codomain);
  }
  if (!sort_set::is_set(codomain))
  {
    return false;
  }
  codomain = atermpp::down_cast<container_sort>(codomain).element_sort();

  sort_expression_list domain = type.domain();
  if (domain.size() != 2)
  {
    return false;
  }

  sort_expression arg1 = domain.front();
  if (is_basic_sort(arg1))
  {
    arg1 = normalize_sort(arg1);
  }
  if (!is_function_sort(arg1))
  {
    return false;
  }

  const sort_expression Arg12 = atermpp::down_cast<function_sort>(arg1).codomain();

  sort_expression new_result;
  if (!unify_minimum_type(Arg12, sort_bool::bool_(), new_result))
  {
    return false;
  }

  const sort_expression_list Arg11l = atermpp::down_cast<function_sort>(arg1).domain();
  if (Arg11l.size() != 1)
  {
    return false;
  }
  const sort_expression& Arg11 = Arg11l.front();

  if (!unify_minimum_type(Arg11, codomain, new_result))
  {
    return false;
  }


  domain.pop_front();
  sort_expression Arg2 = domain.front();
  if (is_basic_sort(Arg2))
  {
    Arg2 = normalize_sort(Arg2);
  }
  if (!sort_fset::is_fset(Arg2))
  {
    return false;
  }
  sort_expression Arg21 = atermpp::down_cast<container_sort>(Arg2).element_sort();

  sort_expression new_result2;
  if (!unify_minimum_type(Arg21, new_result, new_result2))
  {
    return false;
  }

  arg1 = function_sort({new_result2}, sort_bool::bool_());
  Arg2 = sort_fset::fset(new_result2);
  result = function_sort({arg1, Arg2}, sort_set::set_(new_result2));
  return true;
}

bool data_type_checker::match_false(const function_sort& type, sort_expression& result) const
{
  result = type;
  return true;
}

bool data_type_checker::match_bag_constructor(const function_sort& type, sort_expression& result) const
{
  sort_expression codomain = type.codomain();
  if (is_basic_sort(codomain))
  {
    codomain = normalize_sort(codomain);
  }
  if (!sort_bag::is_bag(codomain))
  {
    return false;
  }
  codomain = atermpp::down_cast<container_sort>(codomain).element_sort();

  sort_expression_list domain = type.domain();
  if (domain.size() != 2)
  {
    return false;
  }

  sort_expression arg1 = domain.front();
  if (is_basic_sort(arg1))
  {
    arg1 = normalize_sort(arg1);
  }
  if (!is_function_sort(arg1))
  {
    return false;
  }

  const sort_expression arg12 = atermpp::down_cast<function_sort>(arg1).codomain();

  sort_expression new_result;
  if (!unify_minimum_type(arg12, sort_nat::nat(), new_result))
  {
    return false;
  }

  const sort_expression_list Arg11l = atermpp::down_cast<function_sort>(arg1).domain();
  if (Arg11l.size() != 1)
  {
    return false;
  }
  const sort_expression& Arg11 = Arg11l.front();

  if (!unify_minimum_type(Arg11, codomain, new_result))
  {
    return false;
  }


  domain.pop_front();
  sort_expression Arg2 = domain.front();
  if (is_basic_sort(Arg2))
  {
    Arg2 = normalize_sort(Arg2);
  }
  if (!sort_fbag::is_fbag(Arg2))
  {
    return false;
  }
  sort_expression Arg21 = atermpp::down_cast<container_sort>(Arg2).element_sort();

  sort_expression new_result2;
  if (!unify_minimum_type(Arg21, new_result, new_result2))
  {
    return false;
  }

  arg1 = function_sort(sort_expression_list({new_result2}), sort_nat::nat());
  Arg2 = sort_fbag::fbag(new_result2);
  result = function_sort(sort_expression_list({arg1, Arg2}), sort_bag::bag(new_result2));
  return true;
}

bool data_type_checker::match_in(const function_sort& type, sort_expression& result) const
{
  sort_expression_list domain = type.domain();
  if (domain.size() != 2)
  {
    return false;
  }

  sort_expression arg1 = domain.front();
  domain = domain.tail();
  sort_expression arg2 = domain.front();

  if (is_basic_sort(arg2))
  {
    arg2 = normalize_sort(arg2);
  }
  if (!is_container_sort(arg2))
  {
    return false;
  }
  sort_expression second_sort = atermpp::down_cast<container_sort>(arg2).element_sort();
  sort_expression sort;
  if (!unify_minimum_type(arg1, second_sort, sort))
  {
    return false;
  }

  result = function_sort(
          {sort, container_sort(atermpp::down_cast<const container_sort>(arg2).container_name(), sort)},
          sort_bool::bool_());
  return true;
}

bool data_type_checker::match_fset_insert(const function_sort& type, sort_expression& result) const
{
  sort_expression_list domain = type.domain();
  if (domain.size() != 2)
  {
    return false;
  }

  sort_expression arg1 = domain.front();
  domain = domain.tail();
  sort_expression arg2 = domain.front();

  if (is_basic_sort(arg2))
  {
    arg2 = normalize_sort(arg2);
  }
  if (!is_container_sort(arg2))
  {
    return false;
  }
  sort_expression second_sort = atermpp::down_cast<container_sort>(arg2).element_sort();
  sort_expression sort;
  if (!unify_minimum_type(arg1, second_sort, sort))
  {
    return false;
  }

  const sort_expression fset_type = container_sort(atermpp::down_cast<const container_sort>(arg2).container_name(), sort);
  result = function_sort({sort, fset_type}, fset_type);
  return true;
}

bool data_type_checker::match_fbag_cinsert(const function_sort& type, sort_expression& result) const
{
  sort_expression_list domain = type.domain();
  if (domain.size() != 3)
  {
    return false;
  }

  sort_expression arg1 = domain.front();
  domain = domain.tail();
  sort_expression arg2 = domain.front();

  if (is_basic_sort(arg2))
  {
    arg2 = normalize_sort(arg2);
  }
  domain = domain.tail();
  sort_expression third = domain.front();
  if (is_basic_sort(third))
  {
    third = normalize_sort(third);
  }

  sort_expression second_sort;
  if (!unify_minimum_type(arg2, sort_nat::nat(), second_sort))
  {
    return false;
  }

  if (!is_container_sort(third))
  {
    return false;
  }

  sort_expression third_sort = atermpp::down_cast<container_sort>(third).element_sort();
  sort_expression sort;
  if (!unify_minimum_type(arg1, third_sort, sort))
  {
    return false;
  }

  const sort_expression fbag_type = container_sort(atermpp::down_cast<const container_sort>(third).container_name(), sort);
  result = function_sort({sort, second_sort, fbag_type}, fbag_type);
  return true;
}

bool data_type_checker::match_set_bag_operations(const function_sort& x, sort_expression& result) const
{
  sort_expression codomain = x.codomain();
  if (is_basic_sort(codomain))
  {
    codomain = normalize_sort(codomain);
  }
  if (data::detail::IsNumericType(codomain))
  {
    result = x;
    return true;
  }
  if (!(sort_set::is_set(codomain) || sort_bag::is_bag(codomain) || sort_fset::is_fset(codomain) || sort_fbag::is_fbag(codomain)))
  {
    return false;
  }
  sort_expression_list domain = x.domain();
  if (domain.size() != 2)
  {
    return false;
  }

  sort_expression arg1 = domain.front();
  if (is_basic_sort(arg1))
  {
    arg1 = normalize_sort(arg1);
  }
  if (data::detail::IsNumericType(arg1))
  {
    result = x;
    return true;
  }
  if (!(sort_set::is_set(arg1) || sort_bag::is_bag(arg1) || sort_fset::is_fset(arg1) || sort_fbag::is_fbag(arg1)))
  {
    return false;
  }

  domain = domain.tail();
  sort_expression arg2 = domain.front();

  if (is_basic_sort(arg2))
  {
    arg2 = normalize_sort(arg2);
  }
  if (detail::IsNumericType(arg2))
  {
    result = x;
    return true;
  }
  if (!(sort_set::is_set(arg2) || sort_bag::is_bag(arg2) ||
        sort_fset::is_fset(arg2) || sort_fbag::is_fbag(arg2)))
  {
    return false;
  }

  // If one of the argumenst is an fset/fbag and the other a set/bag, lift it to match the bag/set.
  if (sort_set::is_set(arg1) && sort_fset::is_fset(arg2))
  {
    arg2 = sort_set::set_(container_sort(arg2).element_sort());
  }

  if (sort_fset::is_fset(arg1) && sort_set::is_set(arg2))
  {
    arg1 = sort_set::set_(container_sort(arg1).element_sort());
  }

  if (sort_bag::is_bag(arg1) && sort_fbag::is_fbag(arg2))
  {
    arg2 = sort_bag::bag(container_sort(arg2).element_sort());
  }

  if (sort_fbag::is_fbag(arg1) && sort_bag::is_bag(arg2))
  {
    arg1 = sort_bag::bag(container_sort(arg1).element_sort());
  }

  sort_expression temp_result;
  if (!unify_minimum_type(codomain, arg1, temp_result))
  {
    return false;
  }

  if (!unify_minimum_type(temp_result, arg2, codomain))
  {
    return false;
  }

  result = function_sort({codomain, codomain}, codomain);
  return true;
}

bool data_type_checker::match_set_complement(const function_sort& type, sort_expression& result) const
{
  sort_expression codomain = type.codomain();
  if (is_basic_sort(codomain))
  {
    codomain = normalize_sort(codomain);
  }
  // if (detail::IsNumericType(Res))
  if (codomain == sort_bool::bool_())
  {
    result = type;
    return true;
  }

  const sort_expression_list& domain = type.domain();
  if (domain.size() != 1)
  {
    return false;
  }

  sort_expression arg1 = domain.front();
  if (is_basic_sort(arg1))
  {
    arg1 = normalize_sort(arg1);
  }
  // if (detail::IsNumericType(Arg))
  if (arg1 == sort_bool::bool_())
  {
    result = type;
    return true;
  }
  if (!sort_set::is_set(codomain))
  {
    return false;
  }
  codomain = atermpp::down_cast<container_sort>(codomain).element_sort();
  if (!sort_set::is_set(arg1))
  {
    return false;
  }
  arg1 = atermpp::down_cast<container_sort>(arg1).element_sort();

  sort_expression temp_result;
  if (!unify_minimum_type(codomain, arg1, temp_result))
  {
    return false;
  }
  codomain = temp_result;

  result = function_sort({sort_set::set_(codomain)}, sort_set::set_(codomain));
  return true;
}

//Bags
bool data_type_checker::match_bag2set(const function_sort& type, sort_expression& result) const
{
  sort_expression codomain = type.codomain();
  if (is_basic_sort(codomain))
  {
    codomain = normalize_sort(codomain);
  }
  if (!sort_set::is_set(codomain))
  {
    return false;
  }
  codomain = atermpp::down_cast<container_sort>(codomain).element_sort();

  const sort_expression_list& domain = type.domain();
  if (domain.size() != 1)
  {
    return false;
  }

  sort_expression arg1 = domain.front();
  if (is_basic_sort(arg1))
  {
    arg1 = normalize_sort(arg1);
  }
  if (!sort_bag::is_bag(arg1))
  {
    return false;
  }
  arg1 = atermpp::down_cast<container_sort>(arg1).element_sort();

  sort_expression temp_result;
  if (!unify_minimum_type(arg1, codomain, temp_result))
  {
    return false;
  }
  arg1 = temp_result;

  result = function_sort({ sort_bag::bag(arg1) }, sort_set::set_(arg1));
  return true;
}

bool data_type_checker::match_bag_count(const function_sort& type, sort_expression& result) const
{
  if (!is_function_sort(type))
  {
    result = type;
    return true;
  }
  sort_expression_list domain = type.domain();
  if (domain.size() != 2)
  {
    result = type;
    return true;
  }

  sort_expression arg1 = domain.front();
  domain = domain.tail();
  sort_expression arg2 = domain.front();

  if (is_basic_sort(arg2))
  {
    arg2 = normalize_sort(arg2);
  }
  if (!sort_bag::is_bag(arg2))
  {
    result = type;
    return true;
  }
  arg2 = atermpp::down_cast<container_sort>(arg2).element_sort();

  sort_expression sort;
  if (!unify_minimum_type(arg1, arg2, sort))
  {
    return false;
  }

  result = function_sort(sort_expression_list({sort, sort_bag::bag(sort)}), sort_nat::nat());
  return true;
}


bool data_type_checker::match_function_update(const function_sort& type, sort_expression& result) const
{
  sort_expression_list domain = type.domain();
  if (domain.size() != 3)
  {
    return false;
  }
  function_sort arg1 = atermpp::down_cast<function_sort>(domain.front());
  domain = domain.tail();
  sort_expression arg2 = domain.front();
  domain = domain.tail();
  sort_expression third = domain.front();
  const sort_expression& codomain = type.codomain();
  if (!is_function_sort(codomain))
  {
    return false;
  }

  sort_expression temp_result;
  if (!unify_minimum_type(arg1, codomain, temp_result))
  {
    return false;
  }
  arg1 = atermpp::down_cast<function_sort>(normalize_sort(temp_result));

  // determine A and B from Arg1:
  sort_expression_list LA = arg1.domain();
  if (LA.size() != 1)
  {
    return false;
  }
  const sort_expression& A = LA.front();
  sort_expression B = arg1.codomain();

  if (!unify_minimum_type(A, arg2, temp_result))
  {
    return false;
  }
  if (!unify_minimum_type(B, third, temp_result))
  {
    return false;
  }

  result = function_sort(sort_expression_list({arg1, A, B}), arg1);
  return true;
}


bool
data_type_checker::maximum_type(const sort_expression& x1, const sort_expression& x2, sort_expression& result) const
{
  if (equal_sorts(x1, x2))
  {
    result = x1;
    return true;
  }
  if (data::is_untyped_sort(x1))
  {
    result = x2;
    return true;
  }
  if (data::is_untyped_sort(x2))
  {
    result = x1;
    return true;
  }
  if (equal_sorts(x1, sort_real::real_()))
  {
    if (equal_sorts(x2, sort_int::int_()))
    {
      result = x1;
      return true;
    }
    if (equal_sorts(x2, sort_nat::nat()))
    {
      result = x1;
      return true;
    }
    if (equal_sorts(x2, sort_pos::pos()))
    {
      result = x1;
      return true;
    }
    return false;
  }
  if (equal_sorts(x1, sort_int::int_()))
  {
    if (equal_sorts(x2, sort_real::real_()))
    {
      result = x2;
      return true;
    }
    if (equal_sorts(x2, sort_nat::nat()))
    {
      result = x1;
      return true;
    }
    if (equal_sorts(x2, sort_pos::pos()))
    {
      result = x1;
      return true;
    }
    return false;
  }
  if (equal_sorts(x1, sort_nat::nat()))
  {
    if (equal_sorts(x2, sort_real::real_()))
    {
      result = x2;
      return true;
    }
    if (equal_sorts(x2, sort_int::int_()))
    {
      result = x2;
      return true;
    }
    if (equal_sorts(x2, sort_pos::pos()))
    {
      result = x1;
      return true;
    }
    return false;
  }
  if (equal_sorts(x1, sort_pos::pos()))
  {
    if (equal_sorts(x2, sort_real::real_()))
    {
      result = x2;
      return true;
    }
    if (equal_sorts(x2, sort_int::int_()))
    {
      result = x2;
      return true;
    }
    if (equal_sorts(x2, sort_nat::nat()))
    {
      result = x2;
      return true;
    }
    return false;
  }
  return false;
}

sort_expression_list data_type_checker::expand_numeric_types_up(const sort_expression_list& x) const
{
  sort_expression_vector result;
  for (const auto& i : x)
  {
    result.push_back(expand_numeric_types_up(i));
  }
  return sort_expression_list(result.begin(), result.end());
}

sort_expression data_type_checker::expand_numeric_types_up(const sort_expression& x) const
{
  if (data::is_untyped_sort(x))
  {
    return x;
  }
  if (equal_sorts(sort_pos::pos(), x))
  {
    return untyped_possible_sorts(sort_expression_list({sort_pos::pos(), sort_nat::nat(), sort_int::int_(), sort_real::real_()}));
  }
  if (equal_sorts(sort_nat::nat(), x))
  {
    return untyped_possible_sorts(sort_expression_list({sort_nat::nat(), sort_int::int_(), sort_real::real_()}));
  }
  if (equal_sorts(sort_int::int_(), x))
  {
    return untyped_possible_sorts(sort_expression_list({sort_int::int_(), sort_real::real_()}));
  }
  if (is_basic_sort(x))
  {
    return x;
  }
  if (is_container_sort(x))
  {
    const auto& s = atermpp::down_cast<container_sort>(x);
    const container_type& type = s.container_name();
    if (is_list_container(type))
    {
      return container_sort(s.container_name(), expand_numeric_types_up(s.element_sort()));
    }

    if (is_set_container(type))
    {
      return container_sort(s.container_name(), expand_numeric_types_up(s.element_sort()));
    }

    if (is_bag_container(type))
    {
      return container_sort(s.container_name(), expand_numeric_types_up(s.element_sort()));
    }

    if (is_fset_container(type))
    {
      sort_expression sort = expand_numeric_types_up(s.element_sort());
      return untyped_possible_sorts(sort_expression_list({container_sort(s.container_name(), sort),
                                                          container_sort(set_container(), sort)}));
    }

    if (is_fbag_container(type))
    {
      sort_expression sort = expand_numeric_types_up(s.element_sort());
      return untyped_possible_sorts(sort_expression_list({container_sort(s.container_name(), sort),
                                                          container_sort(bag_container(), sort)}));
    }
  }

  if (is_structured_sort(x))
  {
    return x;
  }

  if (is_function_sort(x))
  {
    const auto& x_ = atermpp::down_cast<const function_sort>(x);
    //the argument types, and if the resulting type is SortArrow -- recursively
    sort_expression_list sorts;
    for (const sort_expression& sort: x_.domain())
    {
      sorts.push_front(expand_numeric_types_up(normalize_sort(sort)));
    }
    const sort_expression& codomain = x_.codomain();
    if (!is_function_sort(codomain))
    {
      return function_sort(atermpp::reverse(sorts), codomain);
    }
    else
    {
      return function_sort(atermpp::reverse(sorts), expand_numeric_types_up(normalize_sort(codomain)));
    }
  }
  return x;
}

sort_expression data_type_checker::expand_numeric_types_down(sort_expression Type) const
{
  if (data::is_untyped_sort(Type))
  {
    return Type;
  }
  if (is_basic_sort(Type))
  {
    Type = normalize_sort(Type);
  }

  bool function = false;
  sort_expression_list Args;
  if (is_function_sort(Type))
  {
    const auto& fs = atermpp::down_cast<const function_sort>(Type);
    function = true;
    Args = fs.domain();
    Type = fs.codomain();
  }

  if (equal_sorts(sort_real::real_(), Type))
  {
    Type = untyped_possible_sorts(
            sort_expression_list({sort_pos::pos(), sort_nat::nat(), sort_int::int_(), sort_real::real_()}));
  }
  if (equal_sorts(sort_int::int_(), Type))
  {
    Type = untyped_possible_sorts(sort_expression_list({sort_pos::pos(), sort_nat::nat(), sort_int::int_()}));
  }
  if (equal_sorts(sort_nat::nat(), Type))
  {
    Type = untyped_possible_sorts(sort_expression_list({sort_pos::pos(), sort_nat::nat()}));
  }
  if (is_container_sort(Type))
  {
    const auto& s = atermpp::down_cast<container_sort>(Type);
    const container_type& type = s.container_name();
    if (is_list_container(type))
    {
      Type = container_sort(s.container_name(), expand_numeric_types_down(s.element_sort()));
    }

    if (is_fset_container(type))
    {
      Type = container_sort(s.container_name(), expand_numeric_types_down(s.element_sort()));
    }

    if (is_fbag_container(type))
    {
      Type = container_sort(s.container_name(), expand_numeric_types_down(s.element_sort()));
    }

    if (is_set_container(type))
    {
      const sort_expression shrinked_sorts = expand_numeric_types_down(s.element_sort());
      Type = untyped_possible_sorts(sort_expression_list({
                                                                 container_sort(s.container_name(), shrinked_sorts),
                                                                 container_sort(set_container(), shrinked_sorts)}));
    }

    if (is_bag_container(type))
    {
      const sort_expression shrinked_sorts = expand_numeric_types_down(s.element_sort());
      Type = untyped_possible_sorts(sort_expression_list({
                                                                 container_sort(s.container_name(), shrinked_sorts),
                                                                 container_sort(bag_container(), shrinked_sorts)}));
    }
  }

  return (function) ? function_sort(Args, Type) : Type;
}

sort_expression
data_type_checker::determine_allowed_type(const data_expression& x, const sort_expression& expected_sort) const
{
  if (is_variable(x))
  {
    variable v(x);
    // Set the type to one option in possible sorts, if there are more options.
    const sort_expression new_type = detail::replace_possible_sorts(expected_sort);
    v = variable(v.name(), new_type);
    return new_type;
  }

  assert(expected_sort.defined());

  sort_expression Type = expected_sort;
  // If d is not a variable it is an untyped name, or a function symbol.
  const core::identifier_string& data_term_name = data::is_untyped_identifier(x) ?
                                                  atermpp::down_cast<const untyped_identifier>(x).name() :
                                                  (atermpp::down_cast<const data::function_symbol>(x).name());

  if (data::detail::if_symbol() == data_term_name)
  {
    sort_expression NewType;
    if (!match_if(atermpp::down_cast<function_sort>(Type), NewType))
    {
      throw mcrl2::runtime_error(
              "The function if has incompatible argument types " + data::pp(Type) + " (while typechecking " +
              data::pp(x) + ").");
    }
    Type = NewType;
  }

  if (data::detail::equal_symbol() == data_term_name
      || data::detail::not_equal_symbol() == data_term_name
      || data::detail::less_symbol() == data_term_name
      || data::detail::less_equal_symbol() == data_term_name
      || data::detail::greater_symbol() == data_term_name
      || data::detail::greater_equal_symbol() == data_term_name
          )
  {
    sort_expression NewType;
    if (!match_relational_operators(atermpp::down_cast<function_sort>(Type), NewType))
    {
      throw mcrl2::runtime_error(
              "The function " + core::pp(data_term_name) + " has incompatible argument types " + data::pp(Type) +
              " (while typechecking " + data::pp(x) + ").");
    }
    Type = NewType;
  }

  if (sort_nat::sqrt_name() == data_term_name)
  {
    sort_expression NewType;
    if (!match_sqrt(atermpp::down_cast<function_sort>(Type), NewType))
    {
      throw mcrl2::runtime_error(
              "The function sqrt has an incorrect argument types " + data::pp(Type) + " (while typechecking " +
              data::pp(x) + ").");
    }
    Type = NewType;
  }

  if (sort_list::cons_name() == data_term_name)
  {
    sort_expression NewType;
    if (!match_cons(atermpp::down_cast<function_sort>(Type), NewType))
    {
      throw mcrl2::runtime_error(
              "The function |> has incompatible argument types " + data::pp(Type) + " (while typechecking " +
              data::pp(x) + ").");
    }
    Type = NewType;
  }

  if (sort_list::snoc_name() == data_term_name)
  {
    sort_expression NewType;
    if (!match_snoc(atermpp::down_cast<function_sort>(Type), NewType))
    {
      throw mcrl2::runtime_error(
              "The function <| has incompatible argument types " + data::pp(Type) + " (while typechecking " +
              data::pp(x) + ").");
    }
    Type = NewType;
  }

  if (sort_list::concat_name() == data_term_name)
  {
    sort_expression NewType;
    if (!match_concat(atermpp::down_cast<function_sort>(Type), NewType))
    {
      throw mcrl2::runtime_error(
              "The function ++ has incompatible argument types " + data::pp(Type) + " (while typechecking " +
              data::pp(x) + ").");
    }
    Type = NewType;
  }

  if (sort_list::element_at_name() == data_term_name)
  {
    sort_expression NewType;
    if (!match_element_at(atermpp::down_cast<function_sort>(Type), NewType))
    {
      throw mcrl2::runtime_error(
              "The function @ has incompatible argument types " + data::pp(Type) + " (while typechecking " +
              data::pp(x) + ").");
    }
    Type = NewType;
  }

  if (sort_list::head_name() == data_term_name ||
      sort_list::rhead_name() == data_term_name)
  {

    sort_expression NewType;
    if (!match_head(atermpp::down_cast<function_sort>(Type), NewType))
    {
      throw mcrl2::runtime_error(
              "The function {R,L}head has incompatible argument types " + data::pp(Type) + " (while typechecking " +
              data::pp(x) + ").");
    }
    Type = NewType;
  }

  if (sort_list::tail_name() == data_term_name ||
      sort_list::rtail_name() == data_term_name)
  {
    sort_expression NewType;
    if (!match_tail(atermpp::down_cast<function_sort>(Type), NewType))
    {
      throw mcrl2::runtime_error(
              "The function {R,L}tail has incompatible argument types " + data::pp(Type) + " (while typechecking " +
              data::pp(x) + ").");
    }
    Type = NewType;
  }

  if (sort_bag::set2bag_name() == data_term_name)
  {
    sort_expression NewType;
    if (!match_set2bag(atermpp::down_cast<function_sort>(Type), NewType))
    {
      throw mcrl2::runtime_error(
              "The function Set2Bag has incompatible argument types " + data::pp(Type) + " (while typechecking " +
              data::pp(x) + ").");
    }
    Type = NewType;
  }

  if (sort_list::in_name() == data_term_name)
  {
    sort_expression NewType;
    if (!match_in(atermpp::down_cast<function_sort>(Type), NewType))
    {
      throw mcrl2::runtime_error("The function {List,Set,Bag}In has incompatible argument types " + data::pp(Type) +
                                 " (while typechecking " + data::pp(x) + ").");
    }
    Type = NewType;
  }

  if (sort_set::union_name() == data_term_name ||
      sort_set::difference_name() == data_term_name ||
      sort_set::intersection_name() == data_term_name)
  {
    sort_expression NewType;
    if (!match_set_bag_operations(atermpp::down_cast<function_sort>(Type), NewType))
    {
      throw mcrl2::runtime_error(
              "The function {Set,Bag}{Union,Difference,Intersect} has incompatible argument types " + data::pp(Type) +
              " (while typechecking " + data::pp(x) + ").");
    }
    Type = NewType;
  }


  if (sort_fset::insert_name() == data_term_name)
  {
    sort_expression NewType;
    if (!match_fset_insert(atermpp::down_cast<function_sort>(Type), NewType))
    {
      throw mcrl2::runtime_error(
              "Set enumeration has incompatible argument types " + data::pp(Type) + " (while typechecking " +
              data::pp(x) + ").");
    }
    Type = NewType;
  }

  if (sort_fbag::cinsert_name() == data_term_name)
  {
    sort_expression NewType;
    if (!match_fbag_cinsert(atermpp::down_cast<function_sort>(Type), NewType))
    {
      throw mcrl2::runtime_error(
              "Bag enumeration has incompatible argument types " + data::pp(Type) + " (while typechecking " +
              data::pp(x) + ").");
    }
    Type = NewType;
  }


  if (sort_set::complement_name() == data_term_name)
  {
    sort_expression NewType;
    if (!match_set_complement(atermpp::down_cast<function_sort>(Type), NewType))
    {
      throw mcrl2::runtime_error(
              "The function SetCompl has incompatible argument types " + data::pp(Type) + " (while typechecking " +
              data::pp(x) + ").");
    }
    Type = NewType;
  }

  if (sort_bag::bag2set_name() == data_term_name)
  {
    sort_expression NewType;
    if (!match_bag2set(atermpp::down_cast<function_sort>(Type), NewType))
    {
      throw mcrl2::runtime_error(
              "The function Bag2Set has incompatible argument types " + data::pp(Type) + " (while typechecking " +
              data::pp(x) + ").");
    }
    Type = NewType;
  }

  if (sort_bag::count_name() == data_term_name)
  {
    sort_expression NewType;
    if (!match_bag_count(atermpp::down_cast<function_sort>(Type), NewType))
    {
      throw mcrl2::runtime_error(
              "The function BagCount has incompatible argument types " + data::pp(Type) + " (while typechecking " +
              data::pp(x) + ").");
    }
    Type = NewType;
  }


  if (data::function_update_name() == data_term_name)
  {
    sort_expression NewType;
    if (!match_function_update(atermpp::down_cast<function_sort>(Type), NewType))
    {
      throw mcrl2::runtime_error(
              "Function update has incompatible argument types " + data::pp(Type) + " (while typechecking " +
              data::pp(x) + ").");
    }
    Type = NewType;
  }

  if (sort_set::constructor_name() == data_term_name)
  {
    sort_expression NewType;
    if (!match_set_constructor(atermpp::down_cast<function_sort>(Type), NewType))
    {
      throw mcrl2::runtime_error(
              "Set constructor has incompatible argument types " + data::pp(Type) + " (while typechecking " +
              data::pp(x) + ").");
    }
    Type = NewType;
  }


  if (sort_bag::constructor_name() == data_term_name)
  {
    sort_expression NewType;
    if (!match_bag_constructor(atermpp::down_cast<function_sort>(Type), NewType))
    {
      throw mcrl2::runtime_error(
              "Bag constructor has incompatible argument types " + data::pp(Type) + " (while typechecking " +
              data::pp(x) + ").");
    }
    Type = NewType;
  }

  if (sort_set::false_function_name() == data_term_name)
  {
    sort_expression NewType;
    if (!match_false(atermpp::down_cast<function_sort>(Type), NewType))
    {
      throw mcrl2::runtime_error(
              "Bag constructor has incompatible argument types " + data::pp(Type) + " (while typechecking " +
              data::pp(x) + ").");
    }
    Type = NewType;
  }

  if (sort_bag::zero_function_name() == data_term_name)
  {
    sort_expression NewType;
    if (!match_bag_constructor(atermpp::down_cast<function_sort>(Type), NewType))
    {
      throw mcrl2::runtime_error(
              "Bag constructor has incompatible argument types " + data::pp(Type) + " (while typechecking " +
              data::pp(x) + ").");
    }
    Type = NewType;
  }

  return Type;
}

// N.B. The parameter parameter_count seems to have a special value std::string::npos, which is awkward (Wieger)
// std::string::npos for parameter_count means the number of arguments is not known.
data_expression data_type_checker::typecheck_n(const data_expression& x, const sort_expression& expected_sort,
                                               const detail::variable_context& declared_variables,
                                               bool strictly_ambiguous,
                                               size_t parameter_count, bool warn_upcasting, bool print_cast_error) const
{
  data_expression x1 = x;
  sort_expression expected_sort1 = expected_sort;
  if (data::is_untyped_identifier(x1) || data::is_function_symbol(x1))
  {
    core::identifier_string name = data::is_untyped_identifier(x1) ? atermpp::down_cast<const untyped_identifier>(x1).name()
                                                                  : atermpp::down_cast<const function_symbol>(x1).name();

    bool variable_ = false;
    bool TypeADefined = false;
    sort_expression TypeA;

    auto i1 = declared_variables.context().find(name);
    if (i1 != declared_variables.context().end())
    {
      TypeA = normalize_sort(i1->second);
      TypeADefined = true;
      if (is_function_sort(TypeA) ? (function_sort(TypeA).domain().size() == parameter_count) : parameter_count == 0)
      {
        variable_ = true;
      }
      else
      {
        TypeADefined = false;
      }
    }

    sort_expression_list ParList;
    if (parameter_count == 0)
    {
      auto i = declared_variables.context().find(name);
      if (i != declared_variables.context().end())
      {
        TypeA = normalize_sort(i->second);
        sort_expression temp;
        if (!match_sorts(TypeA, expected_sort1, temp))
        {
          throw mcrl2::runtime_error("The type " + data::pp(TypeA) + " of variable " + core::pp(name) + " is incompatible with " + data::pp(expected_sort1) + " (typechecking " + data::pp(x1) + ").");
        }
        x1 = variable(name, TypeA);
        return x1;
      }
      else
      {
        auto j = user_constants.find(name);
        if (j != user_constants.end())
        {
          TypeA = j->second;
          sort_expression temp;
          if (!match_sorts(TypeA, expected_sort1, temp))
          {
            throw mcrl2::runtime_error("The type " + data::pp(TypeA) + " of constant " + core::pp(name) + " is incompatible with " + data::pp(expected_sort1) + " (typechecking " + data::pp(x1) + ").");
          }
          x1 = data::function_symbol(name, TypeA);
          return x1;
        }
        else
        {
          auto k = system_constants.find(name);

          if (k != system_constants.end())
          {
            ParList = k->second;
            if (ParList.size() == 1)
            {
              x1 = function_symbol(name, ParList.front());
              return x1;
            }
            else
            {
              x1 = data::function_symbol(name, data::untyped_sort());
              throw mcrl2::runtime_error("Ambiguous system constant " + core::pp(name) + ".");
            }
          }
          else
          {
            throw mcrl2::runtime_error("Unknown constant " + core::pp(name) + ".");
          }
        }
      }
    }

    if (TypeADefined)
    {
      ParList = sort_expression_list({normalize_sort(TypeA)});
    }
    else
    {
      auto j_context = user_functions.find(name);
      auto j_gssystem = system_functions.find(name);

      if (j_context == user_functions.end())
      {
        if (j_gssystem != system_functions.end())
        {
          ParList = j_gssystem->second; // The function only occurs in the system.
        }
        else // None are defined.
        {
          if (parameter_count != std::string::npos)
          {
            throw mcrl2::runtime_error("Unknown operation " + core::pp(name) + " with " + std::to_string(parameter_count) + " parameter" + ((parameter_count != 1) ? "s." : "."));
          }
          else
          {
            throw mcrl2::runtime_error("Unknown operation " + core::pp(name) + ".");
          }
        }
      }
      else if (j_gssystem == system_functions.end())
      {
        ParList = j_context->second; // only the context sorts are defined.
      }
      else  // Both are defined.
      {
        ParList = j_gssystem->second + j_context->second;
      }
    }

    sort_expression_list CandidateParList = ParList;

    {
      // filter ParList keeping only functions A_0#...#A_nFactPars->A
      sort_expression_list new_sorts;
      if (parameter_count != std::string::npos)
      {
        for (; !ParList.empty();
               ParList = ParList.tail())
        {
          const sort_expression& Par = ParList.front();
          if (!is_function_sort(Par))
          {
            continue;
          }
          if (atermpp::down_cast<function_sort>(Par).domain().size() != parameter_count)
          {
            continue;
          }
          new_sorts.push_front(Par);
        }
        ParList = atermpp::reverse(new_sorts);
      }

      if (!ParList.empty())
      {
        CandidateParList = ParList;
      }

      // filter ParList keeping only functions of the right type
      sort_expression_list BackupParList = ParList;
      new_sorts = sort_expression_list();
      for (; !ParList.empty();
             ParList = ParList.tail())
      {
        const sort_expression& Par = ParList.front();
        try
        {
          expected_sort1 = determine_allowed_type(x1, expected_sort1);  // XXXXXXXXXX
          sort_expression result;
          if (match_sorts(Par, expected_sort1, result))
          {
            new_sorts = detail::insert_sort_unique(new_sorts, result);
          }
        }
        catch (mcrl2::runtime_error&)
        {
          // Ignore the error. Just do not add the type to NewParList
        }
      }
      new_sorts = atermpp::reverse(new_sorts);

      if (new_sorts.empty())
      {
        //Ok, this looks like a type error. We are not that strict.
        //Pos can be Nat, or even Int...
        //So lets make PosType more liberal
        //We change every Pos to NotInferred(Pos,Nat,Int)...
        //and get the list. Then we take the min of the list.

        ParList = BackupParList;
        expected_sort1 = expand_numeric_types_up(expected_sort1);
        for (const sort_expression& Par: ParList)
        {
          sort_expression result;
          if (match_sorts(Par, expected_sort1, result))
          {
            new_sorts = detail::insert_sort_unique(new_sorts, result);
          }
        }
        new_sorts = atermpp::reverse(new_sorts);
        if (new_sorts.size() > 1)
        {
          new_sorts = sort_expression_list({new_sorts.front()});
        }
      }

      if (new_sorts.empty())
      {
        //Ok, casting of the arguments did not help.
        //Let's try to be more relaxed about the result, e.g. returning Pos or Nat is not a bad idea for int.

        ParList = BackupParList;

        expected_sort1 = expand_numeric_types_down(expand_numeric_types_up(expected_sort1));
        for (const sort_expression& Par: ParList)
        {
          sort_expression result;
          if (match_sorts(Par, expected_sort1, result))
          {
            new_sorts = detail::insert_sort_unique(new_sorts, result);
          }
        }
        new_sorts = atermpp::reverse(new_sorts);
        if (new_sorts.size() > 1)
        {
          new_sorts = sort_expression_list({new_sorts.front()});
        }
      }

      ParList = new_sorts;
    }
    if (ParList.empty())
    {
      //provide some information to the upper layer for a better error message
      sort_expression Sort;
      if (CandidateParList.size() == 1)
      {
        Sort = CandidateParList.front();
      }
      else
      {
        Sort = untyped_possible_sorts(sort_expression_list(CandidateParList));
      }
      x1 = data::function_symbol(name, Sort);
      if (parameter_count != std::string::npos)
      {
        throw mcrl2::runtime_error("Unknown operation/variable " + core::pp(name)
                                   + " with " + std::to_string(parameter_count) + " argument" +
                                   ((parameter_count != 1) ? "s" : "")
                                   + " that matches type " + data::pp(expected_sort1) + ".");
      }
      else
      {
        throw mcrl2::runtime_error("Unknown operation/variable " + core::pp(name) + " that matches type " + data::pp(expected_sort1) + ".");
      }
    }

    if (ParList.size() == 1)
    {
      // replace PossibleSorts by a single possibility.
      sort_expression Type = ParList.front();

      sort_expression OldType = Type;
      bool result = true;
      assert(Type.defined());
      if (detail::has_unknown(Type))
      {
        sort_expression new_type;
        result = match_sorts(Type, expected_sort1, new_type);
        Type = new_type;
      }

      if (detail::has_unknown(Type) && data::is_function_symbol(x1))
      {
        sort_expression new_type;
        result = match_sorts(Type, x1.sort(), new_type);
        Type = new_type;
      }

      if (!result)
      {
        throw mcrl2::runtime_error("Fail to match sort " + data::pp(OldType) + " with " + data::pp(expected_sort1) + ".");
      }

      Type = determine_allowed_type(x1, Type);
      Type = detail::replace_possible_sorts(Type); // Set the type to one option in possible sorts, if there are more options.

      if (variable_)
      {
        x1 = variable(name, Type);
      }
      else if (is_untyped_identifier(x1))
      {
        x1 = data::function_symbol(untyped_identifier(x1).name(), Type);
      }
      else
      {
        x1 = data::function_symbol(function_symbol(x1).name(), Type);
      }
      assert(Type.defined());
      return x1;
    }
    else
    {
      if (strictly_ambiguous)
      {
        if (parameter_count != std::string::npos)
        {
          throw mcrl2::runtime_error(
                  "Ambiguous operation " + core::pp(name) + " with " + std::to_string(parameter_count) + " parameter" +
                  ((parameter_count != 1) ? "s." : "."));
        }
        else
        {
          throw mcrl2::runtime_error("Ambiguous operation " + core::pp(name) + ".");
        }
      }
      else
      {
        return data::variable(core::empty_identifier_string(), data::untyped_sort());
      }
    }
  }
  else
  {
    return typecheck(x1, expected_sort1, declared_variables, strictly_ambiguous, warn_upcasting, print_cast_error);
  }
}

data_expression data_type_checker::typecheck_abstraction(const data_expression& x, const sort_expression& expected_sort,
                                                         const detail::variable_context& declared_variables, bool strictly_ambiguous,
                                                         bool warn_upcasting, bool print_cast_error) const
{
  data_expression x1 = x;
  const auto& x_ = atermpp::down_cast<const abstraction>(x1);
  //The variable declaration of a binder should have at least 1 declaration
  if (x_.variables().empty())
  {
    throw mcrl2::runtime_error("Binder " + data::pp(x1) + " should have at least one declared variable.");
  }

  const binder_type& binding_operator = x_.binding_operator();
  if (is_untyped_set_or_bag_comprehension_binder(binding_operator) ||
      is_set_comprehension_binder(binding_operator) ||
      is_bag_comprehension_binder(binding_operator))
  {
    const variable_list& comprehension_variables = x_.variables();
    // Type check the comprehension_variables.
    try
    {
      (*this)(comprehension_variables, declared_variables);
    }
    catch (mcrl2::runtime_error& e)
    {
      throw mcrl2::runtime_error(std::string(e.what()) + "\nError occurred while typechecking the bag/set " + data::pp(x1) + ".");
    }

    //A Set/bag comprehension should have exactly one variable declared
    if (comprehension_variables.size() != 1)
    {
      throw mcrl2::runtime_error("Set/bag comprehension " + data::pp(x1) + " should have exactly one declared variable.");
    }

    const sort_expression& element_sort = comprehension_variables.front().sort();

    detail::variable_context declared_variables_copy(declared_variables);
    declared_variables_copy.add_context_variables(comprehension_variables);

    data_expression body = x_.body();

    sort_expression ResType;
    sort_expression NewType;
    try
    {
      body = typecheck(body, data::untyped_sort(), declared_variables_copy, strictly_ambiguous, warn_upcasting, print_cast_error);
      ResType = body.sort();
    }
    catch (mcrl2::runtime_error& e)
    {
      throw mcrl2::runtime_error(std::string(e.what()) + "\nThe condition or count of a set/bag comprehension " + data::pp(x1) + " cannot be determined.");
    }
    sort_expression temp;
    if (match_sorts(sort_bool::bool_(), ResType, temp))
    {
      NewType = sort_set::set_(element_sort);
      x1 = abstraction(set_comprehension_binder(), comprehension_variables, body);
    }
    else if (match_sorts(sort_nat::nat(), ResType, temp))
    {
      NewType = sort_bag::bag(element_sort);
      x1 = abstraction(bag_comprehension_binder(), comprehension_variables, body);
    }
    else if (match_sorts(sort_pos::pos(), ResType, temp))
    {
      NewType = sort_bag::bag(element_sort);
      body = application(sort_nat::cnat(), body);
      x1 = abstraction(bag_comprehension_binder(), comprehension_variables, body);
    }
    else
    {
      throw mcrl2::runtime_error("The condition or count of a set/bag comprehension is not of sort Bool, Nat or Pos, but of sort " + data::pp(ResType) + ".");
    }

    if (!match_sorts(NewType, expected_sort, NewType))
    {
      throw mcrl2::runtime_error("A set or bag comprehension of type " + data::pp(element_sort) + " does not match possible type " + data::pp(expected_sort) + " (while typechecking " + data::pp(x1) + ").");
    }

    return x1;
  }

  if (is_forall_binder(binding_operator) || is_exists_binder(binding_operator))
  {
    const variable_list& bound_variables = x_.variables();

    // Type check the variables.
    try
    {
      (*this)(bound_variables, declared_variables);
    }
    catch (mcrl2::runtime_error& e)
    {
      throw mcrl2::runtime_error(std::string(e.what()) + "\nError occurred while typechecking the quantification " + data::pp(x1) + ".");
    }

    detail::variable_context declared_variables_copy(declared_variables);
    declared_variables_copy.add_context_variables(bound_variables);

    data_expression body = x_.body();
    sort_expression temp;
    if (!match_sorts(sort_bool::bool_(), expected_sort, temp))
    {
      throw mcrl2::runtime_error("The type of an exist/forall for " + data::pp(x1) + " cannot be determined.");
    }
    body = typecheck(body, sort_bool::bool_(), declared_variables_copy, strictly_ambiguous, warn_upcasting, print_cast_error);
    sort_expression NewType = body.sort();

    if (!match_sorts(sort_bool::bool_(), NewType, temp))
    {
      throw mcrl2::runtime_error("The type of an exist/forall for " + data::pp(x1) + " cannot be determined.");
    }

    x1 = abstraction(binding_operator, bound_variables, body);
    return x1;
  }

  if (is_lambda_binder(binding_operator))
  {
    const variable_list& bound_variables = x_.variables();
    // Typecheck the variables.
    try
    {
      (*this)(bound_variables, declared_variables);
    }
    catch (mcrl2::runtime_error& e)
    {
      throw mcrl2::runtime_error(std::string(e.what()) + "\nError occurred while typechecking the lambda expression " + data::pp(x1) + ".");
    }

    detail::variable_context CopyDeclaredVars(declared_variables);
    CopyDeclaredVars.add_context_variables(bound_variables);

    sort_expression_list ArgTypes = detail::variable_list_sorts(bound_variables);
    sort_expression NewType;
    if (!UnArrowProd(ArgTypes, expected_sort, NewType))
    {
      throw mcrl2::runtime_error("No functions with arguments " + data::pp(ArgTypes) + " among " + data::pp(expected_sort) + " (while typechecking " + data::pp(x1) + ").");
    }
    data_expression body = x_.body();

    try
    {
      body = typecheck(body, NewType, CopyDeclaredVars, strictly_ambiguous, warn_upcasting, print_cast_error);
      NewType = body.sort();
    }
    catch (mcrl2::runtime_error& e)
    {
      throw e;
    }

    x1 = abstraction(binding_operator, bound_variables, body);
    // return function_sort(ArgTypes, NewType);
    return x1;
  }
  throw mcrl2::runtime_error("Internal type checking error: " + data::pp(x) + " does not match any type checking case.");
}

data_expression data_type_checker::typecheck_where_clause(const where_clause& x, const sort_expression& expected_sort,
                                             const detail::variable_context& declared_variables, bool strictly_ambiguous, bool warn_upcasting, bool print_cast_error) const
{
  variable_list WhereVarList;
  assignment_list NewWhereList;
  for (const auto& declaration: x.declarations())
  {
    data_expression WhereTerm;
    variable NewWhereVar;
    if (data::is_untyped_identifier_assignment(declaration))
    {
      const auto& t = atermpp::down_cast<const data::untyped_identifier_assignment>(declaration);
      WhereTerm = t.rhs();
      WhereTerm = typecheck(WhereTerm, data::untyped_sort(), declared_variables, strictly_ambiguous, warn_upcasting, print_cast_error);
      sort_expression WhereType = WhereTerm.sort();
      NewWhereVar = variable(t.lhs(), WhereType);
    }
    else
    {
      const auto& t = atermpp::down_cast<assignment>(declaration);
      WhereTerm = t.rhs();
      NewWhereVar = t.lhs();
      WhereTerm = typecheck(WhereTerm, NewWhereVar.sort(), declared_variables, strictly_ambiguous, warn_upcasting, print_cast_error);
      sort_expression WhereType = WhereTerm.sort();
    }
    WhereVarList.push_front(NewWhereVar);
    NewWhereList.push_front(assignment(NewWhereVar, WhereTerm));
  }
  NewWhereList = atermpp::reverse(NewWhereList);

  variable_list where_variables = atermpp::reverse(WhereVarList);
  try
  {
    (*this)(where_variables, declared_variables);
  }
  catch (mcrl2::runtime_error& e)
  {
    throw mcrl2::runtime_error(std::string(e.what()) + "\nError occurred while typechecking the where expression " + data::pp(x) + ".");
  }

  detail::variable_context declared_variables_copy(declared_variables);
  declared_variables_copy.add_context_variables(where_variables);

  data_expression body = x.body();
  body = typecheck(body, expected_sort, declared_variables_copy, strictly_ambiguous, warn_upcasting, print_cast_error);
  sort_expression NewType = body.sort();
  return where_clause(body, NewWhereList);
}

data_expression data_type_checker::typecheck_application(const application& x, const sort_expression& expected_sort,
                                                         const detail::variable_context& declared_variables, bool strictly_ambiguous, bool warn_upcasting, bool print_cast_error) const
{
  //arguments
  std::size_t parameter_count = x.size();

  //The following is needed to check enumerations
  const data_expression& Arg0 = x.head();
  if (data::is_function_symbol(Arg0) || data::is_untyped_identifier(Arg0))
  {
    core::identifier_string Name = is_function_symbol(Arg0) ? atermpp::down_cast<function_symbol>(Arg0).name() : atermpp::down_cast<untyped_identifier>(Arg0).name();
    if (Name == sort_list::list_enumeration_name())
    {
      sort_expression Type;
      if (!UnList(expected_sort, Type))
      {
        throw mcrl2::runtime_error("It is not possible to cast list to " + data::pp(expected_sort) + " (while typechecking " + data::pp(data_expression_list(x.begin(), x.end())) + ").");
      }

      //First time to determine the common type only!
      data_expression_list NewArguments;
      bool Type_is_stable = true;
      for (data_expression Argument: x)
      {
        sort_expression Type0;
        try
        {
          Argument = typecheck(Argument, Type, declared_variables, strictly_ambiguous, warn_upcasting, false);
          Type0 = Argument.sort();
        }
        catch (mcrl2::runtime_error&)
        {
          // Try again, but now without Type as the suggestion.
          // If this does not work, it will be caught in the second pass below.
          Argument = typecheck(Argument, data::untyped_sort(), declared_variables, strictly_ambiguous, warn_upcasting, print_cast_error);
          Type0 = Argument.sort();
        }
        NewArguments.push_front(Argument);
        Type_is_stable = Type_is_stable && (Type == Type0);
        Type = Type0;
      }
      // Arguments=OldArguments;

      //Second time to do the real work, but only if the elements in the list have different types.
      if (!Type_is_stable)
      {
        NewArguments = data_expression_list();
        for (data_expression arg: x)
        {
          arg = typecheck(arg, Type, declared_variables, strictly_ambiguous, warn_upcasting, print_cast_error);
          sort_expression Type0 = arg.sort();
          NewArguments.push_front(arg);
          Type = Type0;
        }
      }

      Type = sort_list::list(Type);
      return sort_list::list_enumeration(Type, data_expression_list(atermpp::reverse(NewArguments)));
    }

    if (Name == sort_set::set_enumeration_name())
    {
      sort_expression Type;
      if (!UnFSet(expected_sort, Type))
      {
        throw mcrl2::runtime_error("It is not possible to cast set to " + data::pp(expected_sort) + " (while typechecking " + data::pp(data_expression_list(x.begin(), x.end())) + ").");
      }

      //First time to determine the common type only (which will be NewType)!
      bool NewTypeDefined = false;
      sort_expression NewType;
      for (auto i = x.begin(); i != x.end(); ++i)
      {
        data_expression Argument = *i;
        sort_expression Type0;
        try
        {
          Argument = typecheck(Argument, Type, declared_variables, strictly_ambiguous, warn_upcasting, print_cast_error);
          Type0 = Argument.sort();
        }
        catch (mcrl2::runtime_error& e)
        {
          throw mcrl2::runtime_error(std::string(e.what()) + "\nImpossible to cast element to " + data::pp(Type) + " (while typechecking " + data::pp(Argument) + ").");
        }

        sort_expression OldNewType = NewType;
        if (!NewTypeDefined)
        {
          NewType = Type0;
          NewTypeDefined = true;
        }
        else
        {
          sort_expression temp;
          if (!maximum_type(NewType, Type0, temp))
          {
            throw mcrl2::runtime_error("Set contains incompatible elements of sorts " + data::pp(OldNewType) + " and " + data::pp(Type0) + " (while typechecking " + data::pp(Argument) + ".");
          }
          NewType = temp;
          NewTypeDefined = true;
        }
      }

      // Type is now the most generic type to be used.
      assert(Type.defined());
      assert(NewTypeDefined);
      Type = NewType;
      // Arguments=OldArguments;

      //Second time to do the real work.
      data_expression_list NewArguments;
      for (auto i = x.begin(); i != x.end(); ++i)
      {
        data_expression Argument = *i;
        sort_expression Type0;
        try
        {
          Argument = typecheck(Argument, Type, declared_variables, strictly_ambiguous, warn_upcasting, print_cast_error);
          Type0 = Argument.sort();
        }
        catch (mcrl2::runtime_error& e)
        {
          throw mcrl2::runtime_error(std::string(e.what()) + "\nImpossible to cast element to " + data::pp(Type) + " (while typechecking " + data::pp(Argument) + ").");
        }
        NewArguments.push_front(Argument);
        Type = Type0;
      }
      data_expression x2 = sort_set::set_enumeration(Type, data_expression_list(atermpp::reverse(NewArguments)));
      if (sort_set::is_set(expected_sort))
      {
        return sort_set::constructor(Type, sort_set::false_function(Type), x2);
      }
      // return sort_fset::fset(Type));
      return x2;
    }
    if (Name == sort_bag::bag_enumeration_name())
    {
      sort_expression Type;
      if (!UnFBag(expected_sort, Type))
      {
        throw mcrl2::runtime_error("Impossible to cast bag to " + data::pp(expected_sort) + "(while typechecking " + data::pp(data_expression_list(x.begin(), x.end())) + ").");
      }

      //First time to determine the common type only!
      sort_expression NewType;
      bool NewTypeDefined = false;
      for (application::const_iterator i = x.begin(); i != x.end(); ++i)
      {
        data_expression Argument0 = *i;
        ++i;
        data_expression Argument1 = *i;
        sort_expression Type0;
        try
        {
          Argument0 = typecheck(Argument0, Type, declared_variables, strictly_ambiguous, warn_upcasting, print_cast_error);
          Type0 = Argument0.sort();
        }
        catch (mcrl2::runtime_error& e)
        {
          throw mcrl2::runtime_error(std::string(e.what()) + "\nImpossible to cast element to " + data::pp(Type) + " (while typechecking " + data::pp(Argument0) + ").");
        }
        sort_expression Type1;
        try
        {
          Argument1 = typecheck(Argument1, sort_nat::nat(), declared_variables, strictly_ambiguous, warn_upcasting, print_cast_error);
          Type1 = Argument1.sort();
        }
        catch (mcrl2::runtime_error& e)
        {
          if (print_cast_error)
          {
            throw mcrl2::runtime_error(std::string(e.what()) + "\nImpossible to cast number to " + data::pp(sort_nat::nat()) + " (while typechecking " + data::pp(Argument1) + ").");
          }
          else
          {
            throw e;
          }
        }
        sort_expression OldNewType = NewType;
        if (!NewTypeDefined)
        {
          NewType = Type0;
          NewTypeDefined = true;
        }
        else
        {
          sort_expression temp;
          if (!maximum_type(NewType, Type0, temp))
          {
            throw mcrl2::runtime_error("Bag contains incompatible elements of sorts " + data::pp(OldNewType) + " and " + data::pp(Type0) + " (while typechecking " + data::pp(Argument0) + ").");
          }

          NewType = temp;
          NewTypeDefined = true;
        }
      }
      assert(Type.defined());
      assert(NewTypeDefined);
      Type = NewType;
      // Arguments=OldArguments;

      //Second time to do the real work.
      data_expression_list NewArguments;
      for (auto i = x.begin(); i != x.end(); ++i)
      {
        data_expression Argument0 = *i;
        ++i;
        data_expression Argument1 = *i;
        sort_expression Type0;
        try
        {
          Argument0 = typecheck(Argument0, Type, declared_variables, strictly_ambiguous, warn_upcasting,
                                print_cast_error);
          Type0 = Argument0.sort();
        }
        catch (mcrl2::runtime_error& e)
        {
          if (print_cast_error)
          {
            throw mcrl2::runtime_error(std::string(e.what()) + "\nImpossible to cast element to " + data::pp(Type) + " (while typechecking " + data::pp(Argument0) + ").");
          }
          else
          {
            throw e;
          }
        }
        sort_expression Type1;
        try
        {
          Argument1 = typecheck(Argument1, sort_nat::nat(), declared_variables, strictly_ambiguous, warn_upcasting, print_cast_error);
          Type1 = Argument1.sort();
        }
        catch (mcrl2::runtime_error& e)
        {
          if (print_cast_error)
          {
            throw mcrl2::runtime_error(std::string(e.what()) + "\nImpossible to cast number to " + data::pp(sort_nat::nat()) + " (while typechecking " + data::pp(Argument1) + ").");
          }
          else
          {
            throw e;
          }
        }
        NewArguments.push_front(Argument0);
        NewArguments.push_front(Argument1);
        Type = Type0;
      }
      data_expression x2 = sort_bag::bag_enumeration(Type, data_expression_list(reverse(NewArguments)));
      if (sort_bag::is_bag(expected_sort))
      {
        return sort_bag::constructor(Type, sort_bag::zero_function(Type), x2);
        // return check_result(sort_bag::bag(Type), x1);
      }
      //return check_result(sort_fbag::fbag(Type), x1);
      return x2;
    }
  }
  sort_expression_list NewArgumentTypes;
  data_expression_list NewArguments;
  sort_expression_list argument_sorts;
  for (auto i = x.begin(); i != x.end(); ++i)
  {
    data_expression Arg = *i;
    Arg = typecheck(Arg, data::untyped_sort(), declared_variables, false, warn_upcasting, print_cast_error);
    sort_expression Type = Arg.sort();
    assert(Type.defined());
    NewArguments.push_front(Arg);
    NewArgumentTypes.push_front(Type);
  }
  data_expression_list Arguments = atermpp::reverse(NewArguments);
  sort_expression_list ArgumentTypes = atermpp::reverse(NewArgumentTypes);

  //function
  data_expression head = x.head();
  sort_expression NewType;
  try
  {
    head = typecheck_n(head, function_sort(ArgumentTypes, expected_sort), declared_variables, false, parameter_count,
                       warn_upcasting, print_cast_error);
    NewType = head.sort();
  }
  catch (mcrl2::runtime_error& e)
  {
    throw mcrl2::runtime_error(std::string(e.what()) + "\nType error while trying to cast an application of " +
                               data::pp(head) + " to arguments " + data::pp(Arguments) + " to type " +
                               data::pp(expected_sort) + ".");
  }

  //it is possible that:
  //1) a cast has happened
  //2) some parameter Types became sharper.
  //we do the arguments again with the types.

  if (is_function_sort(normalize_sort(NewType)))
  {
    sort_expression_list expected_sorts = atermpp::down_cast<function_sort>(normalize_sort(NewType)).domain();

    if (expected_sorts.size() != Arguments.size())
    {
      throw mcrl2::runtime_error("Need argumens of sorts " + data::pp(expected_sorts) +
                                 " which does not match the number of provided arguments "
                                 + data::pp(Arguments) + " (while typechecking "
                                 + data::pp(x) + ").");
    }
    //arguments again
    sort_expression_list new_argument_sorts;
    data_expression_list new_arguments;
    for (; !Arguments.empty();
           Arguments = Arguments.tail(),
           ArgumentTypes = ArgumentTypes.tail(), expected_sorts = expected_sorts.tail())
    {
      assert(!Arguments.empty());
      assert(!expected_sorts.empty());
      data_expression Arg = Arguments.front();
      const sort_expression& NeededType = expected_sorts.front();
      sort_expression Type = ArgumentTypes.front();
      if (!equal_sorts(NeededType, Type))
      {
        //upcasting
        try
        {
          Arg = upcast_numeric_type(Arg, Type, NeededType, declared_variables, strictly_ambiguous, warn_upcasting, print_cast_error);
          Type = Arg.sort();
        }
        catch (mcrl2::runtime_error&)
        {
        }
      }
      if (!equal_sorts(NeededType, Type))
      {
        sort_expression NewArgType;
        if (!match_sorts(NeededType, Type, NewArgType))
        {
          if (!match_sorts(NeededType, expand_numeric_types_up(Type), NewArgType))
          {
            NewArgType = NeededType;
          }
        }
        try
        {
          Arg = typecheck(Arg, NewArgType, declared_variables, strictly_ambiguous, warn_upcasting, print_cast_error);
          NewArgType = Arg.sort();
        }
        catch (mcrl2::runtime_error& e)
        {
          throw mcrl2::runtime_error(
            std::string(e.what()) + "\nRequired type " + data::pp(NeededType) + " does not match possible type "
            + data::pp(Type) + " (while typechecking " + data::pp(Arg) + " in " + data::pp(x) + ").");
        }
        Type = NewArgType;
      }
      new_arguments.push_front(Arg);
      new_argument_sorts.push_front(Type);
    }
    Arguments = atermpp::reverse(new_arguments);
    ArgumentTypes = atermpp::reverse(new_argument_sorts);
  }

  //the function again
  try
  {
    head = typecheck_n(head, function_sort(ArgumentTypes, expected_sort), declared_variables, strictly_ambiguous,
                       parameter_count, warn_upcasting, print_cast_error);
    NewType = head.sort();
  }
  catch (mcrl2::runtime_error& e)
  {
    throw mcrl2::runtime_error(std::string(e.what()) + "\nType error while trying to cast " + data::pp(application(head, Arguments)) + " to type " + data::pp(expected_sort) + ".");
  }

  //and the arguments once more
  if (is_function_sort(normalize_sort(NewType)))
  {
    sort_expression_list expected_sorts = atermpp::down_cast<function_sort>(normalize_sort(NewType)).domain();
    sort_expression_list new_argument_sorts;
    data_expression_list new_arguments;
    for (; !Arguments.empty();
           Arguments = Arguments.tail(),
           ArgumentTypes = ArgumentTypes.tail(), expected_sorts = expected_sorts.tail())
    {
      data_expression Arg = Arguments.front();
      const sort_expression& NeededType = expected_sorts.front();
      sort_expression Type = ArgumentTypes.front();

      if (!equal_sorts(NeededType, Type))
      {
        //upcasting
        try
        {
          Arg = upcast_numeric_type(Arg, Type, NeededType, declared_variables, strictly_ambiguous, warn_upcasting, print_cast_error);
          Type = Arg.sort();
        }
        catch (mcrl2::runtime_error&)
        {
        }
      }
      if (!equal_sorts(NeededType, Type))
      {
        sort_expression NewArgType;
        if (!match_sorts(NeededType, Type, NewArgType))
        {
          if (!match_sorts(NeededType, expand_numeric_types_up(Type), NewArgType))
          {
            NewArgType = NeededType;
          }
        }
        try
        {
          Arg = typecheck(Arg, NewArgType, declared_variables, strictly_ambiguous, warn_upcasting, print_cast_error);
          NewArgType = Arg.sort();
        }
        catch (mcrl2::runtime_error& e)
        {
          throw mcrl2::runtime_error(
            std::string(e.what()) + "\nNeeded type " + data::pp(NeededType) + " does not match possible type "
            + data::pp(Type) + " (while typechecking " + data::pp(Arg) + " in " + data::pp(x) + ").");
        }
        Type = NewArgType;
      }

      new_arguments.push_front(Arg);
      new_argument_sorts.push_front(Type);
    }
    Arguments = atermpp::reverse(new_arguments);
    ArgumentTypes = atermpp::reverse(new_argument_sorts);
  }

  data_expression x2 = application(head, Arguments);

  if (is_function_sort(normalize_sort(NewType)))
  {
    // return atermpp::down_cast<function_sort>(normalize_sort(NewType)).codomain();
    return x2;
  }

  sort_expression temp_type;
  if (!UnArrowProd(ArgumentTypes, NewType, temp_type))
  {
    throw mcrl2::runtime_error("Fail to properly type " + data::pp(x2) + ".");
  }
  if (detail::has_unknown(temp_type))
  {
    throw mcrl2::runtime_error("Fail to properly type " + data::pp(x2) + ".");
  }
  // return check_result(temp_type, x1);
  return x2;
}

data_expression data_type_checker::typecheck_identifier_function_symbol_variable(const data_expression& x, const sort_expression& expected_sort,
                                             const detail::variable_context& declared_variables, bool strictly_ambiguous, bool warn_upcasting, bool print_cast_error) const
{
  data_expression x1 = x;
  core::identifier_string name = data::is_untyped_identifier(x1) ? atermpp::down_cast<untyped_identifier>(x1).name() :
                                 is_function_symbol(x1) ? atermpp::down_cast<function_symbol>(x1).name() :
                                 atermpp::down_cast<variable>(x1).name();
  if (utilities::is_numeric_string(name.function().name()))
  {
    sort_expression sort = sort_int::int_();
    if (detail::IsPos(name))
    {
      sort = sort_pos::pos();
    }
    else if (detail::IsNat(name))
    {
      sort = sort_nat::nat();
    }
    x1 = data::function_symbol(name, sort);

    sort_expression temp;
    if (match_sorts(sort, expected_sort, temp))
    {
      // return check_result(Sort, x1);
      return x1;
    }

    //upcasting
    try
    {
      x1 = upcast_numeric_type(x1, sort, expected_sort, declared_variables, strictly_ambiguous, warn_upcasting, print_cast_error);
    }
    catch (mcrl2::runtime_error& e)
    {
      throw mcrl2::runtime_error(std::string(e.what()) + "\nCannot (up)cast number " + data::pp(x1) + " to type " + data::pp(expected_sort) + ".");
    }
    // return check_result(CastedNewType, x1);
    return x1;
  }

  auto it = declared_variables.context().find(name);
  if (it != declared_variables.context().end())
  {
    sort_expression sort = normalize_sort(it->second);
    x1 = variable(name, sort);

    sort_expression new_sort;
    if (match_sorts(sort, expected_sort, new_sort))
    {
      sort = new_sort;
    }
    else
    {
      //upcasting
      sort_expression CastedNewType;
      try
      {
        x1 = upcast_numeric_type(x1, sort, expected_sort, declared_variables, strictly_ambiguous, warn_upcasting, print_cast_error);
      }
      catch (mcrl2::runtime_error& e)
      {
        if (print_cast_error)
        {
          throw mcrl2::runtime_error(std::string(e.what()) + "\nCannot (up)cast variable " + data::pp(x1) + " to type " + data::pp(expected_sort) + ".");
        }
        else
        {
          throw e;
        }
      }
      sort = CastedNewType;
    }

    // return check_result(Type, x1);
    return x1;
  }

  auto i = user_constants.find(name);
  if (i != user_constants.end())
  {
    sort_expression sort = i->second;
    sort_expression new_sort;
    if (match_sorts(sort, expected_sort, new_sort))
    {
      sort = new_sort;
      return data::function_symbol(name, sort);
      // return check_result(Type, x1);
    }
    else
    {
      // The type cannot be unified. Try upcasting the type.
      x1 = data::function_symbol(name, sort);
      try
      {
        return upcast_numeric_type(x1, sort, expected_sort, declared_variables, strictly_ambiguous, warn_upcasting, print_cast_error);
      }
      catch (mcrl2::runtime_error& e)
      {
        throw mcrl2::runtime_error(std::string(e.what()) + "\nNo constant " + data::pp(x1) + " with type " + data::pp(expected_sort) + ".");
      }
    }
  }

  auto j = system_constants.find(name);
  if (j != system_constants.end())
  {
    sort_expression_list sorts = j->second;
    sort_expression_list NewParList;
    for (const auto& sort: sorts)
    {
      sort_expression result;
      if (match_sorts(sort, expected_sort, result))
      {
        x1 = data::function_symbol(name, result);
        NewParList.push_front(result);
      }
    }
    sort_expression_list ParList = atermpp::reverse(NewParList);
    if (ParList.empty())
    {
      // Try to do the matching again with relaxed typing.
      for (auto Type : sorts)
      {
        if (is_untyped_identifier(x1))
        {
          x1 = data::function_symbol(name, Type);
        }
        x1 = upcast_numeric_type(x1, Type, expected_sort, declared_variables, strictly_ambiguous, warn_upcasting, print_cast_error);
        Type = x1.sort();
        sort_expression result;
        if (match_sorts(Type, expected_sort, result))
        {
          NewParList.push_front(result);
        }
      }
      ParList = atermpp::reverse(NewParList);
    }

    if (ParList.empty())
    {
      throw mcrl2::runtime_error("No system constant " + data::pp(x1) + " with type " + data::pp(expected_sort) + ".");
    }

    if (ParList.size() == 1)
    {
      const sort_expression& Type = ParList.front();

      if (is_untyped_identifier(x1))
      {
        assert(0);
        x1 = data::function_symbol(name, Type);
      }
      try
      {
        return upcast_numeric_type(x1, Type, expected_sort, declared_variables, strictly_ambiguous, warn_upcasting, print_cast_error);
      }
      catch (mcrl2::runtime_error& e)
      {
        throw mcrl2::runtime_error(std::string(e.what()) + "\nNo constant " + data::pp(x1) + " with type " + data::pp(expected_sort) + ".");
      }
    }
    else
    {
      x1 = data::function_symbol(name, data::untyped_sort());
      //return check_result(data::untyped_sort(), x1);
      return x1;
    }
  }

  auto j_context = user_functions.find(name);
  auto j_gssystem = system_functions.find(name);

  sort_expression_list ParList;
  if (j_context == user_functions.end())
  {
    if (j_gssystem != system_functions.end())
    {
      ParList = j_gssystem->second; // The function only occurs in the system.
    }
    else
    {
      throw mcrl2::runtime_error("Unknown operation " + core::pp(name) + ".");
    }
  }
  else if (j_gssystem == system_functions.end())
  {
    ParList = j_context->second; // only the context sorts are defined.
  }
  else  // Both are defined.
  {
    ParList = j_gssystem->second + j_context->second;
  }

  if (ParList.size() == 1)
  {
    const sort_expression& Type = ParList.front();
    x1 = data::function_symbol(name, Type);
    try
    {
      return upcast_numeric_type(x1, Type, expected_sort, declared_variables, strictly_ambiguous, warn_upcasting, print_cast_error);
    }
    catch (mcrl2::runtime_error& e)
    {
      throw mcrl2::runtime_error(std::string(e.what()) + "\nNo constant " + data::pp(x1) + " with type " + data::pp(expected_sort) + ".");
    }
  }
  else
  {
    return typecheck_n(x1, expected_sort, declared_variables, strictly_ambiguous, std::string::npos, warn_upcasting, print_cast_error);
  }
}

inline
const structured_sort& find_structured_sort(const sort_expression& x, const sort_specification& sortspec)
{
  for (const auto& p: sortspec.sort_alias_map())
  {
    if (p.second == x && is_structured_sort(p.first))
    {
      return atermpp::down_cast<structured_sort>(p.first);
    }
  }
  throw mcrl2::runtime_error("Could not find a structured sort corresponding to " + data::pp(x));
}

inline
bool match_structured_sort_constructor(const structured_sort_constructor& x, const std::vector<core::identifier_string>& names)
{
  std::set<core::identifier_string> argument_names;
  for (const structured_sort_constructor_argument& arg: x.arguments())
  {
    argument_names.insert(arg.name());
  }
  for (const core::identifier_string& name: names)
  {
    if (argument_names.find(name) == argument_names.end())
    {
      return false;
    }
  }
  return true;
}

// makes assignments of the shape arg = arg(name), for each constructor argument arg
inline
data::untyped_identifier_assignment_list make_constructor_assignments(const structured_sort_constructor& x, const core::identifier_string& name)
{
  std::vector<untyped_identifier_assignment> assignments;
  for (const structured_sort_constructor_argument& arg: x.arguments())
  {
    assignments.emplace_back(arg.name(), application(untyped_identifier(arg.name()), data_expression_list({ untyped_identifier(name) })));
  }
  return data::untyped_identifier_assignment_list(assignments.begin(), assignments.end());
}

data_expression data_type_checker::typecheck_untyped_variable_assignment(const untyped_variable_assignment& x,
                                                                         const sort_expression& expected_sort,
                                                                         const detail::variable_context& declared_variables,
                                                                         bool strictly_ambiguous, bool warn_upcasting,
                                                                         bool print_cast_error) const
{
  m_checking_untyped_variable_assignment = true;

  // typecheck x.name() as a variable v, to obtain the corresponding sort x_sort
  sort_expression x_sort = typecheck_identifier_function_symbol_variable(untyped_identifier(x.name()), data::untyped_sort(), declared_variables, strictly_ambiguous, warn_upcasting, print_cast_error).sort();
  if (x_sort == untyped_sort())
  {
    throw mcrl2::runtime_error("Could not find a sort corresponding to the expression " + data::pp(x));
  }

  // find the structured sort S corresponding to x_sort
  const structured_sort& S = find_structured_sort(x_sort, m_sort_specification);

  // the left hand sides of the assignments in x
  std::vector<core::identifier_string> left_hand_sides;
  for (const untyped_identifier_assignment& a: x.assignments())
  {
    left_hand_sides.push_back(a.lhs());
  }

  // find the unique structured sort x_constructor that matches x
  std::vector<structured_sort_constructor> matches;
  for (const structured_sort_constructor& constructor: S.constructors())
  {
    if (match_structured_sort_constructor(constructor, left_hand_sides))
    {
      matches.push_back(constructor);
    }
  }
  if (matches.empty())
  {
    throw mcrl2::runtime_error("Could not find a structored sort constructor matching the assignments " + core::detail::print_list(x.assignments()));
  }
  if (matches.size() > 1)
  {
    throw mcrl2::runtime_error("Found multiple structored sorts matching the assignments " + core::detail::print_list(x.assignments()));
  }
  const structured_sort_constructor& x_constructor = matches.front();

  // create an application x1 that is equivalent to x
  std::vector<data_expression> arguments;
  for (const structured_sort_constructor_argument& arg: x_constructor.arguments())
  {
    bool found = false;
    for (const untyped_identifier_assignment& a: x.assignments())
    {
      if (a.lhs() == arg.name())
      {
        found = true;
        arguments.push_back(where_clause(a.rhs(), make_constructor_assignments(x_constructor, x.name())));
      }
    }
    if (!found)
    {
      arguments.push_back(application(untyped_identifier(arg.name()), data_expression_list({ untyped_identifier(x.name()) })));
    }
  }
  application x1(untyped_identifier(x_constructor.name()), data_expression_list(arguments.begin(), arguments.end()));

  auto result = typecheck(x1, untyped_sort(), declared_variables, strictly_ambiguous, warn_upcasting, print_cast_error);
  m_checking_untyped_variable_assignment = false;
  return result;
}

data_expression data_type_checker::typecheck(const data_expression& x, const sort_expression& expected_sort,
  const detail::variable_context& declared_variables, bool strictly_ambiguous, bool warn_upcasting, bool print_cast_error) const
{
  if (is_abstraction(x))
  {
    return typecheck_abstraction(x, expected_sort, declared_variables, strictly_ambiguous, warn_upcasting, print_cast_error);
  }
  else if (is_where_clause(x))
  {
    const auto& x_ = atermpp::down_cast<where_clause>(x);
    return typecheck_where_clause(x_, expected_sort, declared_variables, strictly_ambiguous, warn_upcasting, print_cast_error);
  }
  else if (is_application(x))
  {
    const auto& x_ = atermpp::down_cast<application>(x);
    return typecheck_application(x_, expected_sort, declared_variables, strictly_ambiguous, warn_upcasting, print_cast_error);
  }
  else if (is_untyped_identifier(x) || data::is_function_symbol(x) || is_variable(x))
  {
    return typecheck_identifier_function_symbol_variable(x, expected_sort, declared_variables, strictly_ambiguous, warn_upcasting, print_cast_error);
  }
  else if (is_untyped_variable_assignment(x))
  {
    const auto& x_ = atermpp::down_cast<untyped_variable_assignment>(x);
    return typecheck_untyped_variable_assignment(x_, expected_sort, declared_variables, strictly_ambiguous, warn_upcasting, print_cast_error);
  }
  throw mcrl2::runtime_error("Internal type checking error: " + data::pp(x) + " does not match any type checking case.");
}

void data_type_checker::read_constructors_and_mappings(const function_symbol_vector& constructors,
                                                       const function_symbol_vector& mappings,
                                                       const function_symbol_vector& normalized_constructors)
{
  std::size_t constr_number = constructors.size();
  function_symbol_vector functions_and_constructors = constructors;
  functions_and_constructors.insert(functions_and_constructors.end(), mappings.begin(), mappings.end());
  for (const function_symbol& Func: functions_and_constructors)
  {
    const core::identifier_string& FuncName = Func.name();
    sort_expression FuncType = Func.sort();

    check_sort_is_declared(FuncType);

    //if FuncType is a defined function sort, unwind it
    if (is_basic_sort(FuncType))
    {
      const sort_expression NewFuncType = normalize_sort(FuncType);
      if (is_function_sort(NewFuncType))
      {
        FuncType = NewFuncType;
      }
    }

    if (is_function_sort(FuncType))
    {
      add_function(data::function_symbol(FuncName, FuncType), "function");
    }
    else
    {
      try
      {
        add_constant(data::function_symbol(FuncName, FuncType), "constant");
      }
      catch (mcrl2::runtime_error& e)
      {
        throw mcrl2::runtime_error(std::string(e.what()) + "\nCould not add constant.");
      }
    }

    if (constr_number)
    {
      constr_number--;

      //Here checks for the constructors
      sort_expression ConstructorType = FuncType;
      if (is_function_sort(ConstructorType))
      {
        ConstructorType = atermpp::down_cast<function_sort>(ConstructorType).codomain();
      }
      ConstructorType = normalize_sort(ConstructorType);
      if (!is_basic_sort(ConstructorType) ||
          sort_bool::is_bool(ConstructorType) ||
          sort_pos::is_pos(ConstructorType) ||
          sort_nat::is_nat(ConstructorType) ||
          sort_int::is_int(ConstructorType) ||
          sort_real::is_real(ConstructorType)
              )
      {
        throw mcrl2::runtime_error(
                "Could not add constructor " + core::pp(FuncName) + " of sort " + data::pp(FuncType) +
                ". Constructors of built-in sorts are not allowed.");
      }
    }
  }

  // Check that the constructors are defined such that they cannot generate an empty sort.
  // E.g. in the specification sort D; cons f:D->D; the sort D must be necessarily empty, which is
  // forbidden. The function below checks whether such malicious specifications occur.

  check_for_empty_constructor_domains(normalized_constructors); // throws exception if not ok.
}

void data_type_checker::add_constant(const data::function_symbol& f, const std::string& msg)
{
  const core::identifier_string& name = f.name();
  const sort_expression& sort = f.sort();

  if (user_constants.count(name) > 0)
  {
    throw mcrl2::runtime_error("Double declaration of " + msg + " " + core::pp(name) + ".");
  }

  if (system_constants.count(name) > 0 || system_functions.count(name) > 0)
  {
    throw mcrl2::runtime_error("Attempt to declare a constant with the name that is a built-in identifier (" + core::pp(name) + ").");
  }
  user_constants[name] = normalize_sort(sort);
}


bool data_type_checker::match_sort_lists(
        const sort_expression_list& x1,
        const sort_expression_list& x2,
        sort_expression_list& result) const
{
  if (x1.size() != x2.size())
  {
    return false;
  }

  sort_expression_list Result;
  auto j = x2.begin();
  for (auto i = x1.begin();
       i != x1.end();
       ++i, ++j)
  {
    sort_expression sort;
    if (!match_sorts(*i, *j, sort))
    {
      return false;
    }
    Result.push_front(sort);
  }
  result = atermpp::reverse(Result);
  return true;
}

bool data_type_checker::match_sorts(const sort_expression& x1, const sort_expression& x2, sort_expression& result) const
{
  sort_expression sort1 = x1;
  sort_expression sort2 = x2;

  if (data::is_untyped_sort(sort1))
  {
    result = sort2;
    return true;
  }
  if (data::is_untyped_sort(sort2) || equal_sorts(sort1, sort2))
  {
    result = sort1;
    return true;
  }
  if (is_untyped_possible_sorts(sort1) && !is_untyped_possible_sorts(sort2))
  {
    sort2.swap(sort1);
  }
  if (is_untyped_possible_sorts(sort2))
  {
    sort_expression_list sorts;
    const auto& mps = atermpp::down_cast<const untyped_possible_sorts>(sort2);
    for (auto sort: mps.sorts())
    {
      sort_expression new_sort;
      if (match_sorts(sort1, sort, new_sort))
      {
        sort = new_sort;
        // Avoid double insertions.
        if (std::find(sorts.begin(), sorts.end(), sort) == sorts.end())
        {
          sorts.push_front(sort);
        }
      }
    }
    if (sorts.empty())
    {
      return false;
    }
    if (sorts.tail().empty())
    {
      result = sorts.front();
      return true;
    }

    result = untyped_possible_sorts(atermpp::reverse(sorts));
    return true;
  }

  if (is_basic_sort(sort1))
  {
    sort1 = normalize_sort(sort1);
  }
  if (is_basic_sort(sort2))
  {
    sort2 = normalize_sort(sort2);
  }
  if (is_container_sort(sort1))
  {
    const auto& s = atermpp::down_cast<container_sort>(sort1);
    const container_type& type = s.container_name();
    if (is_list_container(type))
    {
      if (!sort_list::is_list(sort2))
      {
        return false;
      }
      sort_expression sort;
      if (!match_sorts(s.element_sort(), atermpp::down_cast<container_sort>(sort2).element_sort(), sort))
      {
        return false;
      }
      result = sort_list::list(sort);
      return true;
    }

    if (is_set_container(type))
    {
      if (!sort_set::is_set(sort2))
      {
        return false;
      }
      else
      {
        sort_expression sort;
        if (!match_sorts(s.element_sort(), atermpp::down_cast<container_sort>(sort2).element_sort(), sort))
        {
          return false;
        }
        result = sort_set::set_(sort);
        return true;
      }
    }

    if (is_bag_container(type))
    {
      if (!sort_bag::is_bag(sort2))
      {
        return false;
      }
      else
      {
        sort_expression sort;
        if (!match_sorts(s.element_sort(), atermpp::down_cast<container_sort>(sort2).element_sort(), sort))
        {
          return false;
        }
        result = sort_bag::bag(sort);
        return true;
      }
    }

    if (is_fset_container(type))
    {
      if (!sort_fset::is_fset(sort2))
      {
        return false;
      }
      else
      {
        sort_expression sort;
        if (!match_sorts(s.element_sort(), atermpp::down_cast<container_sort>(sort2).element_sort(), sort))
        {
          return false;
        }
        result = sort_fset::fset(sort);
        return true;
      }
    }

    if (is_fbag_container(type))
    {
      if (sort_fbag::is_fbag(sort2))
      {
        sort_expression sort;
        if (!match_sorts(s.element_sort(), atermpp::down_cast<container_sort>(sort2).element_sort(), sort))
        {
          return false;
        }
        result = sort_fbag::fbag(sort);
        return true;
      }
      else
      {
        return false;
      }
    }
  }

  if (is_function_sort(sort1))
  {
    if (!is_function_sort(sort2))
    {
      return false;
    }
    else
    {
      const auto& fs = atermpp::down_cast<const function_sort>(sort1);
      const auto& posfs = atermpp::down_cast<const function_sort>(sort2);
      sort_expression_list ArgTypes;
      if (!match_sort_lists(fs.domain(), posfs.domain(), ArgTypes))
      {
        return false;
      }
      sort_expression ResType;
      if (!match_sorts(fs.codomain(), posfs.codomain(), ResType))
      {
        return false;
      }
      result = function_sort(ArgTypes, ResType);
      return true;
    }
  }

  return false;
}

void data_type_checker::add_system_constant(const data::function_symbol& x)
{
  const core::identifier_string& x_name = x.name();
  const sort_expression& x_sort = x.sort();
  sort_expression_list sorts;
  auto i = system_constants.find(x_name);
  if (i != system_constants.end())
  {
    sorts = i->second;
  }
  sorts = push_back(sorts, x_sort);
  system_constants[x_name] = sorts;
}

void data_type_checker::add_system_function(const data::function_symbol& x)
{
  const core::identifier_string& x_name = x.name();
  const sort_expression& x_sort = x.sort();
  sort_expression_list sorts;
  auto j = system_functions.find(x_name);
  if (j != system_functions.end())
  {
    sorts = j->second;
  }
  sorts = push_back(sorts, x_sort);
  system_functions[x_name] = sorts;
}


void data_type_checker::initialise_system_defined_functions()
{
  //Creation of operation identifiers for system defined operations.
  //Bool
  add_system_constant(sort_bool::true_());
  add_system_constant(sort_bool::false_());
  add_system_function(sort_bool::not_());
  add_system_function(sort_bool::and_());
  add_system_function(sort_bool::or_());
  add_system_function(sort_bool::implies());
  add_system_function(equal_to(data::untyped_sort()));
  add_system_function(not_equal_to(data::untyped_sort()));
  add_system_function(if_(data::untyped_sort()));
  add_system_function(less(data::untyped_sort()));
  add_system_function(less_equal(data::untyped_sort()));
  add_system_function(greater_equal(data::untyped_sort()));
  add_system_function(greater(data::untyped_sort()));
  //Numbers
  add_system_constant(sort_pos::c1());
  add_system_function(sort_pos::cdub());
  add_system_constant(sort_nat::c0());
  add_system_function(sort_nat::cnat());
  add_system_function(sort_nat::pos2nat());
  add_system_function(sort_nat::nat2pos());
  add_system_function(sort_int::cint());
  add_system_function(sort_int::cneg());
  add_system_function(sort_int::int2pos());
  add_system_function(sort_int::int2nat());
  add_system_function(sort_int::pos2int());
  add_system_function(sort_int::nat2int());
  add_system_function(sort_real::creal());
  add_system_function(sort_real::pos2real());
  add_system_function(sort_real::nat2real());
  add_system_function(sort_real::int2real());
  add_system_function(sort_real::real2pos());
  add_system_function(sort_real::real2nat());
  add_system_function(sort_real::real2int());

  //Square root for the natural numbers.
  add_system_function(sort_nat::sqrt());
  //more about numbers
  add_system_function(sort_real::maximum(sort_pos::pos(), sort_pos::pos()));
  add_system_function(sort_real::maximum(sort_pos::pos(), sort_nat::nat()));
  add_system_function(sort_real::maximum(sort_nat::nat(), sort_pos::pos()));
  add_system_function(sort_real::maximum(sort_nat::nat(), sort_nat::nat()));
  add_system_function(sort_real::maximum(sort_pos::pos(), sort_int::int_()));
  add_system_function(sort_real::maximum(sort_int::int_(), sort_pos::pos()));
  add_system_function(sort_real::maximum(sort_nat::nat(), sort_int::int_()));
  add_system_function(sort_real::maximum(sort_int::int_(), sort_nat::nat()));
  add_system_function(sort_real::maximum(sort_int::int_(), sort_int::int_()));
  add_system_function(sort_real::maximum(sort_real::real_(), sort_real::real_()));
  //more
  add_system_function(sort_real::minimum(sort_pos::pos(), sort_pos::pos()));
  add_system_function(sort_real::minimum(sort_nat::nat(), sort_nat::nat()));
  add_system_function(sort_real::minimum(sort_int::int_(), sort_int::int_()));
  add_system_function(sort_real::minimum(sort_real::real_(), sort_real::real_()));
  //more
  // add_system_function(sort_real::abs(sort_pos::pos()));
  // add_system_function(sort_real::abs(sort_nat::nat()));
  add_system_function(sort_real::abs(sort_int::int_()));
  add_system_function(sort_real::abs(sort_real::real_()));
  //more
  add_system_function(sort_real::negate(sort_pos::pos()));
  add_system_function(sort_real::negate(sort_nat::nat()));
  add_system_function(sort_real::negate(sort_int::int_()));
  add_system_function(sort_real::negate(sort_real::real_()));
  add_system_function(sort_real::succ(sort_pos::pos()));
  add_system_function(sort_real::succ(sort_nat::nat()));
  add_system_function(sort_real::succ(sort_int::int_()));
  add_system_function(sort_real::succ(sort_real::real_()));
  add_system_function(sort_real::pred(sort_pos::pos()));
  add_system_function(sort_real::pred(sort_nat::nat()));
  add_system_function(sort_real::pred(sort_int::int_()));
  add_system_function(sort_real::pred(sort_real::real_()));
  add_system_function(sort_real::plus(sort_pos::pos(), sort_pos::pos()));
  add_system_function(sort_real::plus(sort_pos::pos(), sort_nat::nat()));
  add_system_function(sort_real::plus(sort_nat::nat(), sort_pos::pos()));
  add_system_function(sort_real::plus(sort_nat::nat(), sort_nat::nat()));
  add_system_function(sort_real::plus(sort_int::int_(), sort_int::int_()));
  add_system_function(sort_real::plus(sort_real::real_(), sort_real::real_()));
  //more
  add_system_function(sort_real::minus(sort_pos::pos(), sort_pos::pos()));
  add_system_function(sort_real::minus(sort_nat::nat(), sort_nat::nat()));
  add_system_function(sort_real::minus(sort_int::int_(), sort_int::int_()));
  add_system_function(sort_real::minus(sort_real::real_(), sort_real::real_()));
  add_system_function(sort_real::times(sort_pos::pos(), sort_pos::pos()));
  add_system_function(sort_real::times(sort_nat::nat(), sort_nat::nat()));
  add_system_function(sort_real::times(sort_int::int_(), sort_int::int_()));
  add_system_function(sort_real::times(sort_real::real_(), sort_real::real_()));
  //more
  // add_system_function(sort_int::div(sort_pos::pos(), sort_pos::pos()));
  add_system_function(sort_int::div(sort_nat::nat(), sort_pos::pos()));
  add_system_function(sort_int::div(sort_int::int_(), sort_pos::pos()));
  // add_system_function(sort_int::mod(sort_pos::pos(), sort_pos::pos()));
  add_system_function(sort_int::mod(sort_nat::nat(), sort_pos::pos()));
  add_system_function(sort_int::mod(sort_int::int_(), sort_pos::pos()));
  add_system_function(sort_real::divides(sort_pos::pos(), sort_pos::pos()));
  add_system_function(sort_real::divides(sort_nat::nat(), sort_nat::nat()));
  add_system_function(sort_real::divides(sort_int::int_(), sort_int::int_()));
  add_system_function(sort_real::divides(sort_real::real_(), sort_real::real_()));
  add_system_function(sort_real::exp(sort_pos::pos(), sort_nat::nat()));
  add_system_function(sort_real::exp(sort_nat::nat(), sort_nat::nat()));
  add_system_function(sort_real::exp(sort_int::int_(), sort_nat::nat()));
  add_system_function(sort_real::exp(sort_real::real_(), sort_int::int_()));
  add_system_function(sort_real::floor());
  add_system_function(sort_real::ceil());
  add_system_function(sort_real::round());
  //Lists
  add_system_constant(sort_list::empty(data::untyped_sort()));
  add_system_function(sort_list::cons_(data::untyped_sort()));
  add_system_function(sort_list::count(data::untyped_sort()));
  add_system_function(sort_list::snoc(data::untyped_sort()));
  add_system_function(sort_list::concat(data::untyped_sort()));
  add_system_function(sort_list::element_at(data::untyped_sort()));
  add_system_function(sort_list::head(data::untyped_sort()));
  add_system_function(sort_list::tail(data::untyped_sort()));
  add_system_function(sort_list::rhead(data::untyped_sort()));
  add_system_function(sort_list::rtail(data::untyped_sort()));
  add_system_function(sort_list::in(data::untyped_sort()));

  //Sets

  add_system_function(sort_bag::set2bag(data::untyped_sort()));
  add_system_function(sort_set::in(data::untyped_sort(), data::untyped_sort(), sort_fset::fset(data::untyped_sort())));
  add_system_function(sort_set::in(data::untyped_sort(), data::untyped_sort(), sort_set::set_(data::untyped_sort())));
  add_system_function(sort_set::union_(data::untyped_sort(), sort_fset::fset(data::untyped_sort()), sort_fset::fset(data::untyped_sort())));
  add_system_function(sort_set::union_(data::untyped_sort(), sort_set::set_(data::untyped_sort()), sort_set::set_(data::untyped_sort())));
  add_system_function(sort_set::difference(data::untyped_sort(), sort_fset::fset(data::untyped_sort()), sort_fset::fset(data::untyped_sort())));
  add_system_function(sort_set::difference(data::untyped_sort(), sort_set::set_(data::untyped_sort()), sort_set::set_(data::untyped_sort())));
  add_system_function(sort_set::intersection(data::untyped_sort(), sort_fset::fset(data::untyped_sort()), sort_fset::fset(data::untyped_sort())));
  add_system_function(sort_set::intersection(data::untyped_sort(), sort_set::set_(data::untyped_sort()), sort_set::set_(data::untyped_sort())));
  add_system_function(sort_set::false_function(data::untyped_sort())); // Needed as it is used within the typechecker.
  add_system_function(sort_set::constructor(data::untyped_sort())); // Needed as it is used within the typechecker.
  //**** add_system_function(sort_bag::set2bag(data::untyped_sort()));
  // add_system_constant(sort_set::empty(data::untyped_sort()));
  // add_system_function(sort_set::in(data::untyped_sort()));
  // add_system_function(sort_set::union_(data::untyped_sort()));
  // add_system_function(sort_set::difference(data::untyped_sort()));
  // add_system_function(sort_set::intersection(data::untyped_sort()));
  add_system_function(sort_set::complement(data::untyped_sort()));

  //FSets
  add_system_constant(sort_fset::empty(data::untyped_sort()));
  // add_system_function(sort_fset::in(data::untyped_sort()));
  // add_system_function(sort_fset::union_(data::untyped_sort()));
  // add_system_function(sort_fset::intersection(data::untyped_sort()));
  // add_system_function(sort_fset::difference(data::untyped_sort()));
  add_system_function(sort_fset::count(data::untyped_sort()));
  add_system_function(sort_fset::insert(data::untyped_sort())); // Needed as it is used within the typechecker.

  //Bags
  add_system_function(sort_bag::bag2set(data::untyped_sort()));
  add_system_function(sort_bag::in(data::untyped_sort(), data::untyped_sort(), sort_fbag::fbag(data::untyped_sort())));
  add_system_function(sort_bag::in(data::untyped_sort(), data::untyped_sort(), sort_bag::bag(data::untyped_sort())));
  add_system_function(sort_bag::union_(data::untyped_sort(), sort_fbag::fbag(data::untyped_sort()), sort_fbag::fbag(data::untyped_sort())));
  add_system_function(sort_bag::union_(data::untyped_sort(), sort_bag::bag(data::untyped_sort()), sort_bag::bag(data::untyped_sort())));
  add_system_function(sort_bag::difference(data::untyped_sort(), sort_fbag::fbag(data::untyped_sort()), sort_fbag::fbag(data::untyped_sort())));
  add_system_function(sort_bag::difference(data::untyped_sort(), sort_bag::bag(data::untyped_sort()), sort_bag::bag(data::untyped_sort())));
  add_system_function(sort_bag::intersection(data::untyped_sort(), sort_fbag::fbag(data::untyped_sort()), sort_fbag::fbag(data::untyped_sort())));
  add_system_function(sort_bag::intersection(data::untyped_sort(), sort_bag::bag(data::untyped_sort()), sort_bag::bag(data::untyped_sort())));
  add_system_function(sort_bag::count(data::untyped_sort(), data::untyped_sort(), sort_fbag::fbag(data::untyped_sort())));
  add_system_function(sort_bag::count(data::untyped_sort(), data::untyped_sort(), sort_bag::bag(data::untyped_sort())));
  // add_system_constant(sort_bag::empty(data::untyped_sort()));
  // add_system_function(sort_bag::in(data::untyped_sort()));
  //**** add_system_function(sort_bag::count(data::untyped_sort()));
  // add_system_function(sort_bag::count(data::untyped_sort(), data::untyped_sort(), sort_fset::fset(data::untyped_sort())));
  //add_system_function(sort_bag::join(data::untyped_sort()));
  // add_system_function(sort_bag::difference(data::untyped_sort()));
  // add_system_function(sort_bag::intersection(data::untyped_sort()));
  add_system_function(sort_bag::zero_function(data::untyped_sort())); // Needed as it is used within the typechecker.
  add_system_function(sort_bag::constructor(data::untyped_sort())); // Needed as it is used within the typechecker.

  //FBags
  add_system_constant(sort_fbag::empty(data::untyped_sort()));
  // add_system_function(sort_fbag::count(data::untyped_sort()));
  // add_system_function(sort_fbag::in(data::untyped_sort()));
  // add_system_function(sort_fbag::union_(data::untyped_sort()));
  // add_system_function(sort_fbag::intersection(data::untyped_sort()));
  // add_system_function(sort_fbag::difference(data::untyped_sort()));
  add_system_function(sort_fbag::count_all(data::untyped_sort()));
  add_system_function(sort_fbag::cinsert(data::untyped_sort())); // Needed as it is used within the typechecker.

  // function update
  add_system_function(data::function_update(data::untyped_sort(), data::untyped_sort()));
}

void data_type_checker::add_function(const data::function_symbol& f, const std::string& msg, bool allow_double_decls)
{
  const sort_expression_list domain = function_sort(f.sort()).domain();
  const core::identifier_string& Name = f.name();
  const sort_expression& Sort = f.sort();

  if (system_constants.count(Name) > 0)
  {
    throw mcrl2::runtime_error("Attempt to redeclare the system constant with a " + msg + " " + data::pp(f) + ".");
  }

  if (system_functions.count(Name) > 0)
  {
    throw mcrl2::runtime_error("Attempt to redeclare a system function with a " + msg + " " + data::pp(f) + ".");
  }

  std::map<core::identifier_string, sort_expression_list>::const_iterator j = user_functions.find(Name);

  // the table user_functions contains a list of types for each
  // function name. We need to check if there is already such a type
  // in the list. If so -- error, otherwise -- add
  if (j != user_functions.end())
  {
    sort_expression_list Types = j->second;
    if (find_equal_sort(Sort, Types))
    {
      if (!allow_double_decls)
      {
        throw mcrl2::runtime_error("Double declaration of " + msg + " " + core::pp(Name) + ".");
      }
    }
    Types = Types + sort_expression_list({normalize_sort(Sort)});
    user_functions[Name] = Types;
  }
  else
  {
    user_functions[Name] = sort_expression_list({normalize_sort(Sort)});
  }
}

void data_type_checker::read_sort(const sort_expression& x)
{
  if (is_basic_sort(x))
  {
    check_basic_sort_is_declared(atermpp::down_cast<basic_sort>(x).name());
    return;
  }

  if (is_container_sort(x))
  {
    return read_sort(atermpp::down_cast<container_sort>(x).element_sort());
  }

  if (is_function_sort(x))
  {
    const auto& fs = atermpp::down_cast<function_sort>(x);
    read_sort(fs.codomain());

    for (const auto& i : fs.domain())
    {
      read_sort(i);
    }
    return;
  }

  if (is_structured_sort(x))
  {
    const auto& x_ = atermpp::down_cast<structured_sort>(x);
    for (const auto& constructor : x_.constructors())
    {
      // recognizer -- if present -- a function from SortExpr to Bool
      core::identifier_string name = constructor.recogniser();
      if (name != core::empty_identifier_string())
      {
        add_function(data::function_symbol(name, function_sort(sort_expression_list({x}), sort_bool::bool_())),
                     "recognizer");
      }

      // constructor type and projections
      const structured_sort_constructor_argument_list& arguments = constructor.arguments();
      name = constructor.name();
      if (arguments.empty())
      {
        add_constant(data::function_symbol(name, x), "constructor constant");
        continue;
      }

      sort_expression_list sorts;
      for (const auto& arg: arguments)
      {
        read_sort(arg.sort());
        if (arg.name() != core::empty_identifier_string())
        {
          add_function(function_symbol(arg.name(), function_sort(sort_expression_list({x}), arg.sort())),
                       "projection", true);
        }
        sorts.push_front(arg.sort());
      }
      add_function(data::function_symbol(name, function_sort(atermpp::reverse(sorts), x)), "constructor");
    }
    return;
  }
}

data_type_checker::data_type_checker(const data_specification& dataspec)
        : sort_type_checker(dataspec),
          was_warning_upcasting(false)
{
  initialise_system_defined_functions();

  try
  {
    for (const alias& a: get_sort_specification().user_defined_aliases())
    {
      read_sort(a.reference());
    }
    read_constructors_and_mappings(dataspec.user_defined_constructors(), dataspec.user_defined_mappings(),
                                   dataspec.constructors());
  }
  catch (mcrl2::runtime_error& e)
  {
    throw mcrl2::runtime_error(std::string(e.what()) + "\nType checking of data expression failed.");
  }

  type_checked_data_spec = dataspec;
  type_checked_data_spec.declare_data_specification_to_be_type_checked();

  // Type check equations and add them to the specification.
  try
  {
    typecheck_data_specification(type_checked_data_spec);
  }
  catch (mcrl2::runtime_error& e)
  {
    type_checked_data_spec = data_specification(); // Type checking failed. Data specification is not usable.
    throw mcrl2::runtime_error(std::string(e.what()) + "\nFailed to type check data specification.");
  }
}

data_expression
data_type_checker::operator()(const data_expression& x, const detail::variable_context& context_variables) const
{
  data_expression data = x;
  sort_expression sort;
  try
  {
    data = typecheck(data, data::untyped_sort(), context_variables);
    sort = data.sort();
  }
  catch (mcrl2::runtime_error& e)
  {
    throw mcrl2::runtime_error(std::string(e.what()) + "\nType checking of data expression failed.");
  }
  if (data::is_untyped_sort(sort))
  {
    throw mcrl2::runtime_error("Type checking of data expression " + data::pp(x) + " failed. Result is an unknown sort.");
  }

  assert(strict_type_check(data));
  return data;
}


void data_type_checker::operator()(const variable& v, const detail::variable_context& context_variables) const
{
  // First check whether the variable name clashes with a system or user defined function or constant.
  auto i1 = system_constants.find(v.name());
  if (i1 != system_constants.end())
  {
    throw mcrl2::runtime_error("The variable " + core::pp(v.name()) + ":" + data::pp(v.sort()) +
                               " clashes with the system defined constant " + core::pp(i1->first) + ":" +
                               data::pp(i1->second.front()) + ".");
  }
  auto i2 = system_functions.find(v.name());
  if (i2 != system_functions.end())
  {
    throw mcrl2::runtime_error("The variable " + core::pp(v.name()) + ":" + data::pp(v.sort()) +
                               " clashes with the system defined function " + core::pp(i2->first) + ":" +
                               data::pp(i2->second.front()) + ".");
  }
  auto i3 = user_constants.find(v.name());
  if (i3 != user_constants.end())
  {
    throw mcrl2::runtime_error("The variable " + core::pp(v.name()) + ":" + data::pp(v.sort()) +
                               " clashes with the user defined constant " + core::pp(i3->first) + ":" +
                               data::pp(i3->second) + ".");
  }
  auto i4 = user_functions.find(v.name());
  if (i4 != user_functions.end() && !m_checking_untyped_variable_assignment)
  {
    throw mcrl2::runtime_error("The variable " + core::pp(v.name()) + ":" + data::pp(v.sort()) +
                               " clashes with the user defined function " + core::pp(i4->first) + ":" +
                               data::pp(i4->second.front()) + ".");
  }

  // Second, check that the variable has a valid sort.
  try
  {
    sort_type_checker::check_sort_is_declared(v.sort());
  }
  catch (mcrl2::runtime_error& e)
  {
    throw mcrl2::runtime_error(
            std::string(e.what()) + "\nType error while typechecking the data variable " + core::pp(v.name()) + ":" +
            data::pp(v.sort()) + ".");
  }

  // Third, check that the variable did not occur in the context with a different type.
  const std::map<core::identifier_string, sort_expression>::const_iterator i = context_variables.context().find(
          v.name());
  sort_expression temp;
  if (i != context_variables.context().end() && !match_sorts(i->second, v.sort(), temp))
  {
    throw mcrl2::runtime_error("The variable " + core::pp(v.name()) + ":" + data::pp(v.sort()) +
                               " is used in its surrounding context with a different sort " + core::pp(i->second) +
                               ".");
  }
}


void data_type_checker::operator()(const variable_list& variables,
                                   const detail::variable_context& context_variables) const
{
  // Type check each variable.
  for (const variable& v: variables)
  {
    (*this)(v, context_variables);
  }

  // Check that all variable names are unique.
  std::set<core::identifier_string> variable_names;
  for (const variable& v: variables)
  {
    if (!variable_names.insert(v.name()).second) // The variable name is already in the set.
    {
      throw mcrl2::runtime_error("The variable " + data::pp(v) + " occurs multiple times.");
    }
  }
}

// ------------------------------  Here ends the new class based data expression checker -----------------------
// ------------------------------  Here starts the new class based data specification checker -----------------------

// Type check and replace user defined equations.
void data_type_checker::typecheck_data_specification(data_specification& dataspec)
{
  //Create a new specification; admittedly, this is somewhat clumsy.
  data_specification new_specification;
  for (const basic_sort& s: dataspec.user_defined_sorts())
  {
    new_specification.add_sort(s);
  }
  for (const alias& a: dataspec.user_defined_aliases())
  {
    new_specification.add_alias(a);
  }
  for (const function_symbol& f: dataspec.user_defined_constructors())
  {
    new_specification.add_constructor(f);
  }
  for (const function_symbol& f: dataspec.user_defined_mappings())
  {
    new_specification.add_mapping(f);
  }

  for (const data_equation& eqn: dataspec.user_defined_equations())
  {
    const variable_list& variables = eqn.variables();
    data_expression condition = eqn.condition();
    data_expression lhs = eqn.lhs();
    data_expression rhs = eqn.rhs();

    try
    {
      // Typecheck the variables in an equation.
      (*this)(variables, detail::variable_context());
    }
    catch (mcrl2::runtime_error& e)
    {
      throw mcrl2::runtime_error(std::string(e.what()) + "\nThis error occurred while typechecking equation " + data::pp(eqn) + ".");
    }

    detail::variable_context declared_variables;
    declared_variables.add_context_variables(variables);

    sort_expression lhs_sort;
    try
    {
      lhs = typecheck(lhs, data::untyped_sort(), declared_variables, false, true);
      lhs_sort = lhs.sort();
    }
    catch (mcrl2::runtime_error& e)
    {
      throw mcrl2::runtime_error(std::string(e.what()) + "\nError occurred while typechecking " + data::pp(lhs) + " as left hand side of equation " + data::pp(eqn) + ".");
    }

    if (was_warning_upcasting)
    {
      was_warning_upcasting = false;
      mCRL2log(warning) << "Warning occurred while typechecking " << lhs << " as left hand side of equation " << eqn << "." << std::endl;
    }

    condition = typecheck(condition, sort_bool::bool_(), declared_variables);

    sort_expression rhs_sort;
    try
    {
      rhs = typecheck(rhs, lhs_sort, declared_variables, false);
      rhs_sort = rhs.sort();
    }
    catch (mcrl2::runtime_error& e)
    {
      throw mcrl2::runtime_error(std::string(e.what()) + "\nError occurred while typechecking " + data::pp(rhs) + " as right hand side of equation " + data::pp(eqn) + ".");
    }

    //If the types are not uniquely the same now: do once more:
    if (!equal_sorts(lhs_sort, rhs_sort))
    {
      sort_expression sort;
      if (!match_sorts(lhs_sort, rhs_sort, sort))
      {
        throw mcrl2::runtime_error("Types of the left- (" + data::pp(lhs_sort) + ") and right- (" + data::pp(rhs_sort) + ") hand-sides of the equation " + data::pp(eqn) + " do not match.");
      }
      lhs = eqn.lhs();
      try
      {
        lhs = typecheck(lhs, sort, declared_variables, true);
        lhs_sort = lhs.sort();
      }
      catch (mcrl2::runtime_error& e)
      {
        throw mcrl2::runtime_error(std::string(e.what()) + "\nTypes of the left- and right-hand-sides of the equation " + data::pp(eqn) + " do not match.");
      }
      if (was_warning_upcasting)
      {
        was_warning_upcasting = false;
        mCRL2log(warning) << "Warning occurred while typechecking " << lhs << " as left hand side of equation " << eqn << "." << std::endl;
      }
      rhs = eqn.rhs();
      try
      {
        rhs = typecheck(rhs, lhs_sort, declared_variables);
        rhs_sort = rhs.sort();
      }
      catch (mcrl2::runtime_error& e)
      {
        throw mcrl2::runtime_error(std::string(e.what()) + "\nTypes of the left- and right-hand-sides of the equation " + data::pp(eqn) + " do not match.");
      }
      if (!match_sorts(lhs_sort, rhs_sort, sort))
      {
        throw mcrl2::runtime_error("Types of the left- (" + data::pp(lhs_sort) + ") and right- (" + data::pp(rhs_sort) + ") hand-sides of the equation " + data::pp(eqn) + " do not match.");
      }
      if (detail::has_unknown(sort))
      {
        throw mcrl2::runtime_error("Types of the left- (" + data::pp(lhs_sort) + ") and right- (" + data::pp(rhs_sort) + ") hand-sides of the equation " + data::pp(eqn) + " cannot be uniquely determined.");
      }
      // Check that the variable in the condition and the right hand side are a subset of those in the left hand side of the equation.
      const std::set<variable> vars_in_lhs = find_free_variables(lhs);
      const std::set<variable> vars_in_rhs = find_free_variables(rhs);

      variable culprit;
      if (!detail::includes(vars_in_rhs, vars_in_lhs, culprit))
      {
        throw mcrl2::runtime_error("The variable " + data::pp(culprit) + " in the right hand side is not included in the left hand side of the equation " + data::pp(eqn) + ".");
      }

      const std::set<variable> vars_in_condition = find_free_variables(condition);
      if (!detail::includes(vars_in_condition, vars_in_lhs, culprit))
      {
        throw mcrl2::runtime_error("The variable " + data::pp(culprit) + " in the condition is not included in the left hand side of the equation " + data::pp(eqn) + ".");
      }
    }
    new_specification.add_equation(data_equation(variables, condition, lhs, rhs));
  }
  dataspec = new_specification;
}

const data_specification data_type_checker::operator()() const
{
  return type_checked_data_spec;
}

} // namespace data

} // namespace mcrl2
