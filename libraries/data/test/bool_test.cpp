// Author(s): Jeroen Keiren
// Copyright: see the accompanying file COPYING or copy at
// https://svn.win.tue.nl/trac/MCRL2/browser/trunk/COPYING
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
/// \file data_expression_test.cpp
/// \brief Basic regression test for data expressions.

#include <boost/test/minimal.hpp>
#include <iostream>

#include "mcrl2/data/bool.h"


using namespace mcrl2;
using namespace mcrl2::data;
using namespace mcrl2::data::sort_bool;

void bool_sort_test()
{
  const basic_sort& b(bool_());
  BOOST_CHECK(b == bool_());
  BOOST_CHECK(pp(b.name()) == "Bool");
}

int test_main(int argc, char** argv)
{
  bool_sort_test();

  return EXIT_SUCCESS;
}
