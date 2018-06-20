// Author(s): Jeroen Keiren
//            Wieger Wesselink 2018
// Copyright: see the accompanying file COPYING or copy at
// https://github.com/mCRL2org/mCRL2/blob/master/COPYING
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
/// \file mcrl2/lts/detail/lts_generation_options.h
/// \brief Options used during state space generation.

#ifndef MCRL2_LTS_DETAIL_LTS_GENERATION_OPTIONS_H
#define MCRL2_LTS_DETAIL_LTS_GENERATION_OPTIONS_H

#include "mcrl2/data/rewrite_strategy.h"
#include "mcrl2/lps/specification.h"
#include "mcrl2/lts/detail/exploration_strategy.h"
#include "mcrl2/process/action_parse.h"

namespace mcrl2 {

namespace lts {

class lts_generation_options
{
  private:
    static const std::size_t default_max_states = ULONG_MAX;
    static const std::size_t default_init_tsize = 10000UL;

  public:
    static const std::size_t default_max_traces = ULONG_MAX;

    lps::specification specification;
    bool instantiate_global_variables = true;
    bool remove_unused_rewrite_rules = true;

    data::rewriter::strategy strat = data::jitty;
    std::size_t todo_max = (std::numeric_limits<std::size_t>::max)();
    std::size_t max_states = default_max_states;
    std::size_t initial_table_size = default_init_tsize;
    bool suppress_progress_messages = false;

    lts_type outformat = lts_none;
    bool outinfo = true;
    std::string filename;

    bool detect_deadlock = false;
    bool detect_nondeterminism = false;
    bool use_enumeration_caching = false;

    /// \brief Constructor
    lts_generation_options() = default;

    /// \brief Copy assignment operator.
    lts_generation_options& operator=(const lts_generation_options&) = default;
};

} // namespace lts
} // namespace mcrl2

#endif // MCRL2_LTS_DETAIL_LTS_GENERATION_OPTIONS_H
