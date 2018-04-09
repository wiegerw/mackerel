// Author(s): Jeroen Keiren, Jeroen van der Wulp, Jan Friso Groote
// Copyright: see the accompanying file COPYING or copy at
// https://svn.win.tue.nl/trac/MCRL2/browser/trunk/COPYING
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
/// \file mcrl2/data/data_specification.h
/// \brief The class data_specification.

#ifndef MCRL2_DATA_DATA_SPECIFICATION_H
#define MCRL2_DATA_DATA_SPECIFICATION_H

#include <algorithm>
#include <functional>
#include <vector>

#include "mcrl2/atermpp/aterm_appl.h"

#include "mcrl2/core/load_aterm.h"

// utilities
#include "mcrl2/data/detail/data_functional.h"

// sort_specification
#include "mcrl2/data/sort_specification.h"

// data expressions
#include "mcrl2/data/data_equation.h"
#include "mcrl2/data/function_update.h"
#include "mcrl2/data/detail/io.h"


namespace mcrl2
{

namespace data
{

// template function overloads
data_expression normalize_sorts(const data_expression& x, const data::sort_specification& sortspec);
variable_list normalize_sorts(const variable_list& x, const data::sort_specification& sortspec);
data::data_equation normalize_sorts(const data::data_equation& x, const data::sort_specification& sortspec);
data_equation_list normalize_sorts(const data_equation_list& x, const data::sort_specification& sortspec);
void normalize_sorts(data_equation_vector& x, const data::sort_specification& sortspec);

// prototype
class data_specification;

std::string pp(const data::data_specification& x);

/// \cond INTERNAL_DOCS
namespace detail
{
atermpp::aterm_appl data_specification_to_aterm(const data_specification&);

} // namespace detail
/// \endcond

/// \brief Test for a data specification expression
/// \param x A term
/// \return True if \a x is a data specification expression
inline
bool is_data_specification(const atermpp::aterm_appl& x)
{
  return x.function() == core::detail::function_symbols::DataSpec;
}

/// \brief data specification.

class data_specification: public sort_specification
{
  private:

    /// \cond INTERNAL_DOCS
    /// \brief Cached constructors by target sort
    struct target_sort_to_function_map
    {
      bool _outdated;
      std::map<sort_expression, std::vector<function_symbol> > _mapping;

      target_sort_to_function_map()
        : _outdated(true)
      {}

      /// \brief Groups functions according to their target sorts.
      /// \param [in,out] c container in which the functions are stored grouped by target sort.
      /// \param [in] functions a container with function symbols
      template <typename Container>
      void group_functions_by_target_sort(std::map<sort_expression, std::vector<function_symbol> >& c, const Container& functions)
      {
        for (typename Container::const_iterator i = functions.begin(); i != functions.end(); ++i)
        {
          sort_expression index_sort(i->sort().target_sort());
          if(c.find(index_sort) == c.end() || std::find(c[index_sort].begin(), c[index_sort].end(), *i) == c[index_sort].end())
          {
            c[index_sort].push_back(*i);
          }
        }
      }

      template <typename FunctionContainer>
      void reset(const FunctionContainer& c)
      {
        if(_outdated)
        {
          _mapping.clear();
          group_functions_by_target_sort(_mapping, c);
          _outdated = false;
        }
      }

      void expire()
      {
        _outdated = true;
      }

      std::map<sort_expression, std::vector<function_symbol> >&mapping()
      {
        assert(!_outdated);
        return _mapping;
      }
    };

    friend atermpp::aterm_appl detail::data_specification_to_aterm(const data_specification&);

    ///\brief Builds a specification from aterm
    void build_from_aterm(const atermpp::aterm_appl& term);
    /// \endcond


  protected:

    /// \brief A mapping of sort expressions to the constructors corresponding to that sort.
    function_symbol_vector m_user_defined_constructors;

    /// \brief The mappings of the specification.
    function_symbol_vector m_user_defined_mappings;

    /// \brief The equations of the specification.
    std::vector< data_equation > m_user_defined_equations;

    /// \brief Set containing all constructors, including the system defined ones.
    /// The types in these constructors are normalised.
    mutable function_symbol_vector m_normalised_constructors;

    /// \brief Cache normalised constructors grouped by target sort.
    mutable target_sort_to_function_map m_grouped_normalised_constructors;

    /// \brief Set containing system defined all mappings, including the system defined ones.
    /// The types in these mappings are normalised.
    mutable function_symbol_vector m_normalised_mappings;

    /// \brief Cache normalised mappings grouped by target sort.
    mutable target_sort_to_function_map m_grouped_normalised_mappings;
    //
    /// \brief Table containing all equations, including the system defined ones.
    ///        The sorts in these equations are normalised.
    mutable data_equation_vector m_normalised_equations;

    void data_is_not_necessarily_normalised_anymore() const
    {
      m_normalised_data_is_up_to_date=false;
    }

  protected:

    /// \brief Adds a constructor to this specification, and marks it as
    ///        system defined.
    ///
    /// \param[in] f A function symbol.
    /// \pre f does not yet occur in this specification.
    /// \post is_system_defined(f) == true
    /// \note this operation does not invalidate iterators of constructors_const_range
    inline
    void add_normalised_constructor(const function_symbol& f) const
    {
      function_symbol g(normalize_sorts(f, *this));
      if (std::find(m_normalised_constructors.begin(),m_normalised_constructors.end(),g)==m_normalised_constructors.end()) // not found
      {
        m_normalised_constructors.push_back(g);
      }
    }

    /// \brief Adds a mapping to this specification, and marks it as system
    ///        defined.
    ///
    /// \param[in] f A function symbol.
    /// \pre f does not yet occur in this specification.
    /// \post is_system_defined(f) == true
    /// \note this operation does not invalidate iterators of mappings_const_range
    void add_normalised_mapping(const function_symbol& f) const
    {
      function_symbol g(normalize_sorts(f, *this));
      if (std::find(m_normalised_mappings.begin(),m_normalised_mappings.end(),g)==m_normalised_mappings.end()) // not found
      {
        m_normalised_mappings.push_back(g);
      }
    }

    /// \brief Adds an equation to this specification, and marks it as system
    ///        defined.
    ///
    /// \param[in] e An equation.
    /// \pre e does not yet occur in this specification.
    /// \post is_system_defined(f) == true
    /// \note this operation does not invalidate iterators of equations_const_range
    void add_normalised_equation(const data_equation& e) const
    {
      m_normalised_equations.push_back(normalize_sorts(e,*this));
    }

    template < class Iterator >
    void add_normalised_constructors(Iterator begin, Iterator end) const
    {
      for(Iterator i=begin; i!=end; ++i)
      {
        data_specification::add_normalised_constructor(*i);
      }
    }

    template < class Iterator >
    void add_normalised_mappings(Iterator begin, Iterator end) const
    {
      for(Iterator i=begin; i!=end; ++i)
      {
        data_specification::add_normalised_mapping(*i);
      }
    }

    template < class Iterator >
    void add_normalised_equations(Iterator begin, Iterator end) const
    {
      for(Iterator i=begin; i!=end; ++i)
      {
        data_specification::add_normalised_equation(*i);
      }
    }

    /// \brief Adds constructors, mappings and equations for a structured sort
    ///        to this specification, and marks them as system defined.
    ///
    /// \param[in] sort A sort expression that is representing the structured sort.
    void insert_mappings_constructors_for_structured_sort(
                       const structured_sort& sort,
                       std::set < function_symbol >& constructors,
                       std::set < function_symbol >& mappings,
                       std::set < data_equation >& equations,
                       const bool skip_equations) const
    {
      const structured_sort& s_sort(sort);
      function_symbol_vector f(s_sort.constructor_functions(sort));
      constructors.insert(f.begin(),f.end());
      f = s_sort.projection_functions(sort);
      mappings.insert(f.begin(),f.end());
      f = s_sort.recogniser_functions(sort);
      mappings.insert(f.begin(),f.end());
      f = s_sort.comparison_functions(sort);
      mappings.insert(f.begin(),f.end());

      if (!skip_equations)
      {
        data_equation_vector e(s_sort.constructor_equations(sort));
        equations.insert(e.begin(),e.end());
        e = s_sort.projection_equations(sort);
        equations.insert(e.begin(),e.end());
        e = s_sort.recogniser_equations(sort);
        equations.insert(e.begin(),e.end());
        e = s_sort.comparison_equations(sort);
        equations.insert(e.begin(),e.end());
      }
    }

    void add_standard_mappings_and_equations(
                       const sort_expression& sort,
                       std::set < function_symbol >& mappings,
                       std::set < data_equation >& equations,
                       const bool skip_equations) const
    {
      function_symbol_vector f(standard_generate_functions_code(sort));
      mappings.insert(f.begin(), f.end());

      if (!skip_equations)
      {
        const data_equation_vector eq(standard_generate_equations_code(sort));
        equations.insert(eq.begin(), eq.end());
      }
    }

  public:

    ///\brief Default constructor. Generate a data specification that contains
    ///       only booleans and positive numbers.
    data_specification()
    {
    }

    ///\brief Constructor from an aterm.
    /// \param[in] t a term adhering to the internal format.
    data_specification(const atermpp::aterm_appl& t)
    {
      build_from_aterm(t);
    }

    /// \brief Indicates that the data specification is type checked.
    /// \details This builds up internal data structures and allows
    ///  access to the data specification using all the utility functions.
    void declare_data_specification_to_be_type_checked()
    {
      // This is now an empty deprecated method.
    }

    /// \brief Gets all constructors including those that are system defined.
    /// \details The time complexity is the same as for sorts().
    /// \return All constructors in this specification, including those for
    /// structured sorts.
    inline
    const function_symbol_vector& constructors() const
    {
      normalise_data_specification_if_required();
      return m_normalised_constructors;
    }

    /// \brief Gets the constructors defined by the user, excluding those that
    /// are system defined.
    /// \details The time complexity for this operation is constant.
    inline
    const function_symbol_vector& user_defined_constructors() const
    {
      return m_user_defined_constructors;
    }

    /// \brief Gets all constructors of a sort including those that are system defined.
    ///
    /// \details The time complexity is the same as for sorts().
    /// \param[in] s A sort expression.
    /// \return The constructors for sort s in this specification.
    inline
    const function_symbol_vector& constructors(const sort_expression& s) const
    {
      normalise_data_specification_if_required();
      m_grouped_normalised_constructors.reset(constructors());
      return m_grouped_normalised_constructors.mapping()[normalize_sorts(s,*this)];
    }

    /// \brief Gets all mappings in this specification including those that are system defined.
    ///
    /// \brief The time complexity is the same as for sorts().
    /// \return All mappings in this specification, including recognisers and
    /// projection functions from structured sorts.
    inline
    const function_symbol_vector& mappings() const
    {
      normalise_data_specification_if_required();
      return m_normalised_mappings;
    }

    /// \brief Gets all user defined mappings in this specification.
    ///
    /// \brief The time complexity is constant.
    /// \return All mappings in this specification, including recognisers and
    /// projection functions from structured sorts.
    inline
    const function_symbol_vector& user_defined_mappings() const
    {
      return m_user_defined_mappings;
    }

    /// \brief Gets all mappings of a sort including those that are system defined
    ///
    /// \param[in] s A sort expression.
    /// \return All mappings in this specification, for which s occurs as a
    /// right-hand side of the mapping's sort.
    inline
    const function_symbol_vector& mappings(const sort_expression& s) const
    {
      normalise_data_specification_if_required();
      m_grouped_normalised_mappings.reset(mappings());
      return m_grouped_normalised_mappings.mapping()[normalize_sorts(s, *this)];
    }

    /// \brief Gets all equations in this specification including those that are system defined
    ///
    /// \details The time complexity of this operation is the same as that for sort().
    /// \return All equations in this specification, including those for
    ///  structured sorts.
    inline
    const data_equation_vector& equations() const
    {
      normalise_data_specification_if_required();
      return m_normalised_equations;
    }

    /// \brief Gets all user defined equations.
    ///
    /// \details The time complexity of this operation is constant.
    /// \return All equations in this specification, including those for
    ///  structured sorts.
    inline
    const data_equation_vector& user_defined_equations() const
    {
      return m_user_defined_equations;
    }

    /// \brief Adds a constructor to this specification
    ///
    /// \param[in] f A function symbol.
    /// \pre a mapping f does not yet occur in this specification.
    /// \note this operation does not invalidate iterators of constructors_const_range
    void add_constructor(const function_symbol& f)
    {
      if (std::find(m_user_defined_constructors.begin(),m_user_defined_constructors.end(),f)==m_user_defined_constructors.end())
      {
        m_user_defined_constructors.push_back(f);
        import_system_defined_sort(f.sort());
        data_is_not_necessarily_normalised_anymore();
      }
    }

    /// \brief Adds a mapping to this specification
    ///
    /// \param[in] f A function symbol.
    /// \pre a constructor f does not yet occur in this specification.
    /// \note this operation does not invalidate iterators of mappings_const_range
    void add_mapping(const function_symbol& f)
    {
      if (std::find(m_user_defined_mappings.begin(),m_user_defined_mappings.end(),f)==m_user_defined_mappings.end())
      {
        m_user_defined_mappings.push_back(f);
        import_system_defined_sort(f.sort());
        data_is_not_necessarily_normalised_anymore();
      }
    }

    /// \brief Adds an equation to this specification
    ///
    /// \param[in] e An equation.
    /// \pre e does not yet occur in this specification.
    /// \note this operation does not invalidate iterators of equations_const_range
    void add_equation(const data_equation& e)
    {
      import_system_defined_sorts(find_sort_expressions(e));
      // m_user_defined_equations.push_back(data::translate_user_notation(e));
      m_user_defined_equations.push_back(e);
      data_is_not_necessarily_normalised_anymore();
    }

  private:

    ///\brief Puts the constructors, functions and equations in normalised form in
    ///       de data type.
    ///\details See the function normalise_sorts on arbitrary objects for a more detailed description.
    /// All sorts in the constructors, mappings and equations are normalised.
    void add_data_types_for_sorts() const
    {
      // Normalise the sorts of the constructors.
      m_normalised_constructors.clear();
      m_normalised_mappings.clear();
      m_normalised_equations.clear();

      for (const sort_expression& sort: sorts())
      {
        import_data_type_for_system_defined_sort(sort);
      }

      for (const alias& a: user_defined_aliases())
      {
        import_data_type_for_system_defined_sort(a.reference());
      }


      // Normalise the constructors.
      for (const function_symbol& f: m_user_defined_constructors)
      {
        add_normalised_constructor(f);
      }

      // Normalise the sorts of the mappings.
      for (const function_symbol& f: m_user_defined_mappings)
      {
        add_normalised_mapping(f);
      }

      // Normalise the sorts of the expressions and variables in equations.
      for (const data_equation& eq: m_user_defined_equations)
      {
        add_normalised_equation(data::translate_user_notation(eq));
      }
    }

    /// \brief
    /// \details
    void normalise_data_specification_if_required() const
    {
      if (!m_normalised_data_is_up_to_date)
      {
        m_normalised_data_is_up_to_date=true;
        m_grouped_normalised_constructors.expire();
        m_grouped_normalised_mappings.expire();
        add_data_types_for_sorts();
      }
    }

    ///\brief Adds the system defined sorts to the sets with constructors, mappings, and equations for
    //        a given sort. If the boolean skip_equations is true, no equations are added.

    void find_associated_system_defined_data_types_for_a_sort(
                       const sort_expression& sort,
                       std::set < function_symbol >& constructors,
                       std::set < function_symbol >& mappings,
                       std::set < data_equation >& equations,
                       const bool skip_equations=false) const
    {
      // add sorts, constructors, mappings and equations
      if (sort == sort_bool::bool_())
      {
        function_symbol_vector f(sort_bool::bool_generate_constructors_code());
        constructors.insert(f.begin(), f.end());
        f = sort_bool::bool_generate_functions_code();
        mappings.insert(f.begin(), f.end());
        if (!skip_equations)
        {
          data_equation_vector e(sort_bool::bool_generate_equations_code());
          equations.insert(e.begin(),e.end());
        }
      }
      else if (sort == sort_real::real_())
      {
        function_symbol_vector f(sort_real::real_generate_constructors_code());
        constructors.insert(f.begin(),f.end());
        f = sort_real::real_generate_functions_code();
        mappings.insert(f.begin(),f.end());
        if (!skip_equations)
        {
          data_equation_vector e(sort_real::real_generate_equations_code());
          equations.insert(e.begin(),e.end());
        }
      }
      else if (sort == sort_int::int_())
      {
        function_symbol_vector f(sort_int::int_generate_constructors_code());
        constructors.insert(f.begin(),f.end());
        f = sort_int::int_generate_functions_code();
        mappings.insert(f.begin(),f.end());
        if (!skip_equations)
        {
          data_equation_vector e(sort_int::int_generate_equations_code());
          equations.insert(e.begin(),e.end());
        }
      }
      else if (sort == sort_nat::nat())
      {
        function_symbol_vector f(sort_nat::nat_generate_constructors_code());
        constructors.insert(f.begin(),f.end());
        f = sort_nat::nat_generate_functions_code();
        mappings.insert(f.begin(),f.end());
        if (!skip_equations)
        {
          data_equation_vector e(sort_nat::nat_generate_equations_code());
          equations.insert(e.begin(),e.end());
        }
      }
      else if (sort == sort_pos::pos())
      {
        function_symbol_vector f(sort_pos::pos_generate_constructors_code());
        constructors.insert(f.begin(),f.end());
        f = sort_pos::pos_generate_functions_code();
        mappings.insert(f.begin(),f.end());
        if (!skip_equations)
        {
          data_equation_vector e(sort_pos::pos_generate_equations_code());
          equations.insert(e.begin(),e.end());
        }
      }
      else if (is_function_sort(sort))
      {
        const sort_expression& t=function_sort(sort).codomain();
        const sort_expression_list& l=function_sort(sort).domain();
        if (l.size()==1)
        {
          const function_symbol_vector f = function_update_generate_functions_code(l.front(),t);
          mappings.insert(f.begin(),f.end());

          if (!skip_equations)
          {
            data_equation_vector e(function_update_generate_equations_code(l.front(),t));
            equations.insert(e.begin(),e.end());
          }
        }
      }
      else if (is_container_sort(sort))
      {
        sort_expression element_sort(container_sort(sort).element_sort());
        if (sort_list::is_list(sort))
        {
          function_symbol_vector f(sort_list::list_generate_constructors_code(element_sort));
          constructors.insert(f.begin(),f.end());
          f = sort_list::list_generate_functions_code(element_sort);
          mappings.insert(f.begin(),f.end());
          if (!skip_equations)
          {
            data_equation_vector e(sort_list::list_generate_equations_code(element_sort));
            equations.insert(e.begin(),e.end());
          }
        }
        else if (sort_set::is_set(sort))
        {
          sort_expression_list element_sorts;
          element_sorts.push_front(element_sort);
          function_symbol_vector f(sort_set::set_generate_constructors_code(element_sort));
          constructors.insert(f.begin(),f.end());
          f = sort_set::set_generate_functions_code(element_sort);
          mappings.insert(f.begin(),f.end());
          if (!skip_equations)
          {
            data_equation_vector e(sort_set::set_generate_equations_code(element_sort));
            equations.insert(e.begin(),e.end());
          }
        }
        else if (sort_fset::is_fset(sort))
        {
          function_symbol_vector f = sort_fset::fset_generate_constructors_code(element_sort);
          constructors.insert(f.begin(),f.end());
          f = sort_fset::fset_generate_functions_code(element_sort);
          mappings.insert(f.begin(),f.end());
          if (!skip_equations)
          {
            data_equation_vector e = sort_fset::fset_generate_equations_code(element_sort);
            equations.insert(e.begin(),e.end());
          }
        }
        else if (sort_bag::is_bag(sort))
        {
          sort_expression_list element_sorts;
          element_sorts.push_front(element_sort);
          function_symbol_vector f(sort_bag::bag_generate_constructors_code(element_sort));
          constructors.insert(f.begin(),f.end());
          f = sort_bag::bag_generate_functions_code(element_sort);
          mappings.insert(f.begin(),f.end());
          if (!skip_equations)
          {
            data_equation_vector e(sort_bag::bag_generate_equations_code(element_sort));
            equations.insert(e.begin(),e.end());
          }
        }
        else if (sort_fbag::is_fbag(sort))
        {
          function_symbol_vector f = sort_fbag::fbag_generate_constructors_code(element_sort);
          constructors.insert(f.begin(),f.end());
          f = sort_fbag::fbag_generate_functions_code(element_sort);
          mappings.insert(f.begin(),f.end());
          if (!skip_equations)
          {
            data_equation_vector e = sort_fbag::fbag_generate_equations_code(element_sort);
            equations.insert(e.begin(),e.end());
          }
        }
      }
      else if (is_structured_sort(sort))
      {
        insert_mappings_constructors_for_structured_sort(
                        atermpp::down_cast<structured_sort>(sort),
                        constructors, mappings, equations, skip_equations);
      }
      add_standard_mappings_and_equations(sort, mappings, equations, skip_equations);
    }

    ///\brief Adds the system defined sorts in a sequence.
    ///       The second argument is used to check which sorts are added, to prevent
    ///       useless repetitions of additions of sorts.
    /// The function normalise_sorts imports for the given sort_expression sort all sorts, constructors,
    /// mappings and equations that belong to this sort to the `normalised' sets in this
    /// data type. E.g. for the sort Nat of natural numbers, it is required that Pos
    /// (positive numbers) are defined.
    void import_data_type_for_system_defined_sort(const sort_expression& sort) const
    {
      std::set < function_symbol > constructors;
      std::set < function_symbol > mappings;
      std::set < data_equation > equations;
      find_associated_system_defined_data_types_for_a_sort(sort, constructors, mappings, equations);

      // add normalised constructors, mappings and equations
      add_normalised_constructors(constructors.begin(), constructors.end());
      add_normalised_mappings(mappings.begin(), mappings.end());
      add_normalised_equations(equations.begin(), equations.end());
    }

  public:

    /// \brief This function provides a sample of all system defined sorts, constructors and mappings
    ///        that contains at least one specimen of each sort and function symbol. Because types
    ///        can be parameterised not all function symbols for all types are provided.
    /// \details The sorts, constructors and mappings for the following types are added:
    ///            Bool, Pos, Int, Nat, Real, List(Pos), FSet(Pos), FBag(Pos), Set(Pos), Bag(Pos).
    ///       How to deal with struct...
    void get_system_defined_sorts_constructors_and_mappings(
                std::set < sort_expression >& sorts,
                std::set < function_symbol >& constructors,
                std::set <function_symbol >& mappings) const
    {
      sorts.insert(sort_bool::bool_());
      sorts.insert(sort_pos::pos());
      sorts.insert(sort_nat::nat());
      sorts.insert(sort_int::int_());
      sorts.insert(sort_real::real_());
      sorts.insert(sort_list::list(sort_pos::pos()));
      sorts.insert(sort_fset::fset(sort_pos::pos()));
      sorts.insert(sort_set::set_(sort_pos::pos()));
      sorts.insert(sort_fbag::fbag(sort_pos::pos()));
      sorts.insert(sort_bag::bag(sort_pos::pos()));

      std::set < data_equation > dummy_equations;
      for(const sort_expression& s: sorts)
      {
        find_associated_system_defined_data_types_for_a_sort(s, constructors, mappings, dummy_equations, true);
      }
      assert(dummy_equations.size()==0);
    }

    /// \brief Removes constructor from specification.
    ///
    /// Note that this does not remove equations containing the constructor.
    /// \param[in] f A constructor.
    /// \pre f occurs in the specification as constructor.
    /// \post f does not occur as constructor.
    /// \note this operation does not invalidate iterators of constructors_const_range,
    /// only if they point to the element that is removed
    void remove_constructor(const function_symbol& f)
    {
      detail::remove(m_normalised_constructors, normalize_sorts(f,*this));
      detail::remove(m_user_defined_constructors, f);
    }

    /// \brief Removes mapping from specification.
    ///
    /// Note that this does not remove equations in which the mapping occurs.
    /// \param[in] f A function.
    /// \post f does not occur as constructor.
    /// \note this operation does not invalidate iterators of mappings_const_range,
    /// only if they point to the element that is removed
    void remove_mapping(const function_symbol& f)
    {
      detail::remove(m_normalised_mappings, normalize_sorts(f,*this));
      detail::remove(m_user_defined_mappings, f);
    }

    /// \brief Removes equation from specification.
    ///
    /// \param[in] e An equation.
    /// \post e is removed from this specification.
    /// \note this operation does not invalidate iterators of equations_const_range,
    /// only if they point to the element that is removed
    void remove_equation(const data_equation& e)
    {
      const data_equation e1=data::translate_user_notation(e);

      detail::remove(m_normalised_equations, normalize_sorts(e1,*this));
      detail::remove(m_user_defined_equations, e);
    }

    /// \brief Checks whether two sort expressions represent the same sort
    ///
    /// \param[in] s1 A sort expression
    /// \param[in] s2 A sort expression
    bool equal_sorts(sort_expression const& s1, sort_expression const& s2) const
    {
      normalise_data_specification_if_required();
      const sort_expression normalised_sort1=normalize_sorts(s1,*this);
      const sort_expression normalised_sort2=normalize_sorts(s2,*this);
      return (normalised_sort1 == normalised_sort2);
    }

    /// \brief Checks whether a sort is certainly finite.
    ///
    /// \param[in] s A sort expression
    /// \return true if s can be determined to be finite,
    ///      false otherwise.
    bool is_certainly_finite(const sort_expression& s) const;

    /// \brief Checks whether a sort is a constructor sort
    ///
    /// \param[in] s A sort expression
    /// \return true if s is a constructor sort
    bool is_constructor_sort(const sort_expression& s) const
    {
      normalise_data_specification_if_required();
      const sort_expression normalised_sort=normalize_sorts(s,*this);
      return !is_function_sort(normalised_sort) && !constructors(normalised_sort).empty();
    }

    /// \brief Returns true if
    /// <ul>
    /// <li>the domain and range sorts of constructors are contained in the list of sorts</li>
    /// <li>the domain and range sorts of mappings are contained in the list of sorts</li>
    /// </ul>
    /// \return True if the data specification is well typed.
    bool is_well_typed() const;

    bool operator==(const data_specification& other) const
    {
      normalise_data_specification_if_required();
      other.normalise_data_specification_if_required();
      return
        sort_specification::operator==(other) &&
        m_normalised_constructors == other.m_normalised_constructors &&
        m_normalised_mappings == other.m_normalised_mappings &&
        m_normalised_equations == other.m_normalised_equations;
    }

    data_specification& operator=(const data_specification& other)
    {
      sort_specification::operator=(other);
      m_user_defined_constructors=other.m_user_defined_constructors;
      m_user_defined_mappings=other.m_user_defined_mappings;
      m_user_defined_equations=other.m_user_defined_equations;
      m_normalised_mappings=other.m_normalised_mappings;
      m_normalised_constructors=other.m_normalised_constructors;
      m_grouped_normalised_mappings=other.m_grouped_normalised_mappings;
      m_grouped_normalised_constructors=other.m_grouped_normalised_constructors;
      m_normalised_equations=other.m_normalised_equations;
      return *this;
    }

    /// \brief Reads a data specification from a stream.
    /// \param stream An input stream.
    /// \param binary An boolean that if true means the stream contains a term in binary encoding.
    //                Otherwise the encoding is textual.
    /// \param source The source from which the stream originates. Used for error messages.
    void load(std::istream& stream, bool binary = true, const std::string& source = "")
    {
      atermpp::aterm t = core::load_aterm(stream, binary, "data specification", source);
      std::unordered_map<atermpp::aterm_appl, atermpp::aterm> cache;
      t = data::detail::add_index(t, cache);
      if (!t.type_is_appl() || !is_data_specification(atermpp::down_cast<const atermpp::aterm_appl>(t)))
      {
        throw mcrl2::runtime_error("Input stream does not contain a data specification");
      }
      build_from_aterm(atermpp::aterm_appl(t));
    }

    /// \brief Writes the data specification to a stream.
    /// \param stream The output stream.
    /// \param binary
    /// If binary is true the data specification is saved in compressed binary format.
    /// Otherwise an ascii representation is saved. In general the binary format is
    /// much more compact than the ascii representation.
    void save(std::ostream& stream, bool binary=true) const
    {
      atermpp::aterm t = detail::data_specification_to_aterm(*this);
      t = data::detail::remove_index(t);
      if (binary)
      {
        atermpp::write_term_to_binary_stream(t, stream);
      }
      else
      {
        atermpp::write_term_to_text_stream(t, stream);
      }
    }
}; // class data_specification

//--- start generated class data_specification ---//
// prototype declaration
std::string pp(const data_specification& x);

/// \brief Outputs the object to a stream
/// \param out An output stream
/// \param x Object x
/// \return The output stream
inline
std::ostream& operator<<(std::ostream& out, const data_specification& x)
{
  return out << data::pp(x);
}
//--- end generated class data_specification ---//

/// \brief Merges two data specifications into one.
/// \details It is assumed that the two specs can be merged. I.e.
///          that the second is a safe extension of the first.
/// \param[in] spec1 One of the input specifications.
/// \param[in] spec2 The second input specification.
/// \return A specification that is merged.
inline data_specification operator +(data_specification spec1, const data_specification& spec2)
{
  for(const basic_sort& bsort: spec2.user_defined_sorts())
  {
    spec1.add_sort(bsort);
  }

  for(const sort_expression& sort: spec2.context_sorts())
  {
    spec1.add_context_sort(sort);
  }

  for(const alias& a: spec2.user_defined_aliases())
  {
    spec1.add_alias(a);
  }

  for(const function_symbol& f: spec2.user_defined_constructors())
  {
    spec1.add_constructor(f);
  }

  for(const function_symbol& f: spec2.user_defined_mappings())
  {
    spec1.add_mapping(f);
  }

  for(const data_equation& e: spec2.user_defined_equations())
  {
    spec1.add_equation(e);
  }

  return spec1;
}

/// \brief Finds a mapping in a data specification.
/// \param data A data specification
/// \param s A string
/// \return The found mapping

inline
function_symbol find_mapping(data_specification const& data, std::string const& s)
{
  const function_symbol_vector& r(data.mappings());
  function_symbol_vector::const_iterator i = std::find_if(r.begin(), r.end(), detail::function_symbol_has_name(s));
  return (i == r.end()) ? function_symbol() : *i;
}

/// \brief Finds a constructor in a data specification.
/// \param data A data specification
/// \param s A string
/// \return The found constructor

inline
function_symbol find_constructor(data_specification const& data, std::string const& s)
{
  const function_symbol_vector& r(data.constructors());
  function_symbol_vector::const_iterator i = std::find_if(r.begin(), r.end(), detail::function_symbol_has_name(s));
  return (i == r.end()) ? function_symbol() : *i;
}

/// \brief Finds a sort in a data specification.
/// \param data A data specification
/// \param s A string
/// \return The found sort

inline
sort_expression find_sort(data_specification const& data, std::string const& s)
{
  const std::set<sort_expression>& r(data.sorts());
  const std::set<sort_expression>::const_iterator i = std::find_if(r.begin(), r.end(), detail::sort_has_name(s));
  return (i == r.end()) ? sort_expression() : *i;
}

/// \brief Gets all equations with a data expression as head
/// on one of its sides.
///
/// \param[in] specification A data specification.
/// \param[in] d A data expression.
/// \return All equations with d as head in one of its sides.

inline
data_equation_vector find_equations(data_specification const& specification, const data_expression& d)
{
  data_equation_vector result;
  for (const data_equation& eq: specification.equations())
  {
    if (eq.lhs() == d || eq.rhs() == d)
    {
      result.push_back(eq);
    }
    else if (is_application(eq.lhs()))
    {
      if (atermpp::down_cast<application>(eq.lhs()).head() == d)
      {
        result.push_back(eq);
      }
    }
    else if (is_application(eq.rhs()))
    {
      if (atermpp::down_cast<application>(eq.rhs()).head() == d)
      {
        result.push_back(eq);
      }
    }
  }
  return result;
}



/// \brief Order the variables in a variable list such that enumeration over these variables becomes more efficient.
//  \detail This routine is experimental, and may benefit from further investigation. The rule of thumb that is
//          now used is that first variables of enumerated types are put in the variable list. These are sorts with
//          constructors that have no arguments, typically resulting from declarations such as sort Enum = struct e1 | e2 | e3.
//          The variables with the largest number of elements are put in front.
//          Subsequently, the other data types with a finite number of elements are listed, in arbitrary sequence. At the
//          end all other variables are put in an arbitrary sequence.
/// \param[in] l A list of variables that are to be sorted.
/// \param[in] data_spec A data specification containing the constructors that are used to determine efficiency.
/// \return The same list of variables but ordered in such a way that


inline variable_list order_variables_to_optimise_enumeration(const variable_list& l, const data_specification& data_spec)
{
  // Put variables with enumerated types with the largest number of elements in front.
  // Put variables of finite types as second.
  // Finally put the other variables in the list.
  std::map < std::size_t,variable_list> vars_of_enumerated_type;
  variable_list vars_of_finite_type;
  variable_list rest_vars;
  for(const variable& v: l)
  {
    if (data_spec.is_certainly_finite(v.sort()))
    {
      bool is_enumerated_type=true;
      for(const function_symbol& f: data_spec.constructors(v.sort()))
      {
        if (is_function_sort(f.sort()) && function_sort(f.sort()).domain().size()>0)
        {
          is_enumerated_type=false;
          break;
        }
      }
      if (is_enumerated_type)
      {
        vars_of_enumerated_type[data_spec.constructors(v.sort()).size()].push_front(v);
      }
      else
      {
        vars_of_finite_type.push_front(v);
      }
    }
    else
    {
      // variable *i has no finite type
      rest_vars.push_front(v);
    }
  }

  // Accumulate the result in rest_vars
  rest_vars=vars_of_finite_type + rest_vars;
  for(std::map <std::size_t,variable_list>::const_reverse_iterator k=vars_of_enumerated_type.rbegin(); k!=vars_of_enumerated_type.rend(); ++k)
  {
    rest_vars = k->second + rest_vars;
  }
  return rest_vars;
}

/// \brief Returns the names of functions and mappings that occur in a data specification.
/// \param[in] dataspec A data specification
inline
std::set<core::identifier_string> function_and_mapping_identifiers(const data_specification& dataspec)
{
  std::set<core::identifier_string> result;
  for (auto const& f: dataspec.constructors())
  {
    result.insert(f.name());
  }
  for (auto const& f: dataspec.mappings())
  {
    result.insert(f.name());
  }
  return result;
}

} // namespace data

} // namespace mcrl2

#endif // MCRL2_DATA_DATA_SPECIFICATION_H

