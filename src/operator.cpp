// Copyright (C) 2018-2020 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/operator.h"
#include "script/private/operator_p.h"

#include "script/name.h"

namespace script
{

/*!
 * \fn int precedence(OperatorName op)
 * \param the name of an operator
 * \brief returns the precedence group of an operator
 * 
 * For any valid operator name, this function returns the precedence group 
 * of the operator.
 * This is a value between 1 and 15 with 1 being the operator of highest 
 * precedence and 15 of lowest precedence.
 */
int precedence(OperatorName op)
{
  if (op == ScopeResolutionOperator)
    return 1;
  else if (PostIncrementOperator <= op && op <= MemberAccessOperator)
    return 2;
  else if (PreIncrementOperator <= op && op <= BitwiseNot)
    return 3;
  else if (MultiplicationOperator <= op && op <= RemainderOperator)
    return 4;
  else if (AdditionOperator == op || op == SubstractionOperator)
    return 5;
  else if (LeftShiftOperator == op || op == RightShiftOperator)
    return 6;
  else if (LessOperator <= op && op <= GreaterEqualOperator)
    return 7;
  else if (EqualOperator == op || op == InequalOperator)
    return 8;
  else if (op == BitwiseAndOperator)
    return 9;
  else if (op == BitwiseXorOperator)
    return 10;
  else if (op == BitwiseOrOperator)
    return 11;
  else if (op == LogicalAndOperator)
    return 12;
  else if (op == LogicalOrOperator)
    return 13;
  else if (ConditionalOperator <= op && op <= BitwiseXorAssignmentOperator)
    return 14;
  else if (op == CommaOperator)
    return 15;
  return 0;
}

/*!
 * \fn Associativity associativity(int group)
 * \param the id of a precedence group
 * \brief returns the associativity of a an operator given its group
 *
 * The input parameter must be a value returned by the function precedence().
 */
Associativity associativity(int group)
{
  assert(group >= 0 && group <= 15);

  static Associativity table[] = {
    Associativity::LeftToRight,
    Associativity::LeftToRight,
    Associativity::RightToLeft,
    Associativity::LeftToRight,
    Associativity::LeftToRight,
    Associativity::LeftToRight,
    Associativity::LeftToRight,
    Associativity::LeftToRight,
    Associativity::LeftToRight,
    Associativity::LeftToRight,
    Associativity::LeftToRight,
    Associativity::LeftToRight,
    Associativity::LeftToRight,
    Associativity::RightToLeft,
    Associativity::LeftToRight,
  };

  return table[group - 1];
}

OperatorImpl::OperatorImpl(OperatorName op, Engine *engine, FunctionFlags flags)
  : FunctionImpl(engine, flags)
  , operatorId(op)
{

}

SymbolKind OperatorImpl::get_kind() const
{
  return SymbolKind::Operator;
}

Name OperatorImpl::get_name() const
{
  return operatorId;
}

bool OperatorImpl::is_native() const
{
  return false;
}

std::shared_ptr<program::Statement> OperatorImpl::body() const
{
  return program_;
}

void OperatorImpl::set_body(std::shared_ptr<program::Statement> b)
{
  program_ = b;
}



UnaryOperatorImpl::UnaryOperatorImpl(OperatorName op, const Prototype & proto, Engine *engine, FunctionFlags flags)
  : OperatorImpl(op, engine, flags)
  , proto_(proto.returnType(), proto.at(0))
{

}

const Prototype & UnaryOperatorImpl::prototype() const
{
  return this->proto_;
}

void UnaryOperatorImpl::set_return_type(const Type & t)
{
  this->proto_.setReturnType(t);
}

BinaryOperatorImpl::BinaryOperatorImpl(OperatorName op, const Prototype & proto, Engine *engine, FunctionFlags flags)
  : OperatorImpl(op, engine, flags)
  , proto_(proto.returnType(), proto.at(0), proto.at(1))
{

}

const Prototype & BinaryOperatorImpl::prototype() const
{
  return this->proto_;
}

void BinaryOperatorImpl::set_return_type(const Type & t)
{
  this->proto_.setReturnType(t);
}

FunctionCallOperatorImpl::FunctionCallOperatorImpl(OperatorName op, const Prototype & proto, Engine *engine, FunctionFlags flags)
  : OperatorImpl(op, engine, flags)
  , proto_(proto)
{

}

FunctionCallOperatorImpl::FunctionCallOperatorImpl(OperatorName op, DynamicPrototype && proto, Engine *engine, FunctionFlags flags)
  : OperatorImpl(op, engine, flags)
  , proto_(std::move(proto))
{

}

const Prototype & FunctionCallOperatorImpl::prototype() const
{
  return this->proto_;
}

void FunctionCallOperatorImpl::set_return_type(const Type & t)
{
  this->proto_.setReturnType(t);
}

const std::vector<DefaultArgument> & FunctionCallOperatorImpl::default_arguments() const
{
  return defaultargs_;
}

void FunctionCallOperatorImpl::set_default_arguments(std::vector<DefaultArgument> defaults)
{
  defaultargs_ = std::move(defaults);
}

/*!
 * \class Operator
 */

Operator::Operator(const std::shared_ptr<OperatorImpl> & impl)
  : Function(impl)
{

}

Operator::Operator(const Function& f)
  : Function(f.isOperator() ? f.impl() : nullptr)
{

}

/*!
 * \fn OperatorName operatorId() const
 * \brief returns the operator name
 */
OperatorName Operator::operatorId() const
{
  return d ? d->get_name().operatorName() : OperatorName::InvalidOperator;
}

/*!
 * \fn bool isBinary() const
 * \brief returns whether the operator is a binary operator
 */
bool Operator::isBinary() const
{
  return d->prototype().count() == 2;
}

/*!
 * \fn static bool isBinary(BuiltInOperator op)
 * \brief returns whether the operator is a binary operator
 */
bool Operator::isBinary(BuiltInOperator op)
{
  return !isUnary(op) && op != ConditionalOperator
    && op != FunctionCallOperator;
}

/*!
 * \fn static bool isUnary(BuiltInOperator op)
 * \brief returns whether the operator is an unary operator
 */
bool Operator::isUnary(BuiltInOperator op)
{
  return op == PostIncrementOperator ||
    op == PostDecrementOperator ||
    op == PreIncrementOperator ||
    op == PreDecrementOperator ||
    op == UnaryPlusOperator ||
    op == UnaryMinusOperator ||
    op == LogicalNotOperator ||
    op == BitwiseNot;
}

/*!
 * \fn static bool onlyAsMember(BuiltInOperator op)
 * \brief returns whether the operator can only be defined as a member
 */
bool Operator::onlyAsMember(BuiltInOperator op)
{
  return op == AssignmentOperator || op == FunctionCallOperator || op == SubscriptOperator;
}

/*!
 * \fn static int precedence(BuiltInOperator op)
 * \brief returns the precedence of an operator
 */
int Operator::precedence(BuiltInOperator op)
{
  return script::precedence(op);
}

/*!
 * \fn static Associativity associativity(int group)
 * \brief returns the associativity of an operator given its precedence group
 */
Associativity Operator::associativity(int group)
{
  return script::associativity(group);
}

/*!
 * \fn Type firstOperand() const
 * \brief returns the first operand of the operator
 */
Type Operator::firstOperand() const
{
  return d->prototype().at(0);
}

/*!
 * \fn Type secondOperand() const
 * \brief returns the second operand of the operator
 */
Type Operator::secondOperand() const
{
  return d->prototype().at(1);
}

/*!
 * \fn static const std::string& getSymbol(BuiltInOperator op)
 * \brief returns a string representation of the operator
 */
const std::string& Operator::getSymbol(BuiltInOperator op)
{
  if (op == Null)
    throw std::runtime_error{ "Invalid operator" };

  static const std::string names[] = {
    "", "::", "++", "--", "()", "[]", ".", "++", "--", "+",
    "-", "!", "~", "*", "/", "%", "+", "-", "<<", ">>",
    "<", ">", "<=", ">=", "==", "!=", "&", "^", "|", "&&",
    "||", "?:", "=", "*=", "/=",  "%=", "+=", "-=", "<<=", ">>=", 
    "&=", "|=", "^=", ",",
  };

  return names[static_cast<int>(op)];
}

const std::string & Operator::getFullName(BuiltInOperator op)
{
  if (op == Null)
    throw std::runtime_error{ "Invalid operator" };

  static const std::string names[] = {
    "", "operator::", "operator++", "operator--", "operator()", "operator[]", "operator.",
    "operator++","operator--","operator+","operator-", "operator!","operator~", "operator*",
    "operator/","operator%", "operator+", "operator-", "operator<<", "operator>>", "operator<",
    "operator>","operator<=","operator>=", "operator==",  "operator!=", "operator&", "operator^",
    "operator|","operator&&", "operator||","operator?:", "operator=","operator*=","operator/=",
    "operator%=","operator+=","operator-=","operator<<=","operator>>=", "operator&=",  "operator|=",
    "operator^=", "operator,",
  };

  return names[static_cast<int>(op)];
}

Operator & Operator::operator=(const Operator & other)
{
  d = other.d;
  return *(this);
}

bool Operator::operator==(const Operator & other) const
{
  return d == other.d;
}

bool Operator::operator!=(const Operator & other) const
{
  return d != other.d;
}

/*!
 * \endclass
 */

} // namespace script
