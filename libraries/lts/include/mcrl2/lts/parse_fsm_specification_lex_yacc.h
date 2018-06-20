// Author(s): Wieger Wesselink
// Copyright: see the accompanying file COPYING or copy at
// https://svn.win.tue.nl/trac/MCRL2/browser/trunk/COPYING
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
/// \file mcrl2/lts/parse_fsm_specification_lex_yacc.h
/// \brief add your file description here.

#ifndef MCRL2_LTS_PARSE_FSM_SPECIFICATION_LEX_YACC_H
#define MCRL2_LTS_PARSE_FSM_SPECIFICATION_LEX_YACC_H

#include "../../../source/liblts_fsmparser.h"

namespace mcrl2 {

namespace lts {

inline
void parse_fsm_specification_lex_yacc(std::istream& in, lts_fsm_t& result)
{
  parse_fsm(in, result);
}

inline
void parse_fsm_specification_lex_yacc(const std::string& text, lts_fsm_t& result)
{
  std::stringstream ss(text);
  parse_fsm_specification_lex_yacc(ss, result);
}

} // namespace lts

} // namespace mcrl2

#endif // MCRL2_LTS_PARSE_FSM_SPECIFICATION_LEX_YACC_H
