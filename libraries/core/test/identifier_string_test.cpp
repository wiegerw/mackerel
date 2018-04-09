// Author(s): Wieger Wesselink
// Copyright: see the accompanying file COPYING or copy at
// https://svn.win.tue.nl/trac/MCRL2/browser/trunk/COPYING
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
/// \file identifier_string_test.cpp
/// \brief Add your file description here.

#include "mcrl2/core/identifier_string.h"
#include <boost/test/minimal.hpp>
#include <iostream>

using namespace mcrl2;

// Example that didn't compile, submitted by Jan Friso.
void test_identifier_string()
{
  core::identifier_string a("abc");
  const core::identifier_string& b = a;

  const core::identifier_string_list l;
  for (const auto& s: l)
  {
    std::cout << s;
  }
}

int test_main(int argc, char** argv)
{
  test_identifier_string();

  return 0;
}
