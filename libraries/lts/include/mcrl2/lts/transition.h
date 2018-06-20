// Author(s): Muck van Weerdenburg, Jan Friso Groote
// Copyright: see the accompanying file COPYING or copy at
// https://github.com/mCRL2org/mCRL2/blob/master/COPYING
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

/** \file
 *
 * \brief A header file defining a transition as a triple from,label,to.
 * \author Muck van Weerdenburg, Jan Friso Groote
 */


#ifndef MCRL2_LTS_TRANSITION_H
#define MCRL2_LTS_TRANSITION_H

#include <functional>

namespace mcrl2
{
namespace lts
{

/** \brief Transition sort styles.
 * \details This enumerated type defines sort styles for transitions.
 * They can be used to sort the transitions of an LTS based on various
 * criteria. */
enum transition_sort_style
{
  src_lbl_tgt, /**< Sort first on source state, then on label, then on target state */
  lbl_tgt_src /**< Sort first on label, then on target state, then on source state*/
};

/// \brief A class containing triples, source label and target representing transitions.
/// \details A transition consists of three indices, indicated by transition::size_type
///          that refer to a source, label and target.
class transition
{
  public:
    /// \brief The type of the elements in a transition.
    typedef std::size_t size_type;

  private:
    size_type m_from;
    size_type m_label;
    size_type m_to;

  public:
    // There is no default constructor
    transition() = delete;

    /// \brief Constructor (there is no default constructor).
    transition(const std::size_t f,
               const std::size_t l,
               const std::size_t t):m_from(f),m_label(l),m_to(t)
    {}

    /// \brief Copy constructor.
    transition(const transition& t) = default;
    /* {
      m_from = t.m_from;
      m_label = t.m_label;
      m_to = t.m_to;
    } */

    /// \brief The source of the transition.
    size_type
    from() const
    {
      return m_from;
    }

    /// \brief The label of the transition.
    size_type label() const
    {
      return m_label;
    }

    ///\brief The target of the transition.
    size_type
    to() const
    {
      return m_to;
    }

    /// \brief Set the source of the transition.
    void
    set_from(const size_type from)
    {
      m_from = from;
    }

    /// \brief Set the label of the transition.
    void
    set_label(const size_type label)
    {
      m_label = label;
    }

    ///\brief Set the target of the transition.
    void
    set_to(const size_type to)
    {
      m_to = to;
    }

    ///\brief Standard equality on transitions.
    bool
    operator ==(const transition& t) const
    {
      return m_from == t.m_from && m_label == t.m_label && m_to == t.m_to;
    }

    ///\brief Standard inequality on transitions.
    bool
    operator !=(const transition& t) const
    {
      return !(*this==t);
    }

    ///\brief Standard lexicographic ordering on transitions.
    ///\details The ordering is lexicographic from left to right.
    ///         First t.from are compared, then the label, and
    ///         if these do not determine the ordering, to is investigated.
    bool
    operator <(const transition& t) const
    {
      return m_from < t.m_from || (m_from == t.m_from && (m_label
                                   < t.m_label || (m_label == t.m_label && m_to < t.m_to)));
    }
};

} // namespace lts
} // namespace mcrl2

namespace std
{

/// \brief specialization of the standard std::hash function.
template<>
struct hash<mcrl2::lts::transition>
{
  std::size_t operator()(const mcrl2::lts::transition& t) const
  {
    return t.from() << 2 ^ t.label() << 1 ^ t.to();
  }
};

} // namespace std


#include "mcrl2/lts/detail/transition.h"

#endif // MCRL2_LTS_TRANSITION_H
