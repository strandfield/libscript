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
  
  virtual std::string what() const { return "no implemented"; };
};

class UnexpectedEndOfInput : public ParserException
{
public:
  UnexpectedEndOfInput() = default;
  ~UnexpectedEndOfInput() = default;
  ErrorCode code() const override { return ErrorCode::P_UnexpectedEndOfInput; }
};

class UnexpectedFragmentEnd : public ParserException
{
public:
  UnexpectedFragmentEnd() = default;
  ~UnexpectedFragmentEnd() = default;
  ErrorCode code() const override { return ErrorCode::P_UnexpectedFragmentEnd; }
};


class UnexpectedToken : public ParserException
{
public:
  Token actual;
  Token::Type expected;

public:
  UnexpectedToken(const Token & got, Token::Type expect = Token::Invalid) : actual(got), expected(expect) { }
  ~UnexpectedToken() = default;
  ErrorCode code() const override { return ErrorCode::P_UnexpectedToken; }
};

class IllegalUseOfKeyword : public ParserException
{
public:
  Token keyword;

public:
  IllegalUseOfKeyword(const Token & got) : keyword(got) { }
  ~IllegalUseOfKeyword() = default;
  ErrorCode code() const override { return ErrorCode::P_IllegalUseOfKeyword; }
};

class ExpectedEmptyStringLiteral : public ParserException
{
public:
  Token actual;

public:
  ExpectedEmptyStringLiteral(const Token & got) : actual(got) { }
  ~ExpectedEmptyStringLiteral() = default;
  ErrorCode code() const override { return ErrorCode::P_ExpectedEmptyStringLiteral; }
};

class ExpectedOperatorSymbol : public ParserException
{
public:
  Token actual;

public:
  ExpectedOperatorSymbol(const Token & got) : actual(got) { }
  ~ExpectedOperatorSymbol() = default;
  ErrorCode code() const override { return ErrorCode::P_ExpectedOperatorSymbol; }
};

class ExpectedIdentifier : public ParserException
{
public:
  Token actual;

public:
  ExpectedIdentifier(const Token & got) : actual(got) { }
  ~ExpectedIdentifier() = default;
  ErrorCode code() const override { return ErrorCode::P_ExpectedIdentifier; }
};

class ExpectedUserDefinedName : public ParserException
{
public:
  Token actual;

public:
  ExpectedUserDefinedName(const Token & got) : actual(got) { }
  ~ExpectedUserDefinedName() = default;
  ErrorCode code() const override { return ErrorCode::P_ExpectedUserDefinedName; }
};


class ExpectedLiteral : public ParserException
{
public:
  Token actual;

public:
  ExpectedLiteral(const Token & got) : actual(got) { }
  ~ExpectedLiteral() = default;
  ErrorCode code() const override { return ErrorCode::P_ExpectedLiteral; }
};

class InvalidEmptyOperand : public ParserException
{
public:
  InvalidEmptyOperand() = default;
  ~InvalidEmptyOperand() = default;
  ErrorCode code() const override { return ErrorCode::P_InvalidEmptyOperand; }
};

class InvalidEmptyBrackets : public ParserException
{
  /// TODO: add pos ?
public:
  InvalidEmptyBrackets() = default;
  ~InvalidEmptyBrackets() = default;
  ErrorCode code() const override { return ErrorCode::P_InvalidEmptyBrackets; }
};

class ExpectedOperator : public ParserException
{
public:
  Token actual;

public:
  ExpectedOperator(const Token & got) : actual(got) { }
  ~ExpectedOperator() = default;
  ErrorCode code() const override { return ErrorCode::P_ExpectedOperator; }
};

class ExpectedBinaryOperator : public ParserException
{
public:
  Token actual;

public:
  ExpectedBinaryOperator(const Token & got) : actual(got) { }
  ~ExpectedBinaryOperator() = default;
  ErrorCode code() const override { return ErrorCode::P_ExpectedBinaryOperator; }
};

class ExpectedPrefixOperator : public ParserException
{
public:
  Token actual;

public:
  ExpectedPrefixOperator(const Token & got) : actual(got) { }
  ~ExpectedPrefixOperator() = default;
  ErrorCode code() const override { return ErrorCode::P_ExpectedPrefixOperator; }
};

class MissingConditionalColon : public ParserException
{
  /// TODO: add pos ?
public:
  MissingConditionalColon() = default;
  ~MissingConditionalColon() = default;
  ErrorCode code() const override { return ErrorCode::P_MissingConditionalColon; }
};


class CouldNotParseLambdaCapture : public ParserException
{
  /// TODO: add pos ?
public:
  CouldNotParseLambdaCapture() = default;
  ~CouldNotParseLambdaCapture() = default;
  ErrorCode code() const override { return ErrorCode::P_CouldNotParseLambdaCapture; }
};

class ExpectedCurrentClassName : public ParserException
{
  /// TODO: add pos ?
public:
  ExpectedCurrentClassName() = default;
  ~ExpectedCurrentClassName() = default;
  ErrorCode code() const override { return ErrorCode::P_ExpectedCurrentClassName; }
};

class CouldNotReadType : public ParserException
{
  /// TODO: add pos ?
public:
  CouldNotReadType() = default;
  ~CouldNotReadType() = default;
  ErrorCode code() const override { return ErrorCode::P_CouldNotReadType; }
};

class ExpectedDeclaration : public ParserException
{
  /// TODO: add pos ?
public:
  ExpectedDeclaration() = default;
  ~ExpectedDeclaration() = default;
  ErrorCode code() const override { return ErrorCode::P_ExpectedDeclaration; }
};

} // namespace parser

} // namespace script


#endif // LIBSCRIPT_PARSER_ERRORS_H
