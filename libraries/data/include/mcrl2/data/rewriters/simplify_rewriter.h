// Author(s): Wieger Wesselink
// Copyright: see the accompanying file COPYING or copy at
// https://svn.win.tue.nl/trac/MCRL2/browser/trunk/COPYING
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
/// \file mcrl2/data/rewriters/simplify_rewriter.h
/// \brief add your file description here.

#ifndef MCRL2_DATA_REWRITERS_SIMPLIFY_REWRITER_H
#define MCRL2_DATA_REWRITERS_SIMPLIFY_REWRITER_H

#include "mcrl2/data/builder.h"
#include "mcrl2/data/detail/data_sequence_algorithm.h"
#include "mcrl2/data/expression_traits.h"
#include "mcrl2/data/find.h"
#include "mcrl2/data/optimized_boolean_operators.h"

namespace mcrl2 {

namespace data {

namespace detail {

template <typename Derived>
class simplify_rewrite_builder: public data_expression_builder<Derived>
{
  public:
    typedef data_expression_builder<Derived> super;

    using super::apply;

    Derived& derived()
    {
      return static_cast<Derived&>(*this);
    }

    bool is_not(const data_expression& x) const
    {
      return data::sort_bool::is_not_application(x);
    }

    bool is_and(const data_expression& x) const
    {
      return data::sort_bool::is_and_application(x);
    }

    bool is_or(const data_expression& x) const
    {
      return data::sort_bool::is_or_application(x);
    }

    bool is_imp(const data_expression& x) const
    {
      return data::sort_bool::is_implies_application(x);
    }

    bool is_forall(const data_expression& x) const
    {
      return data::is_forall(x);
    }

    bool is_exists(const data_expression& x) const
    {
      return data::is_exists(x);
    }

    data_expression apply(const application& x)
    {
      data_expression result;
      derived().enter(x);
      if (is_not(x)) // x = !y
      {
        data_expression y = derived().apply(*x.begin());
        result = data::optimized_not(y);
      }
      else if (is_and(x)) // x = y && z
      {
        data_expression y = derived().apply(binary_left(x));
        data_expression z = derived().apply(binary_right(x));
        result = data::optimized_and(y, z);
      }
      else if (is_or(x)) // x = y || z
      {
        data_expression y = derived().apply(binary_left(x));
        data_expression z = derived().apply(binary_right(x));
        result = data::optimized_or(y, z);
      }
      else if (is_imp(x)) // x = y => z
      {
        data_expression y = derived().apply(binary_left(x));
        data_expression z = derived().apply(binary_right(x));
        result = data::optimized_imp(y, z);
      }
      else
      {
        result = super::apply(x);
      }
      derived().leave(x);
      return result;
    }

    data_expression apply(const forall& x) // x = forall d. y
    {
      variable_list d = forall(x).variables();
      data_expression y = derived().apply(forall(x).body());
      return data::optimized_forall(d, y, true);
    }

    data_expression apply(const exists& x) // x = exists d. y
    {
      variable_list d = exists(x).variables();
      data_expression y = derived().apply(exists(x).body());
      return data::optimized_exists(d, y, true);
    }
};

} // namespace detail

struct simplify_rewriter: public std::unary_function<data_expression, data_expression>
{
  data_expression operator()(const data_expression& x) const
  {
    return core::make_apply_builder<detail::simplify_rewrite_builder>().apply(x);
  }
};

template <typename T>
void simplify(T& x, typename std::enable_if< !std::is_base_of< atermpp::aterm, T >::value>::type* = 0)
{
  core::make_update_apply_builder<data::data_expression_builder>(simplify_rewriter()).update(x);
}

template <typename T>
T simplify(const T& x, typename std::enable_if< std::is_base_of< atermpp::aterm, T >::value>::type* = nullptr)
{
  T result = core::make_update_apply_builder<data::data_expression_builder>(simplify_rewriter()).apply(x);
  return result;
}

} // namespace data

} // namespace mcrl2

#endif // MCRL2_DATA_REWRITERS_SIMPLIFY_REWRITER_H

