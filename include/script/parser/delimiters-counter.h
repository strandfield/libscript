// Copyright (C) 2020 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_PARSING_DELIMITERSCOUNTER_H
#define LIBSCRIPT_PARSING_DELIMITERSCOUNTER_H

#include "script/parser/token.h"

namespace script
{

namespace parser
{

/*!
 * \class DelimitersCounter
 * \brief a basic facility to count matching delimiters 
 *
 * The DelimitersCounter class can be used to verify proper nesting of 
 * delimiters that comes in pair; i.e. \{\}, () and [].
 */
class DelimitersCounter
{
public:
  int par_depth = 0;
  int brace_depth = 0;
  int bracket_depth = 0;

  void reset();

  void feed(const Token& tok);

  bool balanced() const;
  bool invalid() const;
};

/*!
 * \fn void reset()
 * \brief resets all counters to zero
 */
inline void DelimitersCounter::reset()
{
  par_depth = 0;
  brace_depth = 0;
  bracket_depth = 0;
}

/*!
 * \fn void feed(const Token& tok)
 * \param input token
 * \brief updates the counters
 */
inline void DelimitersCounter::feed(const Token& tok)
{
  switch (tok.id)
  {
  case Token::LeftPar:
    ++par_depth;
    break;
  case Token::RightPar:
    --par_depth;
    break;
  case Token::LeftBrace:
    ++brace_depth;
    break;
  case Token::RightBrace:
    --brace_depth;
    break;
  case Token::LeftBracket:
    ++bracket_depth;
    break;
  case Token::RightBracket:
    --bracket_depth;
    break;
  default:
    break;
  }
}

/*!
 * \fn bool balanced() const
 * \brief returns whether delimiters are properly balanced
 */
inline bool DelimitersCounter::balanced() const
{
  return par_depth == 0 && brace_depth == 0 && bracket_depth == 0;
}

/*!
 * \fn bool invalid() const
 * \brief returns whether the counters are in a state that makes balancing impossible
 *
 * This state is reached when a closing delimiter is encountered before its matching 
 * opening delimiter; e.g. ")[])"
 */
inline bool DelimitersCounter::invalid() const
{
  return par_depth < 0 || brace_depth < 0 || bracket_depth < 0;
}

} // namespace parser

} // namespace script

#endif // LIBSCRIPT_PARSING_DELIMITERSCOUNTER_H
