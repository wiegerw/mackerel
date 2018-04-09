// Author(s): Wieger Wesselink
// Copyright: see the accompanying file COPYING or copy at
// https://svn.win.tue.nl/trac/MCRL2/browser/trunk/COPYING
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
/// \file mcrl2/utilities/detail/container_utility.h
/// \brief add your file description here.

#ifndef MCRL2_UTILITIES_DETAIL_CONTAINER_UTILITY_H
#define MCRL2_UTILITIES_DETAIL_CONTAINER_UTILITY_H

#include "mcrl2/utilities/exception.h"
#include <algorithm>
#include <iterator>
#include <map>
#include <set>
#include <sstream>

namespace mcrl2 {

namespace utilities {

namespace detail {

/// \brief Returns the value corresponding to the given key in the map m. If the key is not
/// present, an exception is thrown.
template <typename Map>
typename Map::mapped_type map_element(const Map& m, const typename Map::key_type& key)
{
  auto i = m.find(key);
  if (i == m.end())
  {
    std::ostringstream out;
    out << "missing key in map!";
    throw mcrl2::runtime_error(out.str());
  }
  return i->second;
}

// Returns the value corresponding to the given key in map m, or undefined_value if no such value exists.
template <typename Map>
typename Map::mapped_type mapped_value(const Map& m, const typename Map::key_type& key, const typename Map::mapped_type& undefined_value)
{
  auto i = m.find(key);
  if (i != m.end())
  {
    return i->second;
  }
  return undefined_value;
}

/// \brief Returns the value corresponding to the given key in the set m. If the key is not
/// present, an exception is thrown.
template <typename Container>
bool contains(const Container& c, const typename Container::value_type& v)
{
  return std::find(c.begin(), c.end(), v) != c.end();
}

// specialization
template <typename T>
bool contains(const std::set<T>& c, const typename std::set<T>::value_type& v)
{
  return c.find(v) != c.end();
}

// specialization
template <typename T>
bool contains(const std::multiset<T>& c, const typename std::multiset<T>::value_type& v)
{
  return c.find(v) != c.end();
}

/// \brief Returns the value corresponding to the given key in the set m. If the key is not
/// present, an exception is thrown.
template <typename Key, typename T>
bool has_key(const std::map<Key, T>& c, const Key& v)
{
  return c.find(v) != c.end();
}

/// \brief Returns the value corresponding to the given key in the set m. If the key is not
/// present, an exception is thrown.
template <typename Key, typename T>
bool has_key(const std::multimap<Key, T>& c, const Key& v)
{
  return c.find(v) != c.end();
}

// Remove an element from v, and return it.
template <typename Container>
typename Container::value_type pick_element(Container& v)
{
  auto i = v.begin();
  auto result = *i;
  v.erase(i);
  return result;
}

// inserts elements of c into s
template <typename T, typename Container>
void set_insert(std::set<T>& s, const Container& c)
{
  for (auto i = c.begin(); i != c.end(); ++i)
  {
    s.insert(*i);
  }
}

// removes elements of c from s
template <typename T, typename Container>
void set_remove(std::set<T>& s, const Container& c)
{
  for (auto i = c.begin(); i != c.end(); ++i)
  {
    s.erase(*i);
  }
}

// C++11; works for sets and maps
// Removes elements that satisfy predicate pred.
template< typename ContainerT, typename PredicateT >
void remove_if(ContainerT& items, const PredicateT& predicate)
{
  for (auto it = items.begin(); it != items.end();)
  {
	if (predicate(*it))
	{
	  it = items.erase(it);
	}
	else ++it;
  }
}

/// Returns true if the sorted ranges [first1, ..., last1) and [first2, ..., last2) have an empty intersection
template <typename InputIterator1, typename InputIterator2>
bool has_empty_intersection(InputIterator1 first1, InputIterator1 last1, InputIterator2 first2, InputIterator2 last2)
{
  while (first1 != last1 && first2 != last2)
  {
    if (*first1 < *first2)
    {
      ++first1;
    }
    else if (*first2 < *first1)
    {
      ++first2;
    }
    else
    {
      return false;
    }
  }
  return true;
}

template <typename T>
bool has_empty_intersection(const std::set<T>& s1, const std::set<T>& s2)
{
  return has_empty_intersection(s1.begin(), s1.end(), s2.begin(), s2.end());
}

/// \brief Returns the union of two sets.
/// \param x A set
/// \param y A set
/// \return The union of two sets.
template <typename T>
std::set<T> set_union(const std::set<T>& x, const std::set<T>& y)
{
  std::set<T> result;
  std::set_union(x.begin(), x.end(), y.begin(), y.end(), std::inserter(result, result.begin()));
  return result;
}

/// \brief Returns the difference of two sets.
/// \param x A set
/// \param y A set
/// \return The difference of two sets.
template <typename T>
std::set<T> set_difference(const std::set<T>& x, const std::set<T>& y)
{
  std::set<T> result;
  std::set_difference(x.begin(), x.end(), y.begin(), y.end(), std::inserter(result, result.begin()));
  return result;
}

/// \brief Returns the intersection of two sets.
/// \param x A set
/// \param y A set
/// \return The intersection of two sets.
template <typename T>
std::set<T> set_intersection(const std::set<T>& x, const std::set<T>& y)
{
  std::set<T> result;
  std::set_intersection(x.begin(), x.end(), y.begin(), y.end(), std::inserter(result, result.begin()));
  return result;
}

} // namespace detail

} // namespace utilities

} // namespace mcrl2

#endif // MCRL2_UTILITIES_DETAIL_CONTAINER_UTILITY_H
