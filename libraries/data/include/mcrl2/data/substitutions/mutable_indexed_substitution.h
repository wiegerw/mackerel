// Author(s): Jeroen Keiren
// Copyright: see the accompanying file COPYING or copy at
// https://svn.win.tue.nl/trac/MCRL2/browser/trunk/COPYING
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
/// \file mcrl2/data/substitutions/mutable_indexed_substitution.h
/// \brief add your file description here.

// The code below contains an experiment to replace the classical mutable_indexed_substitution
// by a std::unordered_map from variables to expressions. This is up to 1.5 times 
// slower in most state space generations when there are not too many variables, which is
// typical for state space generation without complex sum operations or quantifiers.
// When a large number of variables exist, generally generated as fresh variables,
// std::unordered_map can perform much better, leading to the time to generate a 
// state space with a factor 2. 
#define MCRL2_USE_CLASSIC_MUTABLE_INDEXED_SUBSTITUTION


#ifndef MCRL2_DATA_SUBSTITUTIONS_MUTABLE_INDEXED_SUBSTITUTION_H
#define MCRL2_DATA_SUBSTITUTIONS_MUTABLE_INDEXED_SUBSTITUTION_H

#include "mcrl2/data/is_simple_substitution.h"
#include "mcrl2/data/undefined.h"
#include "mcrl2/utilities/exception.h"
#include <functional>
#include <iostream>
#include <sstream>
#include <string>

namespace mcrl2 {

namespace data {

#ifdef MCRL2_USE_CLASSIC_MUTABLE_INDEXED_SUBSTITUTION

/// \brief Generic substitution function.
/// \details This substitution assumes a function variable -> std::size_t, that, for
///          each variable gives a unique index. The substitutions are stored
///          internally as a vector, mapping std::size_t to expression.
///          Provided that, given a variable, its index can be computed in O(1)
///          time, insertion is O(1) amortized, and lookup is O(1).
///          Memory required is O(n) where n is the largest index used.
template <typename VariableType = data::variable, typename ExpressionType = data_expression >
class mutable_indexed_substitution : public std::unary_function<VariableType, ExpressionType>
{
protected:
  typedef std::pair <VariableType, ExpressionType> substitution_type;
  /// \brief Internal storage for substitutions.
  /// Required to be a container with random access through [] operator.
  /// It is essential to store the variable also in the container, as it might be that
  /// this variable is not used anywhere although it has a valid assignment. This happens
  /// for instance when the assignment is already parsed, while the expression to which it
  /// needs to be applied must still be parsed. 
  std::vector < substitution_type > m_container;
  std::vector <std::size_t> m_index_table;
  std::stack<std::size_t> m_free_positions;
  bool m_variables_in_rhs_set_is_defined;
  std::set<variable> m_variables_in_rhs;

public:

  /// \brief Type of variables
  typedef VariableType variable_type;

  /// \brief Type of expressions
  typedef ExpressionType expression_type;

  /// \brief Default constructor
  mutable_indexed_substitution()
    : m_variables_in_rhs_set_is_defined(false)
  {
  }

  /// \brief Wrapper class for internal storage and substitution updates using operator()
  struct assignment
  {
    const variable_type& m_variable;
    std::vector < substitution_type >& m_container;
    std::vector <std::size_t>& m_index_table;
    std::stack<std::size_t>& m_free_positions;
    bool m_variables_in_rhs_set_is_defined;
    std::set<variable>& m_variables_in_rhs;


    /// \brief Constructor.
    /// \param[in] v a variable.
    /// \param[in] c a container of expressions.
    /// \param[in] table a table of indices.
    /// \param[in] fp a stack of free positions in \a table.
    /// \param[in] b Indication that the variables in \a vars are defined.
    /// \param[in] vars Variables in the rhs of the assignments. 
    assignment(const variable_type& v, 
               std::vector < substitution_type >& c, 
               std::vector <std::size_t>& table, 
               std::stack<std::size_t>& fp,
               const bool b, 
               std::set<variable>& vars) :
      m_variable(v),
      m_container(c),
      m_index_table(table),
      m_free_positions(fp),
      m_variables_in_rhs_set_is_defined(b),
      m_variables_in_rhs(vars)
    { }

    /// \brief Actual assignment
    void operator=(const expression_type& e)
    {
      mCRL2log(log::debug2, "substitutions") << "Setting " << m_variable << " := " << e << std::endl;
      assert(e.defined());

      std::size_t i = core::index_traits<data::variable, data::variable_key_type, 2>::index(m_variable);

      if (e != m_variable)
      {
        // Set a new variable;
        if (m_variables_in_rhs_set_is_defined)
        {
          std::set<variable_type> s=find_free_variables(e);
          m_variables_in_rhs.insert(s.begin(),s.end());
        }

        // Resize container if needed
        if (i >= m_index_table.size())
        {
          m_index_table.resize(i+1, std::size_t(-1));
        }

        std::size_t j=m_index_table[i];
        assert(j==std::size_t(-1) || j<m_container.size());
        if (j==std::size_t(-1))
        {
          // The variable was not assigned.
          if (m_free_positions.empty())
          {
            m_index_table[i]=m_container.size();
            m_container.push_back(substitution_type(m_variable,e));
          }
          else
          {
            j=m_free_positions.top();
            m_index_table[i]=j;
            m_container[j]=substitution_type(m_variable,e);
            m_free_positions.pop();
          }
        }
        else
        {
          // The variable was already assigned. Replace the assignment.
          // Note that we do not remove the variables in the term that is replaced.
          m_container[j]=substitution_type(m_variable,e);
        }
      }
      else
      {
        // Indicate that the current variable is free; postpone deleting the
        // actual value assigned to the variable.
        // Note that we do not remove variables in variables_in_rhs;
        if (i<m_index_table.size())
        {
          std::size_t j=m_index_table[i];
          if (j!=std::size_t(-1))
          {
            m_free_positions.push(j);
            m_index_table[i]=std::size_t(-1);
          }
        }
      }
    }
  };

  /// \brief Application operator; applies substitution to v.
  /// \detail This must deliver an expression, and not a reference
  ///         to an expression, as the expressions are stored in 
  ///         a vector that can be resized and moved. 
  const expression_type operator()(const variable_type& v) const
  {
    const std::size_t i = core::index_traits<data::variable, data::variable_key_type, 2>::index(v);
    if (i < m_index_table.size())
    {
      const std::size_t j = m_index_table[i];
      if (j!=std::size_t(-1))
      {
        // the variable has an assigned value.
        assert(j<m_container.size());
        return m_container[j].second;
      }
    }
    // no value assigned to v;
    return v;
  }

  /// \brief Index operator.
  assignment operator[](variable_type const& v)
  {
    return assignment(v, m_container,m_index_table,m_free_positions,m_variables_in_rhs_set_is_defined,m_variables_in_rhs);
  }

  /// \brief Clear substitutions.
  void clear()
  {
    m_index_table.clear();
    m_container.clear();
    m_free_positions=std::stack<std::size_t>();
    m_variables_in_rhs_set_is_defined=false;
    m_variables_in_rhs.clear();
  }

  /// \brief Compare substitutions
  template <typename Substitution>
  bool operator==(const Substitution&) const
  {
    return false;
  }

  /// \brief Provides a set of variables that occur in the right hand sides of the assignments.
  const std::set<variable>& variables_in_rhs()
  {
    if (!m_variables_in_rhs_set_is_defined)
    {
      for(std::vector<std::size_t> ::const_iterator i=m_index_table.begin(); i != m_index_table.end(); ++i)
      {
        if (*i != std::size_t(-1))
        {
          std::set<variable_type> s=find_free_variables(m_container[*i]);
          m_variables_in_rhs.insert(s.begin(),s.end());
        }
      }
      m_variables_in_rhs_set_is_defined=true;
    }
    return m_variables_in_rhs;
  }

  /// \brief Returns true if the substitution is empty
  bool empty()
  {
    assert(m_container.size()>=m_free_positions.size());
    return m_container.size()==m_free_positions.size();
  }

public:
  /// \brief string representation of the substitution. N.B. This is an expensive operation!
  std::string to_string() const
  {
    std::stringstream result;
    bool first = true;
    result << "[";
    for (std::size_t i = 0; i < m_index_table.size(); ++i)
    {
      const std::size_t j=m_index_table[i];
      if (j != std::size_t(-1))
      {
        if (first)
        {
          first = false;
        }
        else
        {
          result << "; ";
        }
        
        result << m_container.at(j).first << " := " << m_container.at(j).second;
      }
    }
    result << "]";
    return result.str();
  }

};
#else // ifdef MCRL2_USE_CLASSICAL_MUTABLE_SUBSTITUTION 

/// \brief Generic substitution function.
/// \details This substitution assumes a function variable -> std::size_t, that, for
///          each variable gives a unique index. The substitutions are stored
///          internally as a vector, mapping std::size_t to expression.
///          Provided that, given a variable, its index can be computed in O(1)
///          time, insertion is O(1) amortized, and lookup is O(1).
///          Memory required is O(n) where n is the largest index used.
template <typename VariableType = data::variable, typename ExpressionType = data_expression >
class mutable_indexed_substitution : public std::unary_function<VariableType, ExpressionType>
{
protected:
  typedef std::pair <VariableType, ExpressionType> substitution_type;
  /// \brief Internal storage for substitutions.
  /// Required to be a container with random access through [] operator.
  /// It is essential to store the variable also in the container, as it might be that
  /// this variable is not used anywhere although it has a valid assignment. This happens
  /// for instance when the assignment is already parsed, while the expression to which it
  /// needs to be applied must still be parsed. 

  std::unordered_map < VariableType, ExpressionType > m_substitution;
  bool m_variables_in_rhs_set_is_defined;
  std::set<variable> m_variables_in_rhs;

public:

  /// \brief Type of variables
  typedef VariableType variable_type;

  /// \brief Type of expressions
  typedef ExpressionType expression_type;

  /// \brief Default constructor
  mutable_indexed_substitution()
    : m_variables_in_rhs_set_is_defined(false)
  {
  }

  /// \brief Wrapper class for internal storage and substitution updates using operator()
  struct assignment
  {
    const variable_type& m_variable;
    mutable_indexed_substitution < VariableType, ExpressionType >& m_super;

    /// \brief Constructor.
    /// \param[in] v a variable.
    /// \param[in] super The surrounding mutable_indexed_substitution.
    assignment(const variable_type& v, 
               mutable_indexed_substitution < VariableType, ExpressionType >& super) :
      m_variable(v),
      m_super(super)
    { }

    /// \brief Actual assignment
    void operator=(const expression_type& e)
    {
      if (e != m_variable)
      {
        // Set a new variable;
        if (m_super.m_variables_in_rhs_set_is_defined)
        {
          std::set<variable_type> s=find_free_variables(e);
          m_super.m_variables_in_rhs.insert(s.begin(),s.end());
        }

        m_super.m_substitution[m_variable]=e;
      }
      else
      {
        // Indicate that the current variable is not assigned.
        m_super.m_substitution.erase(m_variable);
      }
    }
  };

  /// \brief Application operator; applies substitution to v.
  /// \detail This must deliver an expression, and not a reference
  ///         to an expression, as the expressions are stored in 
  ///         a vector that can be resized and moved. 
  const expression_type operator()(const variable_type& v) const
  {
    const typename std::unordered_map < VariableType, ExpressionType >::const_iterator i=m_substitution.find(v);
    if (i!=m_substitution.end())
    {
      // Found.
      return i->second;
    }
    // Else not found. 
    return v;
  }

  /// \brief Index operator.
  assignment operator[](const variable_type& v)
  {
    return assignment(v, *this);
  }

  /// \brief Clear substitutions.
  void clear()
  {
    m_substitution.clear();
    m_variables_in_rhs_set_is_defined=false;
    m_variables_in_rhs.clear();
  }

  /// \brief Compare substitutions
  template <typename Substitution>
  bool operator==(const Substitution&) const
  {
    return false;
  }

  /// \brief Provides a set of variables that occur in the right hand sides of the assignments.
  const std::set<variable>& variables_in_rhs()
  {
    if (!m_variables_in_rhs_set_is_defined)
    {
      for(typename std::unordered_map < VariableType, ExpressionType >::const_iterator i=m_substitution.begin(); i != m_substitution.end(); ++i)
      {
        std::set<variable_type> s=find_free_variables(i->second);
        m_variables_in_rhs.insert(s.begin(),s.end());
      }
      m_variables_in_rhs_set_is_defined=true;
    }
    return m_variables_in_rhs;
  }

  /// \brief Returns true if the substitution is empty
  bool empty()
  {
    return m_substitution.empty();
  }

public:
  /// \brief string representation of the substitution. N.B. This is an expensive operation!
  std::string to_string() const
  {
    std::stringstream result;
    bool first = true;
    result << "[";
    for (typename std::unordered_map < VariableType, ExpressionType >::const_iterator i=m_substitution.begin(); i != m_substitution.end(); ++i)
    {
      if (first)
      {
        first = false;
      }
      else
      {
        result << "; ";
      }
      result << i->first << " := " << i->second;
    }
    result << "]";
    return result.str();
  }

};
#endif // ifdef MCRL2_USE_CLASSICAL_MUTABLE_SUBSTITUTION 

template <typename VariableType, typename ExpressionType>
std::ostream& operator<<(std::ostream& out, const mutable_indexed_substitution<VariableType, ExpressionType>& sigma)
{
  return out << sigma.to_string();
}

} // namespace data

} // namespace mcrl2

#endif // MCRL2_DATA_SUBSTITUTIONS_MUTABLE_INDEXED_SUBSTITUTION_H
