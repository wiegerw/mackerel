// Author(s): Jeroen Keiren
// Copyright: see the accompanying file COPYING or copy at
// https://svn.win.tue.nl/trac/MCRL2/browser/trunk/COPYING
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
/// \file sumelm.h
/// \brief Provides an implemenation of the sum elimination lemma,
///        as well as the removal of unused summation variables.
///        The sum elimination lemma is the following:
///          sum d:D . d == e -> X(d) = X(e).
///        Removal of unused summation variables is according to the
///        following lemma:
///          d not in x implies sum d:D . x = x

#ifndef MCRL2_LPS_SUMELM_H
#define MCRL2_LPS_SUMELM_H

#include "mcrl2/data/join.h"
#include "mcrl2/data/replace.h"
#include "mcrl2/data/substitutions/mutable_map_substitution.h"
#include "mcrl2/lps/decluster.h"
#include "mcrl2/lps/detail/lps_algorithm.h"
#include "mcrl2/lps/replace.h"

namespace mcrl2
{
namespace lps
{

/// \brief Class implementing the sum elimination lemma.
template <typename Specification = specification>
class sumelm_algorithm: public detail::lps_algorithm<Specification>
{
  typedef typename detail::lps_algorithm<Specification> super;
  using super::m_spec;

  protected:
    /// Stores the number of summation variables that has been removed.
    std::size_t m_removed;

    /// Whether to decluster disjunctive conditions first.
    bool m_decluster;

    /// Adds replacement lhs := rhs to the specified map of replacements.
    /// All replacements that have lhs as a right hand side will be changed to
    /// have rhs as a right hand side.
    void sumelm_add_replacement(data::mutable_map_substitution<>& replacements,
                                const data::variable& lhs,
                                const data::data_expression& rhs)
    {
      using namespace mcrl2::data;
      // First apply already present substitutions to rhs
      data_expression new_rhs = data::replace_variables_capture_avoiding(rhs, replacements, substitution_variables(replacements));
      for (auto& replacement: replacements)
      {
        data::mutable_map_substitution<> sigma;
        sigma[lhs] = new_rhs;
        replacement.second = data::replace_variables_capture_avoiding(replacement.second, sigma, data::substitution_variables(sigma));
      }
      replacements[lhs] = new_rhs;
    }

    /// Returns true if x is a summand variable of summand s.
    bool is_summand_variable(const summand_base& s, const data::data_expression& x)
    {
      const data::variable_list& l=s.summation_variables();
      return data::is_variable(x) && std::find(l.begin(),l.end(),atermpp::down_cast<data::variable>(x))!=l.end();
    }

    template <typename T>
    void swap(T& x, T& y)
    {
      T temp(x);
      x = y;
      y = temp;
    }

    data::data_expression compute_substitutions(
                  const summand_base& s,
                  data::mutable_map_substitution<>& substitutions)
    {
      using namespace data;

      std::set<data_expression> new_conjuncts;

      for(const data::data_expression& conjunct: data::split_and(s.condition()))
      {

        bool replacement_added(false);
        data_expression left;
        data_expression right;

        if (is_equal_to_application(conjunct)) // v == e
        {
          left = data::binary_left(application(conjunct));
          right = data::binary_right(application(conjunct));
        }
        else if (is_variable(conjunct) && sort_bool::is_bool(conjunct.sort())) // v equal to v == true
        {
          left = conjunct;
          right = sort_bool::true_();
        }
        else if (sort_bool::is_not_application(conjunct) && is_variable(sort_bool::arg(conjunct))) // !v equal to v == false
        {
          left = sort_bool::arg(conjunct);
          right = sort_bool::false_();
        }

        // This conjunct was one of the above three cases; see if we can build
        // a prober substitution
        if(left != data_expression() && right != data_expression())
        {
          if(!is_summand_variable(s, left) && is_summand_variable(s,right))
          {
            swap(left, right);
          }

          // Expression x == e; we only add a substitution if x is a summation variable, and x does not occur in e;
          // We evaluate the following three cases:
          // 1. there is no substitution assinging to x yet -> add x := e
          // 2. there is a substitution x := d, and e is a summation variable,
          //     for which there is no substitution yet -> add e := x;
          // 3. there is a substitution x := d, and d is a summation variable,
          //    for which there is no substitution yet -> add d := e, and x := e
          if (is_summand_variable(s, left) && !search_data_expression(right, left))
          {
            const data::variable& vleft = atermpp::down_cast<data::variable>(left);
            const data::variable& vright = atermpp::down_cast<data::variable>(right);

            // Check if we already have a substition with left as left hand side
            if (substitutions.find(vleft) == substitutions.end())
            {
              sumelm_add_replacement(substitutions, vleft, right);
              replacement_added = true;
            }
            else if (is_summand_variable(s, right) && substitutions.find(vright) == substitutions.end())
            {
              sumelm_add_replacement(substitutions, vright, substitutions(vleft));
              replacement_added = true;
            }
            else
            {
              data::variable v = atermpp::down_cast<data::variable>(substitutions(vleft));
              if (is_summand_variable(s, v) && substitutions.find(v) != substitutions.end())
              {
                sumelm_add_replacement(substitutions, v, right);
                sumelm_add_replacement(substitutions, vleft, right);
                replacement_added = true;
              }
            }
          }
        }

        if(!replacement_added)
        {
          new_conjuncts.insert(conjunct);
        }
      }

      return data::join_and(new_conjuncts.begin(), new_conjuncts.end());
    }

  public:
    /// \brief Constructor.
    /// \param spec The specification to which sum elimination should be
    ///             applied.
    /// \param decluster Control whether disjunctive conditions need to be split
    ///        into multiple summands.
    sumelm_algorithm(Specification& spec, bool decluster = false)
      : lps::detail::lps_algorithm<Specification>(spec),
        m_removed(0),
        m_decluster(decluster)
    {}

    /// \brief Apply the sum elimination lemma to all summands in the
    ///        specification.
    void run()
    {
      if (m_decluster)
      {
        // First decluster specification
        decluster_algorithm<Specification>(m_spec).run();
      }

      m_removed = 0; // Re-initialise number of removed variables for a fresh run.

      for (action_summand& s: m_spec.process().action_summands())
      {
        (*this)(s);
      }

      for (deadlock_summand& s: m_spec.process().deadlock_summands())
      {
        (*this)(s);
      }

      mCRL2log(log::verbose) << "Removed " << m_removed << " summation variables" << std::endl;
    }

    /// \brief Apply the sum elimination lemma to summand s.
    /// \param s an action_summand.
    void operator()(action_summand& s)
    {
      data::mutable_map_substitution<> substitutions;
      s.condition() = compute_substitutions(s, substitutions);

      // temporarily remove the summation variables, otherwise the capture avoiding substitution will touch them
      const data::variable_list summmation_variables = s.summation_variables();
      s.summation_variables() = data::variable_list();

      lps::replace_variables_capture_avoiding(s, substitutions, data::substitution_variables(substitutions));

      // restore the summation variables
      s.summation_variables() = summmation_variables;

      const std::size_t var_count = s.summation_variables().size();
      super::summand_remove_unused_summand_variables(s);
      m_removed += var_count - s.summation_variables().size();
    }

    /// \brief Apply the sum elimination lemma to summand s.
    /// \param s a deadlock_summand.
    void operator()(deadlock_summand& s)
    {
      data::mutable_map_substitution<> substitutions;
      s.condition() = compute_substitutions(s, substitutions);
      lps::replace_variables_capture_avoiding(s, substitutions, data::substitution_variables(substitutions));

      const std::size_t var_count = s.summation_variables().size();
      super::summand_remove_unused_summand_variables(s);
      m_removed += var_count - s.summation_variables().size();
    }

    /// \brief Returns the amount of removed summation variables.
    std::size_t removed() const
    {
      return m_removed;
    }
};

/// \brief Apply the sum elimination lemma to summand s.
/// \param s an action summand
/// \return \c true if any summation variables have been removed, or \c false otherwise.
inline
bool sumelm(action_summand& s)
{
  specification spec;
  sumelm_algorithm<specification> algorithm(spec);
  algorithm(s);
  return algorithm.removed() > 0;
}

/// \brief Apply the sum elimination lemma to summand s.
/// \param s a stochastic action summand
/// \return \c true if any summation variables have been removed, or \c false otherwise.
inline
bool sumelm(stochastic_action_summand& s)
{
  stochastic_specification spec;
  sumelm_algorithm<stochastic_specification> algorithm(spec);
  algorithm(s);
  return algorithm.removed() > 0;
}

/// \brief Apply the sum elimination lemma to summand s.
/// \param s a deadlock summand
/// \return \c true if any summation variables have been removed, or \c false otherwise.
inline
bool sumelm(deadlock_summand& s)
{
  specification spec;
  sumelm_algorithm<specification> algorithm(spec);
  algorithm(s);
  return algorithm.removed() > 0;
}

} // namespace lps
} // namespace mcrl2

#endif // MCRL2_LPS_SUMELM_H

