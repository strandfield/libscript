// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_PARSER_ERRORS_H
#define LIBSCRIPT_PARSER_ERRORS_H

#include "script/parser/lexer.h"
#include "script/ast/ast.h"

#include "script/diagnosticmessage.h"

namespace script
{

namespace parser
{

class ParserException
{
public:
  ParserException() = default;
  ParserException(const ParserException &) = default;
  virtual ~ParserException() = default;
  
  virtual std::string what() const = 0;
};

#define DECLARE_ERROR(name) class name : public ParserException { \
public: \
  ~name() = default; \
  static const std::string message; \
  std::string what() const override { return message; } \
}


#define DECLARE_ERROR_2(name, parent) class name : public parent { \
public: \
  ~name() = default; \
  static const std::string message; \
  std::string what() const override { return message; } \
}

DECLARE_ERROR(ImplementationError);
DECLARE_ERROR(UnexpectedEndOfInput);
DECLARE_ERROR(UnexpectedFragmentEnd);

DECLARE_ERROR(UnexpectedToken);
DECLARE_ERROR(UnexpectedClassKeyword);
DECLARE_ERROR(UnexpectedOperatorKeyword);

DECLARE_ERROR(ExpectedLeftBrace);
DECLARE_ERROR(ExpectedRightBrace);
DECLARE_ERROR(ExpectedLeftPar);
DECLARE_ERROR(ExpectedRightPar);
DECLARE_ERROR(ExpectedRightBracket);
DECLARE_ERROR(ExpectedSemicolon);
DECLARE_ERROR(ExpectedComma);
DECLARE_ERROR(ExpectedZero);
DECLARE_ERROR(ExpectedEqualSign);
DECLARE_ERROR(ExpectedColon);

DECLARE_ERROR(CouldNotReadIdentifier);
DECLARE_ERROR(IllegalSpaceBetweenPars);
DECLARE_ERROR(IllegalSpaceBetweenBrackets);
DECLARE_ERROR(ExpectedEmptyStringLiteral);
DECLARE_ERROR(ExpectedOperatorSymbol);
DECLARE_ERROR(ExpectedUserDefinedName);

DECLARE_ERROR(CouldNotReadLiteral);


class ExpressionParserException : public ParserException {};
DECLARE_ERROR_2(InvalidEmptyOperand, ExpressionParserException);
DECLARE_ERROR_2(InvalidEmptyBrackets, ExpressionParserException);
DECLARE_ERROR_2(CouldNotReadOperator, ExpressionParserException);
DECLARE_ERROR_2(ExpectedBinaryOperator, ExpressionParserException);
DECLARE_ERROR_2(MissingConditionalColon, ExpressionParserException);
DECLARE_ERROR_2(NotPrefixOperator, ExpressionParserException);



class LambdaParserException : public ParserException {};
DECLARE_ERROR_2(CouldNotParseLambdaCapture, LambdaParserException);


class DeclParserException : public ParserException {};
DECLARE_ERROR_2(IllegalUseOfExplicit, DeclParserException);
DECLARE_ERROR_2(IllegalUseOfVirtual, DeclParserException);
DECLARE_ERROR_2(ExpectedCurrentClassName, DeclParserException);
DECLARE_ERROR_2(CouldNotReadType, DeclParserException);


class ClassParserException : public ParserException {};
DECLARE_ERROR_2(ExpectedDeclaration, ClassParserException);



class NotImplementedError : public ParserException { 
public: 
  NotImplementedError(const std::string & mssg) : message(mssg) {}
  NotImplementedError(std::string && mssg) : message(std::move(mssg)) {}
  std::string what() const override { return this->message; } 

  std::string message;
};

#undef DECLARE_ERROR
#undef DECLARE_ERROR_2


} // namespace parser

} // namespace script


#endif // LIBSCRIPT_PARSER_ERRORS_H
