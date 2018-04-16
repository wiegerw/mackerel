// Author(s): Wieger Wesselink
// Copyright: see the accompanying file COPYING or copy at
// https://svn.win.tue.nl/trac/MCRL2/browser/trunk/COPYING
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
/// \file mcrl2/data/untyped_variable_assignment.h
/// \brief The class untyped_variable_assignment.

#ifndef MCRL2_DATA_UNTYPED_VARIABLE_ASSIGNMENT_H
#define MCRL2_DATA_UNTYPED_VARIABLE_ASSIGNMENT_H

#include "mcrl2/atermpp/aterm_appl.h"
#include "mcrl2/atermpp/aterm_list.h"
#include "mcrl2/core/detail/default_values.h"
#include "mcrl2/core/detail/soundness_checks.h"
#include "mcrl2/core/index_traits.h"
#include "mcrl2/data/assignment.h"

namespace mcrl2
{

namespace data
{

//--- start generated class untyped_variable_assignment ---//
/// \brief An untyped variable assignment
class untyped_variable_assignment: public data_expression
{
  public:
    /// \brief Default constructor.
    untyped_variable_assignment()
      : data_expression(core::detail::default_values::UntypedVariableAssignment)
    {}

    /// \brief Constructor.
    /// \param term A term
    explicit untyped_variable_assignment(const atermpp::aterm& term)
      : data_expression(term)
    {
      assert(core::detail::check_term_UntypedVariableAssignment(*this));
    }

    /// \brief Constructor.
    untyped_variable_assignment(const core::identifier_string& name, const untyped_identifier_assignment_list& assignments)
      : data_expression(atermpp::aterm_appl(core::detail::function_symbol_UntypedVariableAssignment(), name, assignments))
    {}

    /// \brief Constructor.
    template <typename Container>
    untyped_variable_assignment(const std::string& name, const Container& assignments, typename atermpp::enable_if_container<Container, untyped_identifier_assignment>::type* = nullptr)
      : data_expression(atermpp::aterm_appl(core::detail::function_symbol_UntypedVariableAssignment(), core::identifier_string(name), untyped_identifier_assignment_list(assignments.begin(), assignments.end())))
    {}

    /// Move semantics
    untyped_variable_assignment(const untyped_variable_assignment&) noexcept = default;
    untyped_variable_assignment(untyped_variable_assignment&&) noexcept = default;
    untyped_variable_assignment& operator=(const untyped_variable_assignment&) noexcept = default;
    untyped_variable_assignment& operator=(untyped_variable_assignment&&) noexcept = default;

    const core::identifier_string& name() const
    {
      return atermpp::down_cast<core::identifier_string>((*this)[0]);
    }

    const untyped_identifier_assignment_list& assignments() const
    {
      return atermpp::down_cast<untyped_identifier_assignment_list>((*this)[1]);
    }
};

// prototype declaration
std::string pp(const untyped_variable_assignment& x);

/// \brief Outputs the object to a stream
/// \param out An output stream
/// \param x Object x
/// \return The output stream
inline
std::ostream& operator<<(std::ostream& out, const untyped_variable_assignment& x)
{
  return out << data::pp(x);
}

/// \brief swap overload
inline void swap(untyped_variable_assignment& t1, untyped_variable_assignment& t2)
{
  t1.swap(t2);
}
//--- end generated class untyped_variable_assignment ---//

// template function overloads
std::string pp(const data::untyped_variable_assignment& x);

} // namespace data

} // namespace mcrl2

#endif // MCRL2_DATA_UNTYPED_VARIABLE_ASSIGNMENT_H

