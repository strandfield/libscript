// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_OPERATOR_H
#define LIBSCRIPT_OPERATOR_H

#include "function.h"

namespace script
{

class OperatorImpl;

class LIBSCRIPT_API Operator : public Function
{
public:
  Operator() = default;
  Operator(const Operator & other) = default;
  ~Operator() = default;

  Operator(const std::shared_ptr<OperatorImpl> & impl);


  enum BuiltInOperator {
    Null = 0,
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

  enum Associativity {
    LeftToRight,
    RightToLeft
  };

  BuiltInOperator operatorId() const;
  bool isBinary() const;
  bool isBuiltin() const;

  static bool isBinary(BuiltInOperator op);
  static bool isUnary(BuiltInOperator op);
  inline static bool isTernary(BuiltInOperator op) {
    return op == ConditionalOperator;
  }

  static bool onlyAsMember(BuiltInOperator op);

  static int precedence(BuiltInOperator op);
  static Associativity associativity(int group);

  Type firstOperand() const;
  Type secondOperand() const;
  inline Type operandId() const { return this->firstOperand(); }

  static const std::string & getSymbol(BuiltInOperator op);

  Operator & operator=(const Operator & other);
  bool operator==(const Operator & other) const;
  bool operator!=(const Operator & other) const;
  bool operator<(const Operator & other) const { return d < other.d; }


  OperatorImpl * implementation() const;
};

} // namespace script

#endif // LIBSCRIPT_OPERATOR_H
