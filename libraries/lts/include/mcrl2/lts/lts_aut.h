// Author(s): Jan Friso Groote
// Copyright: see the accompanying file COPYING or copy at
// https://github.com/mCRL2org/mCRL2/blob/master/COPYING
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

/** \file
 *
 * \brief This file contains a class that contains labelled transition systems in aut format.
 * \details A labelled transition system in aut format is a transition system
 * with empty state labels, and strings as transition labels.
 * \author Jan Friso Groote
 */


#ifndef MCRL2_LTS_LTS_AUT_H
#define MCRL2_LTS_LTS_AUT_H

#include <istream>
#include "mcrl2/lps/state_probability_pair.h"
#include "mcrl2/lts/probabilistic_arbitrary_precision_fraction.h"
#include "mcrl2/lts/state_label_empty.h"
#include "mcrl2/lts/action_label_string.h"
#include "mcrl2/lts/probabilistic_lts.h"


namespace mcrl2
{
namespace lts
{

namespace detail
{



class lts_aut_base
{
  public:

    typedef mcrl2::lts::probabilistic_state<std::size_t, mcrl2::lts::probabilistic_arbitrary_precision_fraction> probabilistic_state;
    typedef mcrl2::lps::state_probability_pair<std::size_t, mcrl2::lts::probabilistic_arbitrary_precision_fraction> state_probability_pair;

    /** \brief Provides the type of this lts, in casu lts_aut.  */
    lts_type type()
    {
      return lts_aut;
    }

    /** \brief Standard swap function. */
    void swap(lts_aut_base& )
    {
      // Does intentionally not provide any action.
    }
};

} // end namespace detail


/** \brief A simple labelled transition format with only strings as action labels.
 *  \details This lts format corresponds to the Ceasar/Aldebaran labelled transition
 *  system format. There are no state labels, only transition labels which are plain
 *  strings.
 */
class lts_aut_t : public lts< state_label_empty, action_label_string, detail::lts_aut_base>
{
  public:

    /** \brief Load the labelled transition system from a file.
     *  \details If the filename is empty, the result is read from stdin.
                 The input file must be in .aut format.
     *  \param[in] filename Name of the file from which this lts is read.
     */
    void load(const std::string& filename);

    /** \brief Load the labelled transition system from an input stream.
     *  \details The input stream must be in .aut format.
     *  \param[in] is The input stream.
     */
    void load(std::istream& is);

    /** \brief Save the labelled transition system to file.
     *  \details If the filename is empty, the result is written to stdout.
     *  \param[in] filename Name of the file to which this lts is written.
     */
    void save(const std::string& filename) const;
};

/** \brief A simple labelled transition format with only strings as action labels.
 *  \details This lts format corresponds to the Ceasar/Aldebaran labelled transition
 *  system format. There are no state labels, only transition labels which are plain
 *  strings.
 */
class probabilistic_lts_aut_t : 
            public probabilistic_lts< state_label_empty, 
                                      action_label_string, 
                                      detail::lts_aut_base::probabilistic_state, 
                                      detail::lts_aut_base>
{
  public:

    /** \brief Load the labelled transition system from a file.
     *  \details If the filename is empty, the result is read from stdin.
                 The input file must be in .aut format.
     *  \param[in] filename Name of the file from which this lts is read.
     */
    void load(const std::string& filename);

    /** \brief Load the labelled transition system from an input stream.
     *  \details The input stream must be in .aut format.
     *  \param[in] is The input stream.
     */
    void load(std::istream& is);

    /** \brief Save the labelled transition system to file.
     *  \details If the filename is empty, the result is written to stdout.
     *  \param[in] filename Name of the file to which this lts is written.
     */
    void save(const std::string& filename) const;
};

} // namespace lts
} // namespace mcrl2

#endif
