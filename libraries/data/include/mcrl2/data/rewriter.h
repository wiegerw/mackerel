// Author(s): Wieger Wesselink
// Copyright: see the accompanying file COPYING or copy at
// https://svn.win.tue.nl/trac/MCRL2/browser/trunk/COPYING
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
/// \file mcrl2/data/rewriter.h
/// \brief The class rewriter.

#ifndef MCRL2_DATA_REWRITER_H
#define MCRL2_DATA_REWRITER_H

// TODO: this include should not be necessary, it is a problem in the design of the data library
#include "mcrl2/data/expression_traits.h"

#include "mcrl2/data/data_specification.h"
#include "mcrl2/data/detail/rewrite.h"
#include "mcrl2/data/substitutions/mutable_indexed_substitution.h"

namespace mcrl2
{

namespace data
{

/// \brief Rewriter that operates on data expressions.
//
/// \attention As long as normalisation of sorts remains necessary, the data
/// specification object used for construction *must* exist during the
/// lifetime of the rewriter object.
//
// TODO: The rewriter only supports data::mutable_indexed_substitution<> as substitution type. This
// needs to be fixed, because it is a major source of inefficiency.
// TODO: The rewriter is implemented in terms of detail::Rewriter. This needs to be fixed, because
// the class detail::Rewriter is unspecified, and has several problems.
class rewriter
{
  protected:
    /// \brief The wrapped Rewriter.
    std::shared_ptr<detail::Rewriter> m_rewriter;

  public:
    /// \brief The type for the substitution that is used internally.
    typedef data::mutable_indexed_substitution<> substitution_type;

    /// \brief The type for expressions manipulated by the rewriter.
    typedef data_expression term_type;

    /// \brief The rewrite strategies of the rewriter.
    typedef rewrite_strategy strategy;

  public:
    /// \brief Copy constructor.
    /// \param[in] r a rewriter.
    rewriter(const rewriter& r) = default;

    /// \brief Constructor.
    /// \param[in] dataspec A data specification
    /// \param[in] s A rewriter strategy.
    explicit rewriter(const data_specification& dataspec = rewriter::default_specification(), strategy s = jitty):
      m_rewriter(detail::createRewriter(dataspec, used_data_equation_selector(dataspec), s))
    { }

    /// \brief Constructor.
    /// \param[in] dataspec A data specification
    /// \param[in] selector A component that selects the equations that are converted to rewrite rules
    /// \param[in] s A rewriter strategy.
    template < typename EquationSelector >
    rewriter(const data_specification& dataspec, const EquationSelector& equation_selector, strategy s = jitty) :
      m_rewriter(detail::createRewriter(dataspec, equation_selector, s))
    { }

    /// \brief Default specification used if no specification is specified at construction
    static data_specification& default_specification()
    {
      static data_specification specification;
      return specification;
    }

    /// \brief Rewrites a data expression.
    /// \param[in] x A data expression
    /// \return The normal form of x.
    /// N.B. This function is extremely inefficient, due to limitations of the detail::Rewriter class.
    data_expression operator()(const data_expression& x) const
    {
      substitution_type sigma;
      return m_rewriter->rewrite(x, sigma);
    }

    /// \brief Rewrites the data expression x, and on the fly applies a substitution
    /// to data variables.
    /// \param[in] x A data expression
    /// \param[in] sigma A substitution function
    /// \return The normal form of the term.
    /// N.B. This function is extremely inefficient, due to limitations of the detail::Rewriter class.
    template <typename SubstitutionFunction>
    data_expression operator()(const data_expression& x, const SubstitutionFunction& sigma) const
    {
      substitution_type sigma_copy;
      for(const variable& v: data::find_free_variables(x))
      {
        sigma_copy[v] = sigma(v);
      }
      return m_rewriter->rewrite(x, sigma_copy);
    }

    /// \brief Rewrites the data expression x, and on the fly applies a substitution
    /// to data variables.
    /// \param[in] x A data expression
    /// \param[in] sigma A substitution function
    /// \return The normal form of the term.
    data_expression operator()(const data_expression& x, substitution_type& sigma) const
    {
      return m_rewriter->rewrite(x, sigma);
    }
};

} // namespace data

} // namespace mcrl2

#endif // MCRL2_DATA_REWRITER_H
