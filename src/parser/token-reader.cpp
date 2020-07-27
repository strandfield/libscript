// Copyright (C) 2020 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/parser/token-reader.h"

#include "script/parser/delimiters-counter.h"

namespace script
{

namespace parser
{

static bool try_build_template_fragment(Fragment::iterator begin, Fragment::iterator end, 
  Fragment::iterator& o_begin, Fragment::iterator& o_end, bool& o_half_consumed_right_right)
{
  if (begin->id != Token::LeftAngle)
    return false;

  DelimitersCounter counter;
  int angle_counter = 0;

  for (auto it = begin; it != end; ++it)
  {
    counter.feed(*it);

    if (counter.invalid())
      return false;

    if (it->id == Token::RightAngle)
    {
      if (counter.balanced())
      {
        --angle_counter;

        if (angle_counter == 0)
        {
          o_begin = begin + 1;
          o_end = it;
          o_half_consumed_right_right = false;
          return true;
        }
      }
    }
    else if (it->id == Token::RightRightAngle)
    {
      if (counter.balanced())
      {
        if (angle_counter == 1 || angle_counter == 2)
        {
          angle_counter = 0;
          o_half_consumed_right_right = true;
          o_begin = begin + 1;
          o_end = it;
          return true;
        }
        else
        {
          angle_counter -= 2;
        }
      }
    }
    else if (it->id == Token::LeftAngle)
    {
      if (counter.balanced())
        ++angle_counter;
    }
  }

  return false;
}

TokenReader TokenReader::subfragment_helper(Fragment::Type<Fragment::Template>) const
{
  Fragment::iterator frag_begin;
  Fragment::iterator frag_end;
  Fragment::iterator current_frag_end = m_right_right_angle_flag && fragment().end()->id == Token::RightRightAngle ?
    fragment().end() + 1 : fragment().end();
  bool half_consumed_right_right = false;

  bool ok = try_build_template_fragment(iterator(), current_frag_end,
    frag_begin, frag_end, half_consumed_right_right);

  if (ok)
    return TokenReader(m_source, Fragment(frag_begin, frag_end), half_consumed_right_right && !m_right_right_angle_flag);
  else
    return TokenReader(nullptr, Fragment(begin(), begin()));
}

TokenReader TokenReader::subfragment_helper(Fragment::Type<Fragment::DelimiterPair>) const
{
  DelimitersCounter counter;
  counter.feed(*m_iterator);

  if (counter.balanced() || counter.invalid())
    throw std::runtime_error{ "bad call to Fragment ctor" };

  auto begin = std::next(m_iterator);

  auto it = begin;

  while (it != fragment().end())
  {
    counter.feed(*it);

    if(counter.invalid())
      throw SyntaxErr(ParserError::UnexpectedFragmentEnd); // @TODO: create better error enum

    if (counter.balanced())
    {
      return TokenReader(m_source, Fragment(begin, it));
    }
    else
    {
      ++it;
    }
  }

  throw SyntaxErr(ParserError::UnexpectedFragmentEnd); // @TODO: create better error enum
}

TokenReader TokenReader::subfragment_helper(Fragment::Type<Fragment::Statement>) const
{
  DelimitersCounter counter;

  auto it = m_iterator;

  while (it != fragment().end())
  {
    counter.feed(*it);

    if (counter.invalid())
      throw SyntaxErr(ParserError::UnexpectedFragmentEnd); // @TODO: create better error enum

    if (it->id == Token::Semicolon)
    {
      if (counter.balanced())
      {
        return TokenReader(m_source, Fragment(m_iterator, it));;
      }
      else
      {
        // @TODO: we could check that we are inside brackets
        ++it;
      }
    }
    else
    {
      ++it;
    }
  }

  throw SyntaxErr(ParserError::UnexpectedFragmentEnd); // @TODO: create better error enum
}

TokenReader TokenReader::subfragment_helper(Fragment::Type<Fragment::ListElement>) const
{
  DelimitersCounter counter;

  auto it = m_iterator;

  while (it != fragment().end())
  {
    counter.feed(*it);

    if(counter.invalid()) // @TODO: write offset
      throw SyntaxErr(ParserError::UnexpectedFragmentEnd); // @TODO: create better error enum

    if (it->id == Token::Comma)
    {
      if (counter.balanced())
      {
        return TokenReader(m_source, Fragment(m_iterator, it));
      }
      else
      {
        // @TODO: we could check that we are inside brackets
        ++it;
      }
    }
    else
    {
      ++it;
    }
  }

  if (!counter.balanced())
    throw SyntaxErr(ParserError::UnexpectedFragmentEnd); // @TODO: create better error enum

  return TokenReader(m_source, Fragment(m_iterator, fragment().end()), m_right_right_angle_flag);
}

} // parser

} // namespace script

