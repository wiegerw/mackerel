// Author(s): Wieger Wesselink
// Copyright: see the accompanying file COPYING or copy at
// https://svn.win.tue.nl/trac/MCRL2/browser/trunk/COPYING
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
/// \file mcrl2/lts/parse.h
/// \brief A simple straighforward parser for .fsm files.

#ifndef MCRL2_LTS_PARSE_H
#define MCRL2_LTS_PARSE_H

#include <boost/spirit/home/x3.hpp>
#include <iostream>
#include <map>
#include <string>
#include "mcrl2/lts/lts.h"
#include "mcrl2/utilities/exception.h"
#include "mcrl2/utilities/text_utility.h"

namespace mcrl2 {

namespace lts {

namespace detail {

template <typename Iterator>
labeled_transition_system parse_lts(Iterator first, Iterator last)
{
  namespace x3 = boost::spirit::x3;
  namespace ascii = boost::spirit::x3::ascii;

  using x3::_attr;
  using x3::char_;
  using x3::int_;
  using x3::double_;
  using x3::lexeme;
  using x3::lit;
  using x3::ulong_;
  using ascii::space;

  std::map<std::string, std::size_t> label_map;
  labeled_transition_system result;

  std::size_t from;
  std::size_t label;
  std::size_t to;

  auto const quoted_string = lexeme['"' >> +(char_ - '"') >> '"'];

  auto assign_initial_state = [&](auto& ctx)
    {
      result.initial_state = _attr(ctx);
    };

  auto assign_number_of_transitions = [&](auto& ctx)
    {
      std::size_t number_of_transitions = _attr(ctx);
      result.transitions.reserve(number_of_transitions);
    };

  auto assign_number_of_states = [&](auto& ctx)
    {
      result.number_of_states = _attr(ctx);
    };

  auto assign_from = [&](auto& ctx)
    {
      from  = _attr(ctx);
    };

  auto assign_label = [&](auto& ctx)
    {
      std::string name = _attr(ctx);
      auto i = label_map.find(name);
      if (i == label_map.end())
      {
        std::size_t n = label_map.size();
        label_map[name] = n;
        label = n;
        result.action_labels.push_back(name);
      }
      else
      {
        label = i->second;
      }
    };

  auto assign_to = [&](auto& ctx)
    {
      to = _attr(ctx);
      result.transitions.emplace_back(from, label, to);
    };

  bool r = x3::phrase_parse(first, last,

      //  Begin grammar
      (
        // header
        lit("des")
        >> '('
        >> ulong_              [assign_initial_state]
        >> ','
        >> ulong_              [assign_number_of_transitions]
        >> ','
        >> ulong_              [assign_number_of_states]
        >> ')'

        // transitions
        >> *(
              '('
              >> ulong_        [assign_from]
              >> ','
              >> quoted_string [assign_label]
              >> ','
              >> ulong_        [assign_to]
              >> ')'
            )
      )
      ,
      //  End grammar

      space);

  if (!r)
  {
    throw mcrl2::runtime_error("parsing failed");
  }

  if (first != last)
  {
    throw mcrl2::runtime_error("did not get a full match");
  }

  return result;
}

} // namespace detail

inline
labeled_transition_system parse_lts(const std::string& text)
{
  return detail::parse_lts(text.begin(), text.end());
}

} // namespace lts

} // namespace mcrl2

#endif // MCRL2_LTS_PARSE_H
