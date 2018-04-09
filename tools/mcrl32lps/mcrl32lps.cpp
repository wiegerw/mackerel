// Copyright: Wieger Wesselink
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
/// \file mcrl32lps.cpp

#define NAME "mcrl32lps"
#define AUTHOR "Wieger Wesselink"

#include <iostream>

#include "mcrl2/process/parse.h"
#include "mcrl2/utilities/logger.h"
#include "mcrl2/utilities/detail/io.h"
#include "mcrl2/utilities/input_output_tool.h"
#include "mcrl2/utilities/text_utility.h"

using namespace mcrl2;

class mcrl32lps_tool: public utilities::tools::input_output_tool
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
    mcrl32lps_tool()
      : super(NAME, AUTHOR,
              "linearize process specifications",
              "Linearizes the process specification in INFILE."
             )
    {}

    bool run() override
    {
      return true;
    }
};

int main(int argc, char* argv[])
{
  return mcrl32lps_tool().execute(argc, argv);
}
