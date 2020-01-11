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

struct ParserErrorData
{
  virtual ~ParserErrorData();

  template<typename T>
  T& get();
};

template<typename T>
struct ParserErrorDataWrapper : ParserErrorData
{
public:
  T value;

public:
  ParserErrorDataWrapper(const ParserErrorDataWrapper<T>&) = delete;
  ~ParserErrorDataWrapper() = default;

  ParserErrorDataWrapper(T&& data) : value(std::move(data)) { }
};

template<typename T>
inline T& ParserErrorData::get()
{
  return static_cast<ParserErrorDataWrapper<T>*>(this)->value;
}

class SyntaxError : public Exceptional
{
public:
  SourceLocation location;
  std::unique_ptr<ParserErrorData> data;

public:

  SyntaxError(SyntaxError&&) noexcept = default;

  explicit SyntaxError(ParserError e)
    : Exceptional(e)
  {

  }

  template<typename T>
  SyntaxError(ParserError e, T&& d)
    : Exceptional(e)
  {
    data = std::make_unique<ParserErrorDataWrapper<T>>(std::move(d));
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
