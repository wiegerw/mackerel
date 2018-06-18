// Author(s): Wieger Wesselink
// Copyright: see the accompanying file COPYING or copy at
// https://svn.win.tue.nl/trac/MCRL2/browser/trunk/COPYING
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
/// \file parse_test.cpp
/// \brief Add your file description here.

#define BOOST_TEST_MODULE parse_test

#include <iostream>
#include <boost/test/included/unit_test_framework.hpp>
#include "mcrl2/lts_new/parse.h"

using namespace mcrl2;

BOOST_AUTO_TEST_CASE(parse_test1)
{
  std::string text = "des ( 2, 3, 4 ) (0,\"a\",1) (1,\"b\",2) (2,\"c\",3)";
  lts::labeled_transition_system lts1 = lts::parse_lts(text);
  std::cout << lts1 << std::endl;
}
