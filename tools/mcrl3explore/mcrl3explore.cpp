// Author(s): Muck van Weerdenburg
//            Wieger Wesselink 2018
// Copyright: see the accompanying file COPYING or copy at
// https://github.com/mCRL2org/mCRL2/blob/master/COPYING
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
/// \file lps2lts_lts.cpp

#include <cassert>
#include <csignal>
#include <memory>
#include <string>

#include "mcrl2/data/rewriter_tool.h"
#include "mcrl2/lps/io.h"
#include "mcrl2/lts/detail/exploration.h"
#include "mcrl2/lts/lts_io.h"
#include "mcrl2/process/action_parse.h"
#include "mcrl2/utilities/input_output_tool.h"
#include "mcrl2/utilities/logger.h"

using namespace mcrl2;
using namespace mcrl2::utilities::tools;
using namespace mcrl2::utilities;
using namespace mcrl2::core;
using namespace mcrl2::lts;
using namespace mcrl2::lps;
using namespace mcrl2::log;
using mcrl2::data::tools::rewriter_tool;

struct abortable
{
  virtual void abort() = 0;
};

struct next_state_abortable: public abortable
{
  lps2lts_algorithm<lps::next_state_generator>* algorithm;

  explicit next_state_abortable(lps2lts_algorithm<lps::next_state_generator>* algorithm_)
    : algorithm(algorithm_)
  {}

  void abort() override
  {
    algorithm->abort();
  }
};

struct cached_next_state_abortable: public abortable
{
  lps2lts_algorithm<lps::cached_next_state_generator>* algorithm;

  explicit cached_next_state_abortable(lps2lts_algorithm<lps::cached_next_state_generator>* algorithm_)
    : algorithm(algorithm_)
  {}

  void abort() override
  {
    algorithm->abort();
  }
};

class mcrl3explore_tool: public rewriter_tool<input_output_tool>
{
  protected:
    typedef rewriter_tool<input_output_tool> super;

    lts_generation_options m_options;
    std::string m_filename;
    abortable* m_abortable = nullptr;

  public:
    mcrl3explore_tool():
      super("mcrl3explore", "Wieger Wesselink",
            "generate an LTS from an LPS",
            "Generate an LTS from the LPS in INFILE and save the result to OUTFILE. "
            "If INFILE is not supplied, stdin is used. "
            "If OUTFILE is not supplied, the LTS is not stored.\n"
            "\n"
            "If the 'jittyc' rewriter is used, then the MCRL2_COMPILEREWRITER environment "
            "variable (default value: 'mcrl2compilerewriter') determines the script that "
            "compiles the rewriter, and MCRL2_COMPILEDIR (default value: '.') determines "
            "where temporary files are stored.\n"
            "\n"
            "Note that mcrl3explore can deliver multiple transitions with the same label between"
            "any pair of states. If this is not desired, such transitions can be removed by"
            "applying a strong bisimulation reducton using for instance the tool ltsconvert.\n"
            "\n"
            "The format of OUTFILE is determined by its extension (unless it is specified "
            "by an option). The supported formats are:\n"
            "\n"
            +mcrl2::lts::detail::supported_lts_formats_text()+"\n"
            "If the jittyc rewriter is used, then the MCRL2_COMPILEREWRITER environment "
            "variable (default value: mcrl2compilerewriter) determines the script that "
            "compiles the rewriter, and MCRL2_COMPILEDIR (default value: '.') "
            "determines where temporary files are stored."
            "\n"
            "Note that mcrl3explore can deliver multiple transitions with the same "
            "label between any pair of states. If this is not desired, such "
            "transitions can be removed by applying a strong bisimulation reducton "
            "using for instance the tool ltsconvert."
           )
    {
    }

    void abort()
    {
      m_abortable->abort();
    }

    bool run() override
    {
      load_lps(m_options.specification, m_filename);

      try
      {
        if (m_options.use_enumeration_caching)
        {
          lps2lts_algorithm<lps::cached_next_state_generator> algorithm;
          m_abortable = new cached_next_state_abortable(&algorithm);
          algorithm.generate_lts(m_options);
        }
        else
        {
          lps2lts_algorithm<lps::next_state_generator> algorithm;
          m_abortable = new next_state_abortable(&algorithm);
          algorithm.generate_lts(m_options);
        }
      }
      catch (mcrl2::runtime_error& e)
      {
        mCRL2log(error) << e.what() << std::endl;
        return false;
      }

      return true;
    }

  protected:
    void add_options(interface_description& desc) override
    {
      super::add_options(desc);

      desc.
      add_option("cached",
                 "use enumeration caching techniques to speed up state space generation. ").
      add_option("dummy", make_mandatory_argument("BOOL"),
                 "replace free variables in the LPS with dummy values based on the value of BOOL: 'yes' (default) or 'no'. ", 'y').
      add_option("unused-data",
                 "do not remove unused parts of the data specification. ", 'u').
      add_option("max", make_mandatory_argument("NUM"),
                 "explore at most NUM states", 'l').
      add_option("todo-max", make_mandatory_argument("NUM"),
                 "keep at most NUM states in todo lists; this option is only relevant for "
                 "breadth-first search, where NUM is the maximum number of states per "
                 "level, and for depth first search, where NUM is the maximum depth. ").
      add_option("nondeterminism",
                 "detect nondeterministic states, i.e. states with outgoing transitions with the same label to different states. ", 'n').
      add_option("deadlock",
                 "detect deadlocks (i.e. for every deadlock a message is printed). ", 'D').
      add_option("out", make_mandatory_argument("FORMAT"),
                 "save the output in the specified FORMAT. ", 'o').
      add_option("no-info", "do not add state information to OUTFILE. "
                 "Without this option mcrl3explore adds state vector to the LTS. This "
                 "option causes this information to be discarded and states are only "
                 "indicated by a sequence number. Explicit state information is useful "
                 "for visualisation purposes, for instance, but can cause the OUTFILE "
                 "to grow considerably. Note that this option is implicit when writing "
                 "in the AUT format. ").
      add_option("suppress","in verbose mode, do not print progress messages indicating the number of visited states and transitions. "
                 "For large state spaces the number of progress messages can be quite "
                 "horrendous. This feature helps to suppress those. Other verbose messages, "
                 "such as the total number of states explored, just remain visible. ").
      add_option("init-tsize", make_mandatory_argument("NUM"),
                 "set the initial size of the internally used hash tables (default is 10000). ");
    }

    void parse_options(const command_line_parser& parser) override
    {
      super::parse_options(parser);
      m_options.remove_unused_rewrite_rules = parser.options.count("unused-data") == 0;
      m_options.detect_deadlock             = parser.options.count("deadlock") != 0;
      m_options.detect_nondeterminism       = parser.options.count("nondeterminism") != 0;
      m_options.outinfo                     = parser.options.count("no-info") == 0;
      m_options.suppress_progress_messages  = parser.options.count("suppress") != 0;
      m_options.strat                       = parser.option_argument_as<mcrl2::data::rewriter::strategy>("rewriter");
      m_options.use_enumeration_caching     = parser.options.count("cached") > 0;

      if (parser.options.count("dummy"))
      {
        if (parser.options.count("dummy") > 1)
        {
          parser.error("Multiple use of option -y/--dummy; only one occurrence is allowed.");
        }
        std::string dummy_str(parser.option_argument("dummy"));
        if (dummy_str == "yes")
        {
          m_options.instantiate_global_variables = true;
        }
        else if (dummy_str == "no")
        {
          m_options.instantiate_global_variables = false;
        }
        else
        {
          parser.error("Option -y/--dummy has illegal argument '" + dummy_str + "'.");
        }
      }

      if (parser.options.count("max"))
      {
        m_options.max_states = parser.option_argument_as<unsigned long> ("max");
      }

      if (parser.options.count("out"))
      {
        m_options.outformat = mcrl2::lts::detail::parse_format(parser.option_argument("out"));

        if (m_options.outformat == lts_none)
        {
          parser.error("Format '" + parser.option_argument("out") + "' is not recognised.");
        }
      }
      if (parser.options.count("init-tsize"))
      {
        m_options.initial_table_size = parser.option_argument_as< unsigned long >("init-tsize");
      }
      if (parser.options.count("todo-max"))
      {
        m_options.todo_max = parser.option_argument_as< unsigned long >("todo-max");
      }

      if (parser.options.count("suppress") && !mCRL2logEnabled(verbose))
      {
        parser.error("Option --suppress requires --verbose (of -v).");
      }

      if (2 < parser.arguments.size())
      {
        parser.error("Too many file arguments.");
      }
      if (!parser.arguments.empty())
      {
        m_filename = parser.arguments[0];
      }
      if (1 < parser.arguments.size())
      {
        m_options.filename = parser.arguments[1];
      }

      if (!m_options.filename.empty() && m_options.outformat == lts_none)
      {
        m_options.outformat = mcrl2::lts::detail::guess_format(m_options.filename);

        if (m_options.outformat == lts_none)
        {
          mCRL2log(warning) << "no output format set or detected; using default (mcrl2)" << std::endl;
          m_options.outformat = lts_lts;
        }
      }
    }
};

std::unique_ptr<mcrl3explore_tool> tool_instance;

static
void premature_termination_handler(int)
{
  // Reset signal handlers.
  signal(SIGABRT, nullptr);
  signal(SIGINT, nullptr);
  tool_instance->abort();
}

int main(int argc, char** argv)
{
  tool_instance = std::make_unique<mcrl3explore_tool>();
  signal(SIGABRT, premature_termination_handler);
  signal(SIGINT, premature_termination_handler); // At ^C invoke the termination handler.
  return tool_instance->execute(argc, argv);
}
