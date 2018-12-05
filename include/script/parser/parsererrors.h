// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_PARSER_ERRORS_H
#define LIBSCRIPT_PARSER_ERRORS_H

#include <tuple>

#include "script/parser/lexer.h"
#include "script/ast/ast_p.h"

#include "script/diagnosticmessage.h"
#include "script/exception.h"

namespace script
{

namespace parser
{

class ParserException : public Exception
{
public:
  ParserException() = default;
  ParserException(const ParserException &) = default;
  virtual ~ParserException() = default;
  
  virtual std::string what() const = 0;
};

class CParserExceptionBase : public ParserException
{
public:
  virtual std::string get_format_message() const = 0;
};

template<typename...Args>
class CParserException : public CParserExceptionBase
{
public:
  typedef std::tuple<Args...>  container_type;
  container_type items;

  CParserException(const Args &... args)
    : items{ args... }
  {

  }

  template<std::size_t...I>
  std::string what_impl(std::index_sequence<I...>) const
  {
    return diagnostic::format(this->get_format_message(), std::get<I>(items)...);
  }

  std::string what() const override
  {
    return what_impl(std::make_index_sequence<sizeof...(Args)>{});
  }
};

#define DECLARE_PARSER_ERROR(name, mssg, ...) class name : public CParserException<__VA_ARGS__> \
{ \
public: \
  using CParserException<__VA_ARGS__>::CParserException; \
  std::string get_format_message() const override { return mssg; } \
};


DECLARE_PARSER_ERROR(ImplementationError, "Implementation error : %1", std::string);
DECLARE_PARSER_ERROR(UnexpectedEndOfInput, "Unexpected end of input");
DECLARE_PARSER_ERROR(UnexpectedFragmentEnd, "Unexpected token");

DECLARE_PARSER_ERROR(UnexpectedToken, "Unexpected token");
DECLARE_PARSER_ERROR(UnexpectedClassKeyword, "Unexpected 'class' keyword.");
DECLARE_PARSER_ERROR(UnexpectedOperatorKeyword, "Unexpected 'operator' keyword.");

DECLARE_PARSER_ERROR(ExpectedLeftBrace, "Expected left brace");
DECLARE_PARSER_ERROR(ExpectedRightBrace, "Expected right brace");
DECLARE_PARSER_ERROR(ExpectedLeftPar, "Expected left parenthesis");
DECLARE_PARSER_ERROR(ExpectedRightPar, "Expected right parenthesis");
DECLARE_PARSER_ERROR(ExpectedRightBracket, "Expected right bracket");
DECLARE_PARSER_ERROR(ExpectedLeftAngle, "Expected left angle");
DECLARE_PARSER_ERROR(ExpectedRightAngle, "Expected right angle");
DECLARE_PARSER_ERROR(ExpectedSemicolon, "Expected semicolon");
DECLARE_PARSER_ERROR(ExpectedComma, "Expected comma");
DECLARE_PARSER_ERROR(ExpectedZero, "Expected zero");
DECLARE_PARSER_ERROR(ExpectedEqualSign, "Expected equal sign");
DECLARE_PARSER_ERROR(ExpectedColon, "Expected colon");

DECLARE_PARSER_ERROR(CouldNotReadIdentifier, "Could not read identifier");
DECLARE_PARSER_ERROR(IllegalSpaceBetweenPars, "operator() allows no spaces between '(' and ')'");
DECLARE_PARSER_ERROR(IllegalSpaceBetweenBrackets, "operator[] allows no spaces between '[' and ']'");
DECLARE_PARSER_ERROR(ExpectedEmptyStringLiteral, "Expected an empty string literal");
DECLARE_PARSER_ERROR(ExpectedOperatorSymbol, "Expected 'operator' to be followed by operator symbol");
DECLARE_PARSER_ERROR(ExpectedUserDefinedName, "Could not read user-defined name");

DECLARE_PARSER_ERROR(CouldNotReadLiteral, "Could not read literal");

DECLARE_PARSER_ERROR(InvalidEmptyOperand, "Parentheses are empty");
DECLARE_PARSER_ERROR(InvalidEmptyBrackets, "Brackets are empty");
DECLARE_PARSER_ERROR(CouldNotReadOperator, "Could not read operator name");
DECLARE_PARSER_ERROR(ExpectedBinaryOperator, "A binary operator is expected after an operand");
DECLARE_PARSER_ERROR(MissingConditionalColon, "Incomplete conditional expression (read '?' but could not find ':')");
DECLARE_PARSER_ERROR(NotPrefixOperator, "Could not read prefix-operator");

DECLARE_PARSER_ERROR(CouldNotParseLambdaCapture, "Could not read lambda-capture");

DECLARE_PARSER_ERROR(IllegalUseOfExplicit, "'explicit' keyword is not allowed within this context");
DECLARE_PARSER_ERROR(IllegalUseOfVirtual, "'virtual' keyword is not allowed within this context");
DECLARE_PARSER_ERROR(ExpectedCurrentClassName, "Could not read current class name");
DECLARE_PARSER_ERROR(CouldNotReadType, "Could not read type name");

DECLARE_PARSER_ERROR(ExpectedDeclaration, "Expected a declaration");

DECLARE_PARSER_ERROR(ExpectedClassKeywordAfterFriend, "Expected 'class' keyword after 'friend'");
DECLARE_PARSER_ERROR(UnexpectedFriendKeyword, "Friend declarations cannot appear at this level.");

DECLARE_PARSER_ERROR(ExpectedImportKeyword, "Expected 'import' keyword after 'export'");
DECLARE_PARSER_ERROR(ExpectedIdentifier, "Expected an identifier");

DECLARE_PARSER_ERROR(NotImplementedError, "%1", std::string);

#undef DECLARE_PARSER_ERROR


} // namespace parser

} // namespace script


#endif // LIBSCRIPT_PARSER_ERRORS_H
