// Author(s): Wieger Wesselink
// Copyright: see the accompanying file COPYING or copy at
// https://svn.win.tue.nl/trac/MCRL2/browser/trunk/COPYING
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
/// \file mcrl2/lts/parse_fsm_specification_dparser.h
/// \brief add your file description here.

#ifndef MCRL2_LTS_PARSE_FSM_SPECIFICATION_DPARSER_H
#define MCRL2_LTS_PARSE_FSM_SPECIFICATION_DPARSER_H

#include <sstream>
#include <boost/algorithm/string/trim.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/xpressive/xpressive.hpp>

#include "mcrl2/core/parse.h"
#include "mcrl2/core/parser_utility.h"
#include "mcrl2/lts/lts_fsm.h"
#include "mcrl2/lts/detail/fsm_builder.h"
#include "mcrl2/utilities/parse_numbers.h"
#include "mcrl2/utilities/text_utility.h"

namespace mcrl2 {

namespace lts {

namespace detail {

struct probabilistic_state
{
  std::string number;
  std::pair<std::string, std::string> distribution;

  bool has_distribution() const
  {
    return !distribution.first.empty();
  }

  probabilistic_state(std::string number_ = "0", const std::pair<std::string, std::string>& distribution_ = std::pair<std::string, std::string>())
    : number(number_), distribution(distribution_)
  {}
};

inline
std::ostream& operator<<(std::ostream& out, const probabilistic_state& s)
{
  out << s.number;
  if (s.has_distribution())
  {
    out << "[ " << s.distribution.first << " / " << s.distribution.second << " ]";
  }
  return out;
}

/// \brief Parse actions for FSM format
struct fsm_actions: public core::default_parser_actions
{
  fsm_actions(const core::parser& parser_, probabilistic_lts_fsm_t& fsm)
    : core::default_parser_actions(parser_),
      builder(fsm)
  {}

  fsm_builder builder;

  template <typename T, typename Function>
  std::vector<T> parse_vector(const core::parse_node& node, const std::string& type, Function f)
  {
    std::vector<T> result;
    traverse(node, make_collector(m_parser.symbol_table(), type, result, f));
    return result;
  }

  std::string parse_Id(const core::parse_node& node)
  {
    return node.string();
  }

  std::vector<std::string> parse_IdList(const core::parse_node& node)
  {
    return parse_vector<std::string>(node, "Id", [&](const core::parse_node& node) { return parse_Id(node); });
  }

  std::string parse_QuotedString(const core::parse_node& node)
  {
    std::string s = node.string();
    return s.substr(1, s.size() - 2);
  }

  std::string parse_Number(const core::parse_node& node)
  {
    return node.string();
  }

  std::vector<std::string> parse_NumberList(const core::parse_node& node)
  {
    return parse_vector<std::string>(node, "Number", [&](const core::parse_node& node) { return parse_Number(node); });
  }

  std::string parse_ParameterName(const core::parse_node& node)
  {
    return parse_Id(node.child(0));
  }

  std::string parse_SortExpr(const core::parse_node& node)
  {
    std::string result = parse_Id(node.child(0));
    if (result.empty())
    {
      result = "Nat";
    }
    return boost::algorithm::trim_copy(result);
  }

  std::string parse_DomainCardinality(const core::parse_node& node)
  {
    return parse_Number(node.child(1));
  }

  std::string parse_DomainValue(const core::parse_node& node)
  {
    return parse_QuotedString(node.child(0));
  }

  std::vector<std::string> parse_DomainValueList(const core::parse_node& node)
  {
    return parse_vector<std::string>(node, "QuotedString", [&](const core::parse_node& node) { return parse_QuotedString(node); });
  }

  void parse_Parameter(const core::parse_node& node)
  {
    builder.add_parameter(parse_ParameterName(node.child(0)), parse_DomainCardinality(node.child(1)), parse_SortExpr(node.child(2)), parse_DomainValueList(node.child(3)));
  }

  void parse_ParameterList(const core::parse_node& node)
  {
    traverse(node, make_visitor(m_parser.symbol_table(), "Parameter", [&](const core::parse_node& node) { return parse_Parameter(node); }));
  }

  void parse_State(const core::parse_node& node)
  {
    std::vector<std::string> v = parse_NumberList(node.child(0));
    std::vector<std::size_t> result;
    if (v.size() != builder.parameters.size())
    {
      throw mcrl2::runtime_error("parse_State: wrong number of elements");
    }
    for (auto i = v.begin(); i != v.end(); ++i)
    {
      if (builder.parameters[i - v.begin()].cardinality() != 0)
      {
        result.push_back(utilities::parse_natural_number(*i));
      }
    }
    builder.add_state(result);
  }

  void parse_StateList(const core::parse_node& node)
  {
    traverse(node, make_visitor(m_parser.symbol_table(), "State", [&](const core::parse_node& node) { return parse_State(node); }));
  }

  void parse_Transition(const core::parse_node& node)
  {
    std::vector<probabilistic_state> target = parse_Target(node.child(1));
    assert(target.size() == 1);
    builder.add_transition(parse_Source(node.child(0)), target.front().number, parse_Label(node.child(2)));
  }

  void parse_TransitionList(const core::parse_node& node)
  {
    traverse(node, make_visitor(m_parser.symbol_table(), "Transition", [&](const core::parse_node& node) { return parse_Transition(node); }));
  }

  std::string parse_Source(const core::parse_node& node)
  {
    return parse_Number(node.child(0));
  }

  std::string parse_Label(const core::parse_node& node)
  {
    return parse_QuotedString(node.child(0));
  }

  std::pair<std::string, std::string> parse_Distribution(const core::parse_node& node)
  {
    return std::make_pair(parse_Number(node.child(1)), parse_Number(node.child(3)));
  }

  probabilistic_state parse_ProbabilisticStateElement(const core::parse_node& node)
  {
    return probabilistic_state(parse_Number(node.child(0)), parse_Distribution(node.child(1)));
  }

  std::vector<probabilistic_state> parse_ProbabilisticState(const core::parse_node& node)
  {
    std::vector<probabilistic_state> result;
    if (node.child(0) && node.child(0).child(0))
    {
      result = parse_vector<probabilistic_state>(node.child(0).child(0), "ProbabilisticStateElement", [&](const core::parse_node& node) { return parse_ProbabilisticStateElement(node); });
    }
    std::string number = parse_Number(node.child(1));
    std::pair<std::string, std::string> distribution;
    if (node.child(2) && node.child(2).child(0))
    {
      distribution = parse_Distribution(node.child(2).child(0));
    }
    result.push_back(probabilistic_state(number, distribution));
    return result;
  }

  std::vector<probabilistic_state> parse_InitialState(const core::parse_node& node)
  {
    return parse_ProbabilisticState(node.child(0));
  }

  std::vector<probabilistic_state> parse_InitialStateSection(const core::parse_node& node)
  {
    return parse_InitialState(node.child(1));
  }

  std::vector<probabilistic_state> parse_Target(const core::parse_node& node)
  {
    return parse_ProbabilisticState(node.child(0));
  }

  void parse_FSM(const core::parse_node& node)
  {
    builder.start();

    // parse parameters
    parse_ParameterList(node.child(0));

    builder.write_parameters();

    // parse states
    parse_StateList(node.child(2));

    // parse transitions
    parse_TransitionList(node.child(4));

    if (node.child(5) && node.child(5).child(0))
    {
      std::vector<probabilistic_state> init = parse_InitialStateSection(node.child(5).child(0));
    }

    builder.finish();
  }
};

} // namespace detail

inline
void parse_fsm_specification_dparser(const std::string& text, probabilistic_lts_fsm_t& result)
{
  core::parser p(parser_tables_fsm);
  unsigned int start_symbol_index = p.start_symbol_index("FSM");
  bool partial_parses = false;
  core::parse_node node = p.parse(text, start_symbol_index, partial_parses);
  detail::fsm_actions(p, result).parse_FSM(node);
  p.destroy_parse_node(node);
}

inline
void parse_fsm_specification_dparser(std::istream& in, probabilistic_lts_fsm_t& result)
{
  std::string text = utilities::read_text(in);
  parse_fsm_specification_dparser(text, result);
}

} // namespace lts

} // namespace mcrl2

#endif // MCRL2_LTS_PARSE_FSM_SPECIFICATION_DPARSER_H
