// Author(s): Wieger Wesselink
// Copyright: see the accompanying file COPYING or copy at
// https://github.com/mCRL2org/mCRL2/blob/master/COPYING
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
/// \file fsm-syntax.g
/// \brief dparser grammar of the FSM format

${declare longest_match}

//--- FSM syntax

FSM : ParameterList Separator StateList Separator TransitionList (Separator InitialState)? ;

Separator: '---' EOLN ;

ParameterList : Parameter* ;

Parameter : ParameterName DomainCardinality SortExpr? DomainValueList EOLN ;

ParameterName: Id ;

DomainCardinality : '(' Number  ')' ;

SortExpr: "[a-zA-Z_][a-zA-Z0-9_'@#\-> \t=,\\(\\):]*" $term -1 ;

DomainValueList : DomainValue* ;

DomainValue: QuotedString ;

StateList: State* ;

State: Number* EOLN ;

TransitionList: Transition* ;

Transition: Source Target Label EOLN ;

Distribution: '[' Number '/' Number ']' ;

ProbabilisticStateElement : Number Distribution ;

ProbabilisticState : ProbabilisticStateElement* Number Distribution? ;

InitialState : ProbabilisticState ;

Source: Number ;

Target: ProbabilisticState ;

Label: QuotedString ;

//--- Identifiers

QuotedString: "\"[^\"]*\"" $term -1 ;

Id: "[a-zA-Z_][a-zA-Z0-9_'@]*" $term -1 ;

Number: "0|([1-9][0-9]*)" $term -1 ;

//--- Whitespace

EOLN: "\r?\n" ;

whitespace: "[ \t]*" ;
