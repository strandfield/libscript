// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/parser/errors.h"
#include "script/parser/parsererrors.h"

namespace script
{

namespace errors
{

class ParserCategory : public std::error_category
{
public:

  const char* name() const noexcept override
  {
    return "parser-category";
  }

  std::string message(int code) const override
  {
    switch (static_cast<ParserError>(code))
    {
    case ParserError::UnexpectedEndOfInput:
      return "unexpected end of input";
    case ParserError::UnexpectedFragmentEnd:
      return "unexpcted end of sub-expression";
    case ParserError::UnexpectedToken:
      return "unexpected token";
    case ParserError::InvalidEmptyBrackets:
      return "invalid empty brackets";
    case ParserError::IllegalUseOfKeyword:
      return "illegal use of keyword";
    case ParserError::ExpectedIdentifier:
      return "expected identifier";
    case ParserError::ExpectedUserDefinedName:
      return "expected user-defined name";
    case ParserError::ExpectedLiteral:
      return "expected literal";
    case ParserError::ExpectedOperatorSymbol:
      return "expected operator symbol";
    case ParserError::InvalidEmptyOperand:
      return "invalid empty operand";
    case ParserError::ExpectedDeclaration:
      return "expected a declaration";
    case ParserError::MissingConditionalColon:
      return "missing ':' in conditional expression";
    case ParserError::CouldNotParseLambdaCapture:
      return "could not parse lambda-capture";
    case ParserError::ExpectedCurrentClassName:
      return "expected current class name";
    case ParserError::CouldNotReadType:
      return "could not read type";
    default:
      return "unknown parser error";
    }
  }
};

const std::error_category& parser_category() noexcept
{
  static ParserCategory static_instance = {};
  return static_instance;
}

} // namespace errors

} // namespace script

