// Copyright (C) 2020 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_PARSING_FRAGMENT_H
#define LIBSCRIPT_PARSING_FRAGMENT_H

#include "script/parser/token.h"

#include <vector>

namespace script
{

namespace parser
{

/*!
 * \class Fragment
 * \brief a range in a token list
 */
class Fragment
{
public:
  Fragment(const Fragment&) = default;

  explicit Fragment(const std::vector<Token>& tokens);

  /*!
   * \enum FragmentKind
   * \brief provides a description for subfragments
   * This enum is used by the TokenReader class for constructing 
   * subfragment.
   */
  enum FragmentKind
  {
    DelimiterPair,
    Statement,
    ListElement,
    Template,
  };

  template<FragmentKind FK>
  struct Type {};

  /*!
   * \typedef std::vector<Token>::const_iterator iterator
   * \brief typedef provided for convenience
   */
  typedef std::vector<Token>::const_iterator iterator;

  Fragment(iterator begin, iterator end);

  iterator begin() const;
  iterator end() const;

  size_t size() const;

  Fragment& operator=(const Fragment&) = default;

private:
  iterator m_begin;
  iterator m_end;
};

/*!
 * \fn Fragment(const std::vector<Token>& tokens)
 * \brief constructs a fragment consisting of the full token list
 */
inline Fragment::Fragment(const std::vector<Token>& tokens)
  : Fragment(tokens.begin(), tokens.end())
{

}

/*!
 * \fn Fragment(iterator begin, iterator end)
 * \brief constructs a fragment from an explicitely specified range
 */
inline Fragment::Fragment(iterator begin, iterator end)
  : m_begin(begin),
    m_end(end)
{

}

/*!
 * \fn iterator begin() const
 * \brief returns the begin iterator of the fragment
 */
inline std::vector<Token>::const_iterator Fragment::begin() const
{
  return m_begin;
}

/*!
 * \fn iterator end() const
 * \brief returns the end iterator of the fragment
 */
inline std::vector<Token>::const_iterator Fragment::end() const
{
  return m_end;
}

/*!
 * \fn size_t size() const
 * \brief returns the number of tokens in the fragment
 */
inline size_t Fragment::size() const
{
  return std::distance(begin(), end());
}

/*!
 * \fn bool operator==(const Fragment& lhs, const Fragment& rhs)
 * \brief compares two fragments for equality
 * Note that comparing two fragments constructed from different token lists 
 * is undefined behavior.
 */
inline bool operator==(const Fragment& lhs, const Fragment& rhs)
{
  return lhs.begin() == rhs.begin() && lhs.end() == rhs.end();
}

/*!
 * \fn bool operator!=(const Fragment& lhs, const Fragment& rhs)
 * \brief compares two fragments for inequality
 * Note that comparing two fragments constructed from different token lists
 * is undefined behavior.
 */
inline bool operator!=(const Fragment& lhs, const Fragment& rhs) 
{ 
  return !(lhs == rhs); 
}

} // namespace parser

} // namespace script

#endif // LIBSCRIPT_PARSING_FRAGMENT_H
