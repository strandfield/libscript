// Copyright (C) 2018-2020 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_OPERATOR_NAMES_H
#define LIBSCRIPT_OPERATOR_NAMES_H

#include "libscriptdefs.h"

namespace script
{

/*!
 * \enum Associativity
 * \brief describes the associativity of operators
 *
 * Possible values are LeftToRight and RightToLeft.
 */
enum class Associativity {
  LeftToRight,
  RightToLeft
};

/*!
 * \enum OperatorName
 * \brief names the existing operators provided by the language
 */
enum OperatorName {
  InvalidOperator = 0,
  // Precedence 1 (Left-to-right)
  ScopeResolutionOperator = 1,
  // Precedence 2 (Left-to-right)
  PostIncrementOperator = 2,
  PostDecrementOperator = 3,
  FunctionCallOperator = 4,
  SubscriptOperator = 5,
  MemberAccessOperator = 6,
  // Precedence 3 (Right-to-left)
  PreIncrementOperator = 7,
  PreDecrementOperator = 8,
  UnaryPlusOperator = 9,
  UnaryMinusOperator = 10,
  LogicalNotOperator = 11,
  BitwiseNot = 12,
  // Precedence 4 (Left-to-right)
  MultiplicationOperator = 13,
  DivisionOperator = 14,
  RemainderOperator = 15,
  // Precedence 5 (Left-to-right)
  AdditionOperator = 16,
  SubstractionOperator = 17,
  // Precedence 6 (Left-to-right)
  LeftShiftOperator = 18,
  RightShiftOperator = 19,
  // Precedence 7 (Left-to-right)
  LessOperator = 20,
  GreaterOperator = 21,
  LessEqualOperator = 22,
  GreaterEqualOperator = 23,
  // Precedence 8 (Left-to-right)
  EqualOperator = 24,
  InequalOperator = 25,
  // Precedence 9 (Left-to-right)
  BitwiseAndOperator = 26,
  // Precedence 10 (Left-to-right)
  BitwiseXorOperator = 27,
  // Precedence 11 (Left-to-right)
  BitwiseOrOperator = 28,
  // Precedence 12 (Left-to-right)
  LogicalAndOperator = 29,
  // Precedence 13 (Left-to-right)
  LogicalOrOperator = 30,
  // Precedence 14 (Right-to-left)
  ConditionalOperator = 31,
  AssignmentOperator = 32,
  MultiplicationAssignmentOperator = 33,
  DivisionAssignmentOperator = 34,
  RemainderAssignmentOperator = 35,
  AdditionAssignmentOperator = 36,
  SubstractionAssignmentOperator = 37,
  LeftShiftAssignmentOperator = 38,
  RightShiftAssignmentOperator = 39,
  BitwiseAndAssignmentOperator = 40,
  BitwiseOrAssignmentOperator = 41,
  BitwiseXorAssignmentOperator = 42,
  // Precedence 15 (Left-to-right)
  CommaOperator = 43,
};

LIBSCRIPT_API int precedence(OperatorName op);
LIBSCRIPT_API Associativity associativity(int group);

} // namespace script

#endif // LIBSCRIPT_OPERATOR_NAMES_H
