// Author(s): Wieger Wesselink
// Copyright: see the accompanying file COPYING or copy at
// https://svn.win.tue.nl/trac/MCRL2/browser/trunk/COPYING
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
/// \file mcrl2/data/variable_assignment.h
/// \brief The class variable_assignment.

#ifndef MCRL2_DATA_VARIABLE_ASSIGNMENT_H
#define MCRL2_DATA_VARIABLE_ASSIGNMENT_H

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

//--- start generated class variable_assignment ---//
/// \brief A variable assignment
class variable_assignment: public data_expression
{
  public:
    /// \brief Default constructor.
    variable_assignment()
      : data_expression(core::detail::default_values::VariableAssignment)
    {}

    /// \brief Constructor.
    /// \param term A term
    explicit variable_assignment(const atermpp::aterm& term)
      : data_expression(term)
    {
      assert(core::detail::check_term_VariableAssignment(*this));
    }

    /// \brief Constructor.
    variable_assignment(const core::identifier_string& name, const sort_expression& sort, const assignment_list& assignments)
      : data_expression(atermpp::aterm_appl(core::detail::function_symbol_VariableAssignment(), name, sort, assignments))
    {}

    /// \brief Constructor.
    template <typename Container>
    variable_assignment(const std::string& name, const sort_expression& sort, const Container& assignments, typename atermpp::enable_if_container<Container, assignment>::type* = nullptr)
      : data_expression(atermpp::aterm_appl(core::detail::function_symbol_VariableAssignment(), core::identifier_string(name), sort, assignment_list(assignments.begin(), assignments.end())))
    {}

    /// Move semantics
    variable_assignment(const variable_assignment&) noexcept = default;
    variable_assignment(variable_assignment&&) noexcept = default;
    variable_assignment& operator=(const variable_assignment&) noexcept = default;
    variable_assignment& operator=(variable_assignment&&) noexcept = default;

    const core::identifier_string& name() const
    {
      return atermpp::down_cast<core::identifier_string>((*this)[0]);
    }

    const sort_expression& sort() const
    {
      return atermpp::down_cast<sort_expression>((*this)[1]);
    }

    const assignment_list& assignments() const
    {
      return atermpp::down_cast<assignment_list>((*this)[2]);
    }
};

// prototype declaration
std::string pp(const variable_assignment& x);

/// \brief Outputs the object to a stream
/// \param out An output stream
/// \param x Object x
/// \return The output stream
inline
std::ostream& operator<<(std::ostream& out, const variable_assignment& x)
{
  return out << data::pp(x);
}

/// \brief swap overload
inline void swap(variable_assignment& t1, variable_assignment& t2)
{
  t1.swap(t2);
}
//--- end generated class variable_assignment ---//

} // namespace data

} // namespace mcrl2

#endif // MCRL2_DATA_VARIABLE_ASSIGNMENT_H

