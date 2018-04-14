// Author(s): Jan Friso Groote, Wieger Wesselink
// Copyright: see the accompanying file COPYING or copy at
// https://svn.win.tue.nl/trac/MCRL2/browser/trunk/COPYING
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
/// \file mcrl2/data/typecheck.h
/// \brief add your file description here.

#ifndef MCRL2_DATA_TYPECHECK_H
#define MCRL2_DATA_TYPECHECK_H

#include "mcrl2/data/data_specification.h"
#include "mcrl2/data/detail/variable_context.h"
#include "mcrl2/data/sort_type_checker.h"

namespace mcrl2 {

namespace data {

class data_type_checker : public sort_type_checker
{
  protected:
    mutable bool was_warning_upcasting; // This variable is used to limit the number of upcasting warnings.

    std::map<core::identifier_string, sort_expression_list> system_constants;   //name -> Set(sort expression)
    std::map<core::identifier_string, sort_expression_list> system_functions;   //name -> Set(sort expression)
    std::map<core::identifier_string, sort_expression> user_constants;          //name -> sort expression
    std::map<core::identifier_string, sort_expression_list> user_functions;     //name -> Set(sort expression)
    data_specification type_checked_data_spec;

  public:
    /** \brief     make a data type checker.
     *             Throws a mcrl2::runtime_error exception if the data_specification is not well typed.
     *  \param[in] dataspec A data specification that does not need to have been type checked.
     *  \return    A data expression where all untyped identifiers have been replace by typed ones.
     **/
    explicit data_type_checker(const data_specification& dataspec);

    /** \brief     Type checks a variable.
     *             Throws an mcrl2::runtime_error exception if the variable is not well typed.
     *  \details   A variable is not well typed if its name clashes with the name of a declared function, when its sort does not exist, or when
     *             the variable is used in its context with a different sort.
     *  \param[in] v A variables that is to be type checked.
     *  \param[in] context Information about the context of the variable.
     **/
    void operator()(const variable& v, const detail::variable_context& context) const;

    /** \brief     Type checks a variable list.
     *             Throws an mcrl2::runtime_error exception if the variables are not well typed.
     *  \details   A variable is not well typed if its name clashes with the name of a declared function, when its sort does not exist, or when
     *             a variable occurs in the context. Furthermore, variables cannot occur multiple times in a variable list.
     *  \param[in] variables A list of variables that must be type checked.
     *  \param[in] context Information about the context of the variables in the list.
     **/
    void operator()(const variable_list& variables, const detail::variable_context& context) const;

    /** \brief     Yields a type checked data specification, provided typechecking was successful.
     *             If not successful an exception is thrown.
     *  \return    a data specification where all untyped identifiers have been replace by typed ones.
     **/
    const data_specification operator()() const;

  protected:
    /** \brief     Type check a data expression.
     *  Throws a mcrl2::runtime_error exception if the expression is not well typed.
     *  \param[in] x A data expression that has not been type checked.
     *  \param[in] context The variable context in which this term must be typechecked.
     *  \return    a data expression where all untyped identifiers have been replace by typed ones.
     **/
    data_expression operator()(const data_expression& x,
                               const detail::variable_context& context) const;

    void read_sort(const sort_expression& x);

    void
    read_constructors_and_mappings(const function_symbol_vector& constructors, const function_symbol_vector& mappings,
                                   const function_symbol_vector& normalized_constructors);

    void add_function(const data::function_symbol& f, const std::string& msg, bool allow_double_decls = false);

    void add_constant(const data::function_symbol& f, const std::string& msg);

    void initialise_system_defined_functions();

    void add_system_constant(const data::function_symbol& x);

    void add_system_function(const data::function_symbol& x);

    // Checks if Type and PosType match by instantiating unknown sorts.
    // It returns the matching instantiation of Type in result. If matching fails,
    // it returns false, otherwise true.
    bool match_sorts(const sort_expression& x1, const sort_expression& x2, sort_expression& result) const;

    bool match_sort_lists(const sort_expression_list& x1, const sort_expression_list& x2,
                          sort_expression_list& result) const;

    sort_expression normalize_sort(const sort_expression& x) const
    {
      return data::normalize_sorts(x, get_sort_specification());
    }

    data_expression typecheck_abstraction(const data_expression& x, const sort_expression& expected_sort,
                              const detail::variable_context& declared_variables, bool strictly_ambiguous = true,
                              bool warn_upcasting = false, bool print_cast_error = true) const;

    data_expression typecheck_where_clause(const data_expression& x, const sort_expression& expected_sort,
                                          const detail::variable_context& declared_variables, bool strictly_ambiguous = true,
                                          bool warn_upcasting = false, bool print_cast_error = true) const;

    data_expression typecheck_application(const data_expression& x, const sort_expression& expected_sort,
                                          const detail::variable_context& declared_variables, bool strictly_ambiguous = true,
                                          bool warn_upcasting = false, bool print_cast_error = true) const;

    data_expression typecheck_identifier_function_symbol_variable(const data_expression& x, const sort_expression& expected_sort,
                                          const detail::variable_context& declared_variables, bool strictly_ambiguous = true,
                                          bool warn_upcasting = false, bool print_cast_error = true) const;

    //Type checks and transforms x replacing Unknown datatype with other ones.
    //Returns the type of the term which should match expected_sort.
    data_expression typecheck(const data_expression& x, const sort_expression& expected_sort,
                              const detail::variable_context& declared_variables, bool strictly_ambiguous = true,
                              bool warn_upcasting = false, bool print_cast_error = true) const;

    data_expression typecheck_n(const data_expression& x, const sort_expression& expected_sort,
                                const detail::variable_context& declared_variables, bool strictly_ambiguous,
                                size_t parameter_count, bool warn_upcasting, bool print_cast_error) const;

    bool equal_sorts(const sort_expression& x1, const sort_expression& x2) const
    {
      return x1 == x2 || normalize_sort(x1) == normalize_sort(x2);
    }

    bool find_equal_sort(const sort_expression& x, const sort_expression_list& sorts) const
    {
      for (const sort_expression& s: sorts)
      {
        if (equal_sorts(x, s))
        {
          return true;
        }
      }
      return false;
    }

    // if x1 is convertible into x2 or vice versa, the most general
    // of these types are returned in result. If no conversion is possible false is returned
    // and result is not changed. Conversions only take place between numerical types
    bool maximum_type(const sort_expression& x1, const sort_expression& x2, sort_expression& result) const;

    //Expand sort x to possible bigger types.
    sort_expression expand_numeric_types_up(const sort_expression& x) const;

    sort_expression_list expand_numeric_types_up(const sort_expression_list& x) const;

    // Expand Numeric types down
    sort_expression expand_numeric_types_down(sort_expression Type) const;

    //Find the minimal type that Unifies the 2. If not possible, return false.
    bool unify_minimum_type(const sort_expression& x1, const sort_expression& x2, sort_expression& result) const;

    sort_expression determine_allowed_type(const data_expression& x, const sort_expression& expected_sort) const;

    //tries to sort out the types for if.
    //If some of the parameters are Pos,Nat, or Int do upcasting
    bool match_if(const function_sort& type, sort_expression& result) const;

    //tries to sort out the types for ==, !=, <, <=, >= and >.
    //If some of the parameters are Pos,Nat, or Int do upcasting.
    bool match_relational_operators(const function_sort& type, sort_expression& result) const;

    //tries to sort out the types for sqrt. There is only one option: sqrt:Nat->Nat.
    bool match_sqrt(const function_sort& type, sort_expression& result) const;

    //tries to sort out the types of Cons operations (SxList(S)->List(S))
    //If some of the parameters are Pos,Nat, or Int do upcasting.
    bool match_cons(const function_sort& type, sort_expression& result) const;

    //tries to sort out the types of Cons operations (SxList(S)->List(S))
    //If some of the parameters are Pos,Nat, or Int do upcasting.
    bool match_snoc(const function_sort& type, sort_expression& result) const;

    //tries to sort out the types of Cons operations (SxList(S)->List(S))
    //If some of the parameters are Pos,Nat, or Int do upcasting.
    bool match_concat(const function_sort& type, sort_expression& result) const;

    //tries to sort out the types of EltAt operations (List(S)xNat->S)
    //If some of the parameters are Pos,Nat, or Int do upcasting.
    bool match_element_at(const function_sort& type, sort_expression& result) const;

    //tries to sort out the types of Cons operations (SxList(S)->List(S))
    //If some of the parameters are Pos,Nat, or Int do upcasting.
    bool match_head(const function_sort& type, sort_expression& result) const;

    //tries to sort out the types of Cons operations (SxList(S)->List(S))
    //If some of the parameters are Pos,Nat, or Int do upcasting.
    bool match_tail(const function_sort& type, sort_expression& result) const;

    //tries to sort out the types of Set2Bag (Set(S)->Bag(s))
    //If some of the parameters are Pos,Nat, or Int do upcasting.
    bool match_set2bag(const function_sort& type, sort_expression& result) const;

    //tries to sort out the types of @false (S->Bool)
    //If some of the parameters are Pos,Nat, or Int do upcasting.
    bool match_false(const function_sort& type, sort_expression& result) const;

    //tries to sort out the type of EltIn (SxList(S)->Bool or SxSet(S)->Bool or SxBag(S)->Bool)
    //If some of the parameters are Pos,Nat, or Int do upcasting.
    bool match_in(const function_sort& type, sort_expression& result) const;

    bool match_fset_insert(const function_sort& type, sort_expression& result) const;

    bool match_fbag_cinsert(const function_sort& type, sort_expression& result) const;

    //tries to sort out the types of Set or Bag Union, Diff or Intersect
    //operations (Set(S)xSet(S)->Set(S)). It can also be that this operation is
    //performed on numbers. In this case we do nothing.
    //If some of the parameters are Pos,Nat, or Int do upcasting.
    bool match_set_bag_operations(const function_sort& x, sort_expression& result) const;

    //tries to sort out the types of SetCompl operation (Set(S)->Set(S))
    //If some of the parameters are Pos,Nat, or Int do upcasting.
    bool match_set_complement(const function_sort& type, sort_expression& result) const;

    //tries to sort out the types of Bag2Set (Bag(S)->Set(S))
    //If some of the parameters are Pos,Nat, or Int do upcasting.
    bool match_bag2set(const function_sort& type, sort_expression& result) const;

    //tries to sort out the types of BagCount (SxBag(S)->Nat)
    //If some of the parameters are Pos,Nat, or Int do upcasting.
    //If the second argument is not a Bag, don't match
    bool match_bag_count(const function_sort& type, sort_expression& result) const;

    //tries to sort out the types of FuncUpdate ((A->B)xAxB->(A->B))
    //If some of the parameters are Pos,Nat, or Int do upcasting.
    bool match_function_update(const function_sort& type, sort_expression& result) const;

    //tries to sort out the types of @set (Set(S)->Bool)->FSet(s))->Set(S)
    //If some of the parameters are Pos,Nat, or Int do upcasting.
    bool match_set_constructor(const function_sort& type, sort_expression& result) const;

    //tries to sort out the types of @bag (Bag(S)->Bool)->FBag(s))->Bag(S)
    //If some of the parameters are Pos,Nat, or Int do upcasting.
    bool match_bag_constructor(const function_sort& type, sort_expression& result) const;

    //Filter PosType to contain only functions ArgTypes -> TypeX
    //result is TypeX if unique, the set of TypeX as NotInferred if many.
    //return true if successful, otherwise false.
    bool UnArrowProd(const sort_expression_list& ArgTypes, sort_expression PosType, sort_expression& result) const;

    //select Set(Type), elements, return their list of arguments.
    bool UnFSet(const sort_expression& x, sort_expression& result) const;

    //select Bag(Type), elements, return their list of arguments.
    bool UnFBag(const sort_expression& x, sort_expression& result) const;

    //select List(Type), elements, return their list of arguments.
    bool UnList(const sort_expression& x, sort_expression& result) const;

    // Makes upcasting from sort to expected_sort for x. Returns the resulting type.
    // Moreover, *Par is extended with the required type transformations.
    data_expression upcast_numeric_type(const data_expression& x, sort_expression sort, sort_expression expected_sort,
                                        const detail::variable_context& declared_variables, bool strictly_ambiguous,
                                        bool warn_upcasting, bool print_cast_error) const;

    void typecheck_data_specification(data_specification& dataspec);

    bool strict_type_check(const data_expression& x) const;

    // for example Pos -> Nat, or Nat -> Int
    data_expression upcast_numeric_type(const data_expression& x,
                                        const sort_expression& expected_sort,
                                        const detail::variable_context& variable_context
    )
    {
      try
      {
        data_expression x1 = x;
        x1 = upcast_numeric_type(x1, x1.sort(), expected_sort, variable_context, false, false, false);
        return data::normalize_sorts(x1, get_sort_specification());
      }
      catch (mcrl2::runtime_error& e)
      {
        throw mcrl2::runtime_error(
          std::string(e.what()) + "\ncannot (up)cast " + data::pp(x) + " to type " + data::pp(expected_sort));
      }
    }

  public:
    data_expression typecheck_data_expression(const data_expression& x,
                                              const sort_expression& expected_sort,
                                              const detail::variable_context& variable_context
    )
    {
      data_expression x1 = x;
      x1 = typecheck(x1, expected_sort, variable_context);
      x1 = data::normalize_sorts(x1, get_sort_specification());
      if (x1.sort() != expected_sort)
      {
        x1 = upcast_numeric_type(x1, expected_sort, variable_context);
      }
      return x1;
    }

    assignment typecheck_assignment(const assignment& x, const detail::variable_context& variable_context)
    {
      sort_type_checker::check_sort_is_declared(x.lhs().sort());
      data_expression rhs = typecheck_data_expression(x.rhs(), x.lhs().sort(), variable_context);
      return assignment(x.lhs(), rhs);
    }

    assignment_list
    typecheck_assignment_list(const assignment_list& assignments, const detail::variable_context& variable_context)
    {
      // check for name clashes
      std::set<core::identifier_string> names;
      for (const assignment& a: assignments)
      {
        const core::identifier_string& name = a.lhs().name();
        if (names.find(name) != names.end())
        {
          throw mcrl2::runtime_error("duplicate variable names in assignments: " + data::pp(assignments) + ")");
        }
        names.insert(name);
      }

      // typecheck the assignments
      assignment_vector result;
      for (const assignment& a: assignments)
      {
        result.push_back(typecheck_assignment(a, variable_context));
      }
      return assignment_list(result.begin(), result.end());
    }

    const data_specification& typechecked_data_specification() const
    {
      return type_checked_data_spec;
    }

    void print_context() const
    {
      auto const& sortspec = get_sort_specification();
      std::cout << "--- basic sorts ---" << std::endl;
      for (auto const& x: sortspec.user_defined_sorts())
      {
        std::cout << x << std::endl;
      }
      std::cout << "--- aliases ---" << std::endl;
      for (auto const& x: sortspec.user_defined_aliases())
      {
        std::cout << x << std::endl;
      }
      std::cout << "--- user constants ---" << std::endl;
      for (const auto& user_constant: user_constants)
      {
        std::cout << user_constant.first << " -> " << user_constant.second << std::endl;
      }
      std::cout << "--- user functions ---" << std::endl;
      for (const auto& user_function: user_functions)
      {
        std::cout << user_function.first << " -> " << user_function.second << std::endl;
      }
    }
};

/** \brief     Type check a sort expression.
 *  Throws an exception if something went wrong.
 *  \param[in] sort_expr A sort expression that has not been type checked.
 *  \param[in] data_spec The data specification to use as context.
 *  \post      sort_expr is type checked.
 **/
inline
void typecheck_sort_expression(const sort_expression& sort_expr, const data_specification& data_spec)
{
  try
  {
    // sort_type_checker type_checker(data_spec.user_defined_sorts(), data_spec.user_defined_aliases());
    sort_type_checker type_checker(data_spec);
    type_checker(sort_expr);
  }
  catch (mcrl2::runtime_error& e)
  {
    throw mcrl2::runtime_error(std::string(e.what()) + "\nCould not type check sort " + pp(sort_expr));
  }
}

/** \brief     Type check a data expression.
 *  Throws an exception if something went wrong.
 *  \param[in] x A data expression that has not been type checked.
 *  \param[in] variables A container with variables that can occur in the data expression.
 *  \param[in] dataspec The data specification that is used for type checking.
 *  \post      data_expr is type checked.
 **/
template<typename VariableContainer>
data_expression typecheck_data_expression(const data_expression& x,
                                          const VariableContainer& variables,
                                          const data_specification& dataspec = data_specification()
)
{
  try
  {
    data_type_checker typechecker(dataspec);
    detail::variable_context variable_context;
    variable_context.add_context_variables(variables, typechecker);
    return typechecker.typecheck_data_expression(x, untyped_sort(), variable_context);
  }
  catch (mcrl2::runtime_error& e)
  {
    throw mcrl2::runtime_error(std::string(e.what()) + "\nCould not type check data expression " + data::pp(x));
  }
}

/** \brief     Type check a data expression.
 *  Throws an exception if something went wrong.
 *  \param[in] x A data expression that has not been type checked.
 *  \param[in] dataspec Data specification to be used as context.
 *  \post      data_expr is type checked.
 **/
inline
data_expression
typecheck_data_expression(const data_expression& x, const data_specification& dataspec = data_specification())
{
  return typecheck_data_expression(x, variable_list(), dataspec);
}

/** \brief     Type check a parsed mCRL2 data specification.
 *  Throws an exception if something went wrong.
 *  \param[in] data_spec A data specification that has not been type checked.
 *  \post      data_spec is type checked.
 **/
inline
void typecheck_data_specification(data_specification& data_spec)
{
  data_type_checker type_checker(data_spec);
  data_spec = type_checker();
}

inline
data_expression typecheck_untyped_data_parameter(data_type_checker& typechecker,
                                                 const core::identifier_string& name,
                                                 const data_expression_list& parameters,
                                                 const data::sort_expression& expected_sort,
                                                 const detail::variable_context& variable_context
)
{
  if (parameters.empty())
  {
    return typechecker.typecheck_data_expression(untyped_identifier(name), expected_sort, variable_context);
  }
  else
  {
    return typechecker.typecheck_data_expression(application(untyped_identifier(name), parameters), expected_sort,
                                                 variable_context);
  }
}

typedef atermpp::term_list<sort_expression_list> sorts_list;

} // namespace data

} // namespace mcrl2

#endif // MCRL2_DATA_TYPECHECK_H
