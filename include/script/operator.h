// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_OPERATOR_H
#define LIBSCRIPT_OPERATOR_H

#include "script/function.h"

#include "script/operators.h"

namespace script
{

class OperatorImpl;

/*!
 * \class Operator
 */

class LIBSCRIPT_API Operator : public Function
{
public:
  Operator() = default;
  Operator(const Operator&) = default;
  ~Operator() = default;

  [[deprecated("use the more general overload")]] Operator(const std::shared_ptr<OperatorImpl>& impl);
  Operator(const Function& f);

  typedef OperatorName BuiltInOperator;

  static constexpr OperatorName Null = OperatorName::InvalidOperator;

  BuiltInOperator operatorId() const;
  bool isBinary() const;

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
  static const std::string & getFullName(BuiltInOperator op);

  Operator & operator=(const Operator & other);
  bool operator==(const Operator & other) const;
  bool operator!=(const Operator & other) const;
  bool operator<(const Operator & other) const { return d < other.d; }
};

/*!
 * \endclass
 */

} // namespace script

#endif // LIBSCRIPT_OPERATOR_H
