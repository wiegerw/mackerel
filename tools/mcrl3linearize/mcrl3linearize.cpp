// Copyright: Wieger Wesselink
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
/// \file mcrl3linearize.cpp

#define NAME "mcrl3linearize"
#define AUTHOR "Wieger Wesselink"

#include <iostream>
#include "mcrl2/lps/detail/lps_io.h"
#include "mcrl2/process/detail/process_io.h"
#include "mcrl2/process/linearize.h"
#include "mcrl2/utilities/logger.h"
#include "mcrl2/utilities/detail/io.h"
#include "mcrl2/utilities/input_output_tool.h"
#include "mcrl2/utilities/text_utility.h"

using namespace mcrl2;

class mcrl3linearize_tool: public utilities::tools::input_output_tool
{
  protected:
    typedef utilities::tools::input_output_tool super;

    void parse_options(const utilities::command_line_parser& parser) override
    {
      super::parse_options(parser);
    }

    void add_options(utilities::interface_description& desc) override
    {
      super::add_options(desc);
    }

  public:
    mcrl3linearize_tool()
      : super(NAME, AUTHOR,
              "linearize process specifications",
              "Linearizes the process specification in INFILE. N.B. Supports a very limited class of\n"
              "process specifications!"
             )
    {}

    bool run() override
    {
      timer().start("parse + type check process specification");
      process::process_specification procspec = process::detail::parse_process_specification(input_filename());
      timer().finish("parse + type check process specification");
      lps::specification lpsspec = linearize(procspec);
      lps::detail::save_lps(lpsspec, output_filename());
      return true;
    }
};

int main(int argc, char* argv[])
{
  return mcrl3linearize_tool().execute(argc, argv);
}
