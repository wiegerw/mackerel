// Author(s): Wieger Wesselink
// Copyright: see the accompanying file COPYING or copy at
// https://svn.win.tue.nl/trac/MCRL2/browser/trunk/COPYING
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
/// \file mcrl2/core/print.h
/// \brief Functions for pretty printing ATerms.

#ifndef MCRL2_CORE_PRINT_H
#define MCRL2_CORE_PRINT_H

#include "mcrl2/atermpp/aterm_appl.h"
#include "mcrl2/core/detail/precedence.h"
#include "mcrl2/core/print_format.h"
#include "mcrl2/core/traverser.h"
#include "mcrl2/utilities/exception.h"
#include "mcrl2/utilities/logger.h"
#include <cassert>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <list>
#include <ostream>
#include <sstream>
#include <string>
#include <vector>

namespace mcrl2
{
namespace core
{

using namespace core::detail::precedences;

/// \cond INTERNAL_DOCS
namespace detail
{

template <typename Derived>
struct printer: public core::traverser<Derived>
{
  typedef core::traverser<Derived> super;

  using super::enter;
  using super::leave;
  using super::apply;

  std::ostream* m_out;

  Derived& derived()
  {
    return static_cast<Derived&>(*this);
  }

  std::ostream& out()
  {
    return *m_out;
  }

  void print(const std::string& s)
  {
    out() << s;
  }

  template <typename T>
  void print_expression(const T& x, int context_precedence, int x_precedence)
  {
    bool print_parens = (x_precedence < context_precedence);
    if (print_parens)
    {
      derived().print("(");
    }
    derived().apply(x);
    if (print_parens)
    {
      derived().print(")");
    }
  }

  template <typename T>
  void print_expression(const T& x, int context_precedence = 5)
  {
    print_expression(x, context_precedence, left_precedence(x));
  }

  template <typename T>
  void print_unary_operation(const T& x, const std::string& op)
  {
    derived().print(op);
    print_expression(unary_operand(x), left_precedence(x));
  }

  template <typename T>
  void print_binary_operation(const T& x, const std::string& op)
  {
    print_expression(binary_left(x), is_same_different_precedence(x, binary_left(x)) ? left_precedence(x) + 1 : left_precedence(x));
    derived().print(op);
    print_expression(binary_right(x), is_same_different_precedence(x, binary_right(x)) ? left_precedence(x) + 1 : left_precedence(x), right_precedence(binary_right(x)));
  }

  template <typename Container>
  void print_list(const Container& container,
                  const std::string& opener = "(",
                  const std::string& closer = ")",
                  const std::string& separator = ", ",
                  bool print_empty_container = false
                 )
  {
    if (container.empty() && !print_empty_container)
    {
      return;
    }
    derived().print(opener);
    for (typename Container::const_iterator i = container.begin(); i != container.end(); ++i)
    {
      if (i != container.begin())
      {
        derived().print(separator);
      }
      derived().apply(*i);
    }
    derived().print(closer);
  }

  template <typename T>
  void apply(const atermpp::term_appl<T>& x)
  {
    derived().enter(x);
    derived().print(utilities::to_string(x));
    derived().leave(x);
  }

  template <typename T>
  void apply(const std::list<T>& x)
  {
    derived().enter(x);
    print_list(x, "", "", ", ");
    derived().leave(x);
  }

  template <typename T>
  void apply(const atermpp::term_list<T>& x)
  {
    derived().enter(x);
    print_list(x, "", "", ", ");
    derived().leave(x);
  }

  template <typename T>
  void apply(const std::set<T>& x)
  {
    derived().enter(x);
    print_list(x, "", "", ", ");
    derived().leave(x);
  }

  void apply(const core::identifier_string& x)
  {
    derived().enter(x);
    if (x == core::identifier_string())
    {
      derived().print("@NoValue");
    }
    else
    {
      derived().print(std::string(x));
    }
    derived().leave(x);
  }

  void apply(const atermpp::aterm& x)
  {
    derived().enter(x);
    derived().print(utilities::to_string(x));
    derived().leave(x);
  }

  void apply(const atermpp::aterm_list& x)
  {
    derived().enter(x);
    derived().print(utilities::to_string(x));
    derived().leave(x);
  }

  void apply(const atermpp::aterm_appl& x)
  {
    derived().enter(x);
    derived().print(utilities::to_string(x));
    derived().leave(x);
  }

  void apply(const atermpp::aterm_int& x)
  {
    derived().enter(x);
    derived().print(utilities::to_string(x));
    derived().leave(x);
  }
};

template <template <class> class Traverser>
struct apply_printer: public Traverser<apply_printer<Traverser> >
{
  typedef Traverser<apply_printer<Traverser> > super;

  using super::enter;
  using super::leave;
  using super::apply;

  apply_printer(std::ostream& out)
  {
    typedef printer<apply_printer<Traverser> > Super;
    static_cast<Super&>(*this).m_out = &out;
  }

};

} // namespace detail
/// \endcond

/// \brief Prints the object x to a stream.
struct stream_printer
{
  template <typename T>
  void operator()(const T& x, std::ostream& out)
  {
    core::detail::apply_printer<core::detail::printer> printer(out);
    printer.apply(x);
  }
};

/// \brief Returns a string representation of the object x.
template <typename T>
std::string pp(const T& x)
{
  std::ostringstream out;
  stream_printer()(x, out);
  return out.str();
}

} // namespace core

} // namespace mcrl2

#endif // MCRL2_CORE_PRINT_H
