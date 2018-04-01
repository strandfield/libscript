// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/parser/parsererrors.h"

namespace script
{

namespace parser
{

const std::string ImplementationError::message = "Implementation error";
const std::string UnexpectedEndOfInput::message = "Unexpected end if input";
const std::string UnexpectedFragmentEnd::message = "Unexpected token";


const std::string UnexpectedToken::message = "Unexpected token";
const std::string UnexpectedClassKeyword::message = "Unexpected 'class' keyword.";
const std::string UnexpectedOperatorKeyword::message = "Unexpected 'operator' keyword.";

const std::string ExpectedLeftBrace::message = "Expected left brace";
const std::string ExpectedRightBrace::message = "Expected right brace";
const std::string ExpectedLeftPar::message = "Expected left parenthesis";
const std::string ExpectedRightPar::message = "Expected right parenthesis";
const std::string ExpectedRightBracket::message = "Expected right bracket";
const std::string ExpectedSemicolon::message = "Expected semicolon";
const std::string ExpectedComma::message = "Expected comma";
const std::string ExpectedZero::message = "Expected zero";
const std::string ExpectedEqualSign::message = "Expected equal sign";
const std::string ExpectedColon::message = "Expected colon";

const std::string CouldNotReadIdentifier::message = "Could not read identifier";
const std::string IllegalSpaceBetweenPars::message = "operator() allows no spaces between '(' and ')'";
const std::string IllegalSpaceBetweenBrackets::message = "operator[] allows no spaces between '[' and ']'";
const std::string ExpectedEmptyStringLiteral::message = "Expected an empty string literal";
const std::string ExpectedOperatorSymbol::message = "Expected 'operator' to be followed by operator symbol";
const std::string ExpectedUserDefinedName::message = "Could not read user-defined name";

const std::string CouldNotReadLiteral::message = "Could not read literal";

const std::string InvalidEmptyOperand::message = "Parentheses are empty";
const std::string InvalidEmptyBrackets::message = "Brackets are empty";
const std::string CouldNotReadOperator::message = "Could not read operator name";
const std::string ExpectedBinaryOperator::message = "A binary operator is expected after an operand";
const std::string MissingConditionalColon::message = "Incomplete conditional expression (read '?' but could not find ':')";
const std::string NotPrefixOperator::message = "Could not read prefix-operator";

const std::string CouldNotParseLambdaCapture::message = "Could not read lambda-capture";

const std::string IllegalUseOfExplicit::message = "'explicit' keyword is not allowed within this context";
const std::string IllegalUseOfVirtual::message = "'virtual' keyword is not allowed within this context";
const std::string ExpectedCurrentClassName::message = "Could not read current class name";
const std::string CouldNotReadType::message = "Could not read type name";

const std::string ExpectedDeclaration::message = "Expected a declaration";



} // namespace parser



} // naemespace script
