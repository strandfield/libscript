// Copyright (C) 2020 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_PARSER_ERRORS_H
#define LIBSCRIPT_PARSER_ERRORS_H

#include <system_error>

namespace script
{

namespace errors
{

const std::error_category& parser_category() noexcept;

} // namespace errors

enum class ParserError {
  UnexpectedEndOfInput = 1,
  UnexpectedFragmentEnd,
  UnexpectedToken,
  ExpectedEmptyStringLiteral,
  InvalidEmptyBrackets,
  IllegalUseOfKeyword,
  ExpectedIdentifier,
  ExpectedUserDefinedName,
  ExpectedLiteral,
  ExpectedOperator,
  ExpectedBinaryOperator,
  ExpectedPrefixOperator,
  ExpectedOperatorSymbol,
  InvalidEmptyOperand,
  ExpectedDeclaration,
  MissingConditionalColon,
  CouldNotParseLambdaCapture,
  ExpectedCurrentClassName,
  CouldNotReadType,
};

inline std::error_code make_error_code(script::ParserError e) noexcept
{
  return std::error_code(static_cast<int>(e), script::errors::parser_category());
}

} // namespace script

namespace std
{

template<> struct is_error_code_enum<script::ParserError> : std::true_type { };

} // namespace std

#endif // LIBSCRIPT_PARSER_ERRORS_H
