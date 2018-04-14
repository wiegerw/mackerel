// Author(s): Wieger Wesselink
// Copyright: see the accompanying file COPYING or copy at
// https://svn.win.tue.nl/trac/MCRL2/browser/trunk/COPYING
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
/// \file mcrl2/data/normalize_sorts.h
/// \brief add your file description here.

#ifndef MCRL2_DATA_NORMALIZE_SORTS_H
#define MCRL2_DATA_NORMALIZE_SORTS_H

#include "mcrl2/data/builder.h"
#include "mcrl2/data/sort_specification.h"
#include <functional>

namespace mcrl2
{

namespace data
{

namespace detail
{

struct normalize_sorts_builder: public sort_expression_builder<normalize_sorts_builder>
{
  typedef sort_expression_builder<normalize_sorts_builder> super;
  using super::apply;

  const std::map<sort_expression, sort_expression>& m_normalized_aliases;

  normalize_sorts_builder(const data::sort_specification& sortspec)
    : m_normalized_aliases(sortspec.sort_alias_map())
  {}

  sort_expression apply(const sort_expression& x)
  {
    auto i = m_normalized_aliases.find(x);
    if (i != m_normalized_aliases.end())
    {
      return i->second;
    }
    sort_expression result = super::apply(x);

    // Rewrite result to normal form.
    auto j = m_normalized_aliases.find(result);
    if (j != m_normalized_aliases.end())
    {
      result = apply(j->second);
    }

    return result;
  }
};

struct normalize_sorts_function: public std::unary_function<data::sort_expression, data::sort_expression>
{
  const data::sort_specification& sortspec;

  normalize_sorts_function(const data::sort_specification& sortspec_)
    : sortspec(sortspec_)
  {}

  sort_expression operator()(const sort_expression& x) const
  {
    return normalize_sorts_builder(sortspec).apply(x);
  }
};

} // namespace detail

template <typename T>
void normalize_sorts(T& x,
                     const data::sort_specification& sortspec,
                     typename std::enable_if< !std::is_base_of<atermpp::aterm, T>::value >::type* = nullptr
                    )
{
  core::make_update_apply_builder<data::sort_expression_builder>
  (data::detail::normalize_sorts_function(sortspec)).update(x);
}

template <typename T>
T normalize_sorts(const T& x,
                  const data::sort_specification& sortspec,
                  typename std::enable_if< std::is_base_of<atermpp::aterm, T>::value >::type* = nullptr
                 )
{
  return core::make_update_apply_builder<data::sort_expression_builder>
         (data::detail::normalize_sorts_function(sortspec)).apply(x);
}

} // namespace data

} // namespace mcrl2

#endif // MCRL2_DATA_NORMALIZE_SORTS_H
