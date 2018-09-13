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
#include "mcrl2/utilities/detail/io.h"
#include "mcrl2/utilities/input_output_tool.h"

using namespace mcrl2;

class mcrl3linearize_tool: public utilities::tools::input_output_tool
{
  protected:
    typedef utilities::tools::input_output_tool super;

    bool expand_structured_sorts = false;
    int max_equation_usage = 0;

    void parse_options(const utilities::command_line_parser& parser) override
    {
      super::parse_options(parser);
      expand_structured_sorts = parser.options.count("expand-structured-sorts") > 0;
      max_equation_usage = parser.option_argument_as<int>("max-equation-usage");
    }

    void add_options(utilities::interface_description& desc) override
    {
      super::add_options(desc);
      desc.add_option("expand-structured-sorts", "expand structured sorts", 'e');
      desc.add_option("max-equation-usage", utilities::make_optional_argument("NAME", "1"),
                  "The maximum times an equation may be duplicated", 'm');
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
      lps::specification lpsspec = linearize(procspec, expand_structured_sorts, max_equation_usage);
      lps::detail::save_lps(lpsspec, output_filename());
      return true;
    }
};

int main(int argc, char* argv[])
{
  return mcrl3linearize_tool().execute(argc, argv);
}
