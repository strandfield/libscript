// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_PARSER_PARSERERRORS_H
#define LIBSCRIPT_PARSER_PARSERERRORS_H

#include "script/parser/lexer.h"
#include "script/ast/ast_p.h"

#include "script/diagnosticmessage.h"
#include "script/exception.h"

#include "script/parser/errors.h"

namespace script
{

namespace parser
{

class SyntaxError : public Exceptional
{
public:
  SourceLocation location;

public:

  SyntaxError(SyntaxError&&) noexcept = default;

  explicit SyntaxError(ParserError e)
    : Exceptional(e)
  {

  }

  template<typename T>
  SyntaxError(ParserError e, T&& d)
    : Exceptional(e, std::forward<T>(d))
  {

  }
};

namespace errors
{

struct ActualToken
{
  Token token;
};

struct KeywordToken
{
  Token keyword;
};

struct UnexpectedToken
{
  Token actual;
  Token::Type expected;
};

} // namespace errors

} // namespace parser

} // namespace script


#endif // LIBSCRIPT_PARSER_PARSERERRORS_H
