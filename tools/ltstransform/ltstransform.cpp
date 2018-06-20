// Author(s): Wieger Wesselink
// Copyright: see the accompanying file COPYING or copy at
// https://svn.win.tue.nl/trac/MCRL2/browser/trunk/COPYING
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
/// \file ltstransform.cpp

#include <iostream>
#include <sstream>

#include "mcrl2/core/detail/print_utility.h"
#include "mcrl2/lts_new/parse.h"
#include "mcrl2/lts_new/remove_tau_action.h"
#include "mcrl2/utilities/detail/command.h"
#include "mcrl2/utilities/detail/io.h"
#include "mcrl2/utilities/detail/transform_tool.h"
#include "mcrl2/utilities/input_output_tool.h"

using namespace mcrl2;
using utilities::detail::transform_tool;
using utilities::tools::input_output_tool;

struct lts_command: public utilities::detail::command
{
  lts::labeled_transition_system ltsspec;

  lts_command(const std::string& name,
              const std::string& input_filename,
              const std::string& output_filename,
              const std::vector<std::string>& options
             )
    : utilities::detail::command(name, input_filename, output_filename, options)
  {}

  void execute() override
  {
    std::string text = utilities::detail::read_text(input_filename);
    ltsspec = lts::parse_lts(text);
  }

  void save()
  {
    std::ostringstream out;
    out << ltsspec;
    utilities::detail::write_text(output_filename, out.str());
  }
};

/// \brief Eliminates trivial process equations of the shape P = Q
struct remove_tau_action_command: public lts_command
{
  remove_tau_action_command(const std::string& input_filename, const std::string& output_filename, const std::vector<std::string>& options)
    : lts_command("remove-tau", input_filename, output_filename, options)
  {}

  void execute() override
  {
    lts_command::execute();
    lts::remove_tau_action(ltsspec);
    save();
  }
};

class ltstransform_tool: public transform_tool<input_output_tool>
{
  typedef transform_tool<input_output_tool> super;

  public:
    ltstransform_tool()
      : super("ltstransform",
              "Wieger Wesselink",
              "applies a transformation to an LTS",
              "Transform the object in INFILE and write the result to OUTFILE. If OUTFILE "
              "is not present, stdout is used. If INFILE is not present, stdin is used."
             )
    {}

    void add_commands(const std::vector<std::string>& options) override
    {
      add_command(std::make_shared<remove_tau_action_command>(input_filename(), output_filename(), options));
    }
};

int main(int argc, char* argv[])
{
  return ltstransform_tool().execute(argc, argv);
}
