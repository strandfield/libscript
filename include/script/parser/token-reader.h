// Copyright (C) 2020 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_PARSING_TOKENREADER_H
#define LIBSCRIPT_PARSING_TOKENREADER_H

#include "script/parser/fragment.h"
#include "script/parser/parsererrors.h"

namespace script
{

namespace parser
{

/*!
 * \class TokenReader
 * \brief basic facility for consuming tokens
 */
class TokenReader
{
public:
  const char* m_source; // used only for computing location when throwing
  Fragment m_fragment;
  Fragment::iterator m_iterator;
  bool m_right_right_angle_flag;

public:
  TokenReader(const TokenReader&) = default;
  ~TokenReader() = default;

  TokenReader(const char* src, const std::vector<Token>& tokens);
  TokenReader(const char* src, const Fragment& frag, bool right_right_angle = false);

  bool valid() const { return m_source != nullptr; }

  Fragment::iterator iterator() const { return m_iterator; }
  const Fragment& fragment() const { return m_fragment; }

  Fragment::iterator begin() const { return fragment().begin(); }
  Fragment::iterator end() const { return fragment().end(); }

  bool atEnd() const;
  Token read();
  Token unsafe_read();
  Token read(const Token::Id& t);
  Token peek() const;
  Token peek(size_t n) const;
  Token unsafe_peek() const;
  void seek(Fragment::iterator it);
  TokenReader subfragment() const;

  template<Fragment::FragmentKind FK>
  TokenReader subfragment() const
  {
    return subfragment_helper(Fragment::Type<FK>());
  }

  template<Fragment::FragmentKind FK>
  TokenReader next()
  {
    TokenReader r = subfragment<FK>();
    seek(r.end());
    return r;
  }

  operator const Fragment& () const { return m_fragment; }

  SyntaxError SyntaxErr(ParserError e) const
  {
    SyntaxError err{ e };
    err.offset = std::distance(m_source, m_iterator->text().data());
    return err;
  }

  template<typename T>
  SyntaxError SyntaxErr(ParserError e, T&& d) const
  {
    SyntaxError err{ e, std::forward<T>(d) };
    err.offset = std::distance(m_source, m_iterator->text().data());
    return err;
  }

  TokenReader& operator=(const TokenReader&) = default;

private:
  TokenReader subfragment_helper(Fragment::Type<Fragment::DelimiterPair>) const;
  TokenReader subfragment_helper(Fragment::Type<Fragment::Statement>) const;
  TokenReader subfragment_helper(Fragment::Type<Fragment::ListElement>) const;
  TokenReader subfragment_helper(Fragment::Type<Fragment::Template>) const;
};

/*!
 * \fn TokenReader(const char* src, const std::vector<Token>& tokens)
 * \param the source string for the tokens
 * \param the token list
 * \brief constructs a token reader from a list of tokens
 */
inline TokenReader::TokenReader(const char* src, const std::vector<Token>& tokens)
  : TokenReader(src, Fragment(tokens.begin(), tokens.end()))
{

}

inline TokenReader::TokenReader(const char* src, const Fragment& frag, bool right_right_angle)
  : m_source(src),
  m_fragment(frag),
  m_iterator(frag.begin()),
  m_right_right_angle_flag(right_right_angle)
{

}

/*!
 * \fn bool atEnd() const
 * \brief returns whether all tokens have been read
 */
inline bool TokenReader::atEnd() const
{
  return m_iterator == fragment().end();
}

/*!
 * \fn Token read()
 * \brief reads the next token
 * Note that this function throws SyntaxError if no more token is available.
 */
inline Token TokenReader::read()
{
  if (atEnd())
    throw SyntaxError{ ParserError::UnexpectedEndOfInput };

  return *(m_iterator++);
}

/*!
 * \fn Token unsafe_read()
 * \brief reads the next token without bounds checking
 * Use this function only after checking that atEnd() returns false.
 */
inline Token TokenReader::unsafe_read()
{
  return *(m_iterator++);
}

/*!
 * \fn Token read(const Token::Id& type)
 * \param the type of the token to read
 * \brief reads a particular token
 * This function throws SyntaxError if no token are available or if the token 
 * is not of the requested type.
 */
inline Token TokenReader::read(const Token::Id& type)
{
  Token ret = read();

  if (ret != type)
    throw SyntaxError{ ParserError::UnexpectedToken, errors::UnexpectedToken{ret, type} };

  return ret;
}

/*!
 * \fn Token peek() const
 * \brief returns the next token without consuming it
 * Successive calls to peek() return the same token. Call read() to get the current 
 * token and go to the next.
 */
inline Token TokenReader::peek() const
{
  return *m_iterator;
}

/*!
 * \fn Token peek(size_t n) const
 * \param number of token to skip
 * \brief look ahead a token
 */
inline Token TokenReader::peek(size_t n) const
{
  return *(std::next(m_iterator, n));
}

/*!
 * \fn Token unsafe_peek() const
 * \brief returns the current token without performing bounds checking
 */
inline Token TokenReader::unsafe_peek() const
{
  return *(m_iterator);
}

/*!
 * \fn void seek(Fragment::iterator it)
 * \param iterator to the token to set as current
 * \brief moves the reading cursor
 */
inline void TokenReader::seek(Fragment::iterator it)
{
  m_iterator = it;
}

/*!
 * \fn TokenReader subfragment() const
 * \brief returns a token reader working on a subrange of tokens
 */
inline TokenReader TokenReader::subfragment() const
{
  return TokenReader(m_source, Fragment(iterator(), fragment().end()), m_right_right_angle_flag);
}

/*!
 * \fn bool operator==(const TokenReader& lhs, const TokenReader& rhs)
 * \brief compares two token readers for equality
 * Two token readers are considered equal if the work on the same range of tokens 
 * and if the current cursor is at the same token.
 */
inline bool operator==(const TokenReader& lhs, const TokenReader& rhs)
{
  return lhs.fragment() == rhs.fragment() && lhs.iterator() == rhs.iterator();
}

/*!
 * \fn bool operator!=(const TokenReader& lhs, const TokenReader& rhs)
 * \brief compares two token readers for inequality
 */
inline bool operator!=(const TokenReader& lhs, const TokenReader& rhs) 
{
  return !(lhs == rhs); 
}

} // namespace parser

} // namespace script

#endif // LIBSCRIPT_PARSING_TOKENREADER_H
