// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/program/expression.h"

namespace script
{
namespace program
{

Value ArrayExpression::accept(ExpressionVisitor & visitor)
{
  return visitor.visit(*this);
}

Value BindExpression::accept(ExpressionVisitor & visitor)
{
  return visitor.visit(*this);
}

Value CaptureAccess::accept(ExpressionVisitor & visitor)
{
  return visitor.visit(*this);
}

Value CommaExpression::accept(ExpressionVisitor & visitor)
{
  return visitor.visit(*this);
}

Value ConditionalExpression::accept(ExpressionVisitor & visitor)
{
  return visitor.visit(*this);
}

Value ConstructorCall::accept(ExpressionVisitor & visitor)
{
  return visitor.visit(*this);
}

Value Copy::accept(ExpressionVisitor & visitor)
{
  return visitor.visit(*this);
}

Value FetchGlobal::accept(ExpressionVisitor & visitor)
{
  return visitor.visit(*this);
}

Value FunctionCall::accept(ExpressionVisitor & visitor)
{
  return visitor.visit(*this);
}

Value FunctionVariableCall::accept(ExpressionVisitor & visitor)
{
  return visitor.visit(*this);
}

Value FundamentalConversion::accept(ExpressionVisitor & visitor)
{
  return visitor.visit(*this);
}

Value InitializerList::accept(ExpressionVisitor & visitor)
{
  throw std::runtime_error{ "Implementation error : InitializerList::accept() should never be called." };
}

Value LambdaExpression::accept(ExpressionVisitor & visitor)
{
  return visitor.visit(*this);
}

Value Literal::accept(ExpressionVisitor & visitor)
{
  return visitor.visit(*this);
}

Value LogicalAnd::accept(ExpressionVisitor & visitor)
{
  return visitor.visit(*this);
}

Value LogicalOr::accept(ExpressionVisitor & visitor)
{
  return visitor.visit(*this);
}

Value MemberAccess::accept(ExpressionVisitor & visitor)
{
  return visitor.visit(*this);
}

Value StackValue::accept(ExpressionVisitor & visitor)
{
  return visitor.visit(*this);
}

Value VariableAccess::accept(ExpressionVisitor & visitor)
{
  return visitor.visit(*this);
}

Value VirtualCall::accept(ExpressionVisitor & visitor)
{
  return visitor.visit(*this);
}





StackValue::StackValue(int si, const Type & t)
  : stackIndex(si)
  , valueType(t)
{

}

Type StackValue::type() const
{
  return Type::ref(this->valueType);
}

std::shared_ptr<StackValue> StackValue::New(int si, const Type & t)
{
  return std::make_shared<StackValue>(si, t);
}



FetchGlobal::FetchGlobal(int si, int gi, const Type & t)
  : script_index(si)
  , global_index(gi)
  , value_type(t)
{

}

Type FetchGlobal::type() const
{
  return value_type;
}

std::shared_ptr<FetchGlobal> FetchGlobal::New(int si, int gi, const Type & t)
{
  return std::make_shared<FetchGlobal>(si, gi, t);
}



Literal::Literal(const Value & val)
  : value(val)
{

}

Type Literal::type() const
{
  return Type::cref(this->value.type());
}

std::shared_ptr<Literal> Literal::New(const Value & val)
{
  return std::make_shared<Literal>(val);
}



VariableAccess::VariableAccess(const Value & val)
  : value(val)
{

}

Type VariableAccess::type() const
{
  return Type::ref(this->value.type());
}

std::shared_ptr<VariableAccess> VariableAccess::New(const Value & val)
{
  return std::make_shared<VariableAccess>(val);
}



LogicalOperation::LogicalOperation(const std::shared_ptr<Expression> & a, const std::shared_ptr<Expression> & b)
  : lhs(a)
  , rhs(b)
{

}



LogicalAnd::LogicalAnd(const std::shared_ptr<Expression> & a, const std::shared_ptr<Expression> & b)
  : LogicalOperation(a, b)
{

}

Type LogicalAnd::type() const
{
  return Type::cref(Type::Boolean);
}

std::shared_ptr<LogicalAnd> LogicalAnd::New(const std::shared_ptr<Expression> & a, const std::shared_ptr<Expression> & b)
{
  return std::make_shared<LogicalAnd>(a, b);
}



LogicalOr::LogicalOr(const std::shared_ptr<Expression> & a, const std::shared_ptr<Expression> & b)
  : LogicalOperation(a, b)
{

}

Type LogicalOr::type() const
{
  return Type::cref(Type::Boolean);
}

std::shared_ptr<LogicalOr> LogicalOr::New(const std::shared_ptr<Expression> & a, const std::shared_ptr<Expression> & b)
{
  return std::make_shared<LogicalOr>(a, b);
}



ConditionalExpression::ConditionalExpression(const std::shared_ptr<Expression> & condi, const std::shared_ptr<Expression> & ifTrue, const std::shared_ptr<Expression> & ifFalse)
  : cond(condi)
  , onTrue(ifTrue)
  , onFalse(ifFalse)
{

}

Type ConditionalExpression::type() const
{
  return this->onTrue->type();
}

std::shared_ptr<ConditionalExpression> ConditionalExpression::New(const std::shared_ptr<Expression> & condi, const std::shared_ptr<Expression> & ifTrue, const std::shared_ptr<Expression> & ifFalse)
{
  return std::make_shared<ConditionalExpression>(condi, ifTrue, ifFalse);
}



ConstructorCall::ConstructorCall(const Function & ctor, std::vector<std::shared_ptr<Expression>> && args)
  : constructor(ctor)
  , arguments(std::move(args))
{

}

Type ConstructorCall::type() const
{
  /// TODO : is that correct, should we remove const-ref ?
  return constructor.returnType();
}

std::shared_ptr<ConstructorCall> ConstructorCall::New(const Function & ctor, std::vector<std::shared_ptr<Expression>> && args)
{
  return std::make_shared<ConstructorCall>(ctor, std::move(args));
}



CommaExpression::CommaExpression(const std::shared_ptr<Expression> & a, const std::shared_ptr<Expression> & b)
  : lhs(a)
  , rhs(a)
{

}

Type CommaExpression::type() const
{
  return rhs->type();
}

std::shared_ptr<CommaExpression> CommaExpression::New(const std::shared_ptr<Expression> & a, const std::shared_ptr<Expression> & b)
{
  return std::make_shared<CommaExpression>(a, b);
}



FunctionCall::FunctionCall(const Function & f, std::vector<std::shared_ptr<Expression>> && arguments)
  : callee(f)
  , args(std::move(arguments))
{
  assert(f.prototype().count() == args.size());
}

Type FunctionCall::type() const
{
  return this->callee.prototype().returnType();
}

std::shared_ptr<FunctionCall> FunctionCall::New(const Function & f, std::vector<std::shared_ptr<Expression>> && arguments)
{
  return std::make_shared<FunctionCall>(f, std::move(arguments));
}



Copy::Copy(const Type & t, const std::shared_ptr<Expression> & arg)
  : value_type(t)
  , argument(arg)
{

}

Type Copy::type() const
{
  return value_type;
}

std::shared_ptr<Copy> Copy::New(const Type & t, const std::shared_ptr<Expression> & arg)
{
  return std::make_shared<Copy>(t, arg);
}



FundamentalConversion::FundamentalConversion(const Type & t, const std::shared_ptr<Expression> & arg)
  : dest_type(t)
  , argument(arg)
{

}

Type FundamentalConversion::type() const
{
  return dest_type;
}

std::shared_ptr<FundamentalConversion> FundamentalConversion::New(const Type & t, const std::shared_ptr<Expression> & arg)
{
  return std::make_shared<FundamentalConversion>(t, arg);
}



VirtualCall::VirtualCall(const std::shared_ptr<Expression> & obj, int methodIndex, const Type & t, std::vector<std::shared_ptr<Expression>> && arguments)
  : object(obj)
  , vtableIndex(methodIndex)
  , returnValueType(t)
  , args(std::move(arguments))
{

}

Type VirtualCall::type() const
{
  return this->returnValueType;
}


std::shared_ptr<VirtualCall> VirtualCall::New(const std::shared_ptr<Expression> & obj, int methodIndex, const Type & t, std::vector<std::shared_ptr<Expression>> && arguments)
{
  return std::make_shared<VirtualCall>(obj, methodIndex, t, std::move(arguments));
}




ArrayExpression::ArrayExpression(const Type & arrayType, std::vector<std::shared_ptr<Expression>> && elems)
  : arrayType(arrayType)
  , elements(std::move(elems))
{

}

Type ArrayExpression::type() const
{
  return Type::cref(this->arrayType);
}

std::shared_ptr<ArrayExpression> ArrayExpression::New(const Type & arrayType, std::vector<std::shared_ptr<Expression>> && elems)
{
  return std::make_shared<ArrayExpression>(arrayType, std::move(elems));
}



MemberAccess::MemberAccess(const Type & mt, const std::shared_ptr<Expression> & obj, int index)
  : memberType(mt)
  , object(obj)
  , offset(index)
{

}

Type MemberAccess::type() const
{
  return this->memberType;
}

std::shared_ptr<MemberAccess> MemberAccess::New(const Type & mt, const std::shared_ptr<Expression> & obj, int index)
{
  return std::make_shared<MemberAccess>(mt, obj, index);
}



LambdaExpression::LambdaExpression(const Type & ct, std::vector<std::shared_ptr<Expression>> && caps)
  : closureType(ct)
  , captures(std::move(caps))
{

}

Type LambdaExpression::type() const
{
  return this->closureType;
}

std::shared_ptr<LambdaExpression> LambdaExpression::New(const Type & ct, std::vector<std::shared_ptr<Expression>> && caps)
{
  return std::make_shared<LambdaExpression>(ct, std::move(caps));
}



CaptureAccess::CaptureAccess(const Type & ct, const std::shared_ptr<Expression> & lam, int index)
  : captureType(ct)
  , lambda(lam)
  , offset(index)
{

}

Type CaptureAccess::type() const
{
  return this->captureType;
}

std::shared_ptr<CaptureAccess> CaptureAccess::New(const Type & ct, const std::shared_ptr<Expression> & lam, int offset)
{
  return std::make_shared<CaptureAccess>(ct, lam, offset);
}



InitializerList::InitializerList(std::vector<std::shared_ptr<Expression>> && elems)
  : elements(std::move(elems))
{

}

Type InitializerList::type() const
{
  return Type::InitializerList;
}

std::shared_ptr<InitializerList> InitializerList::New(std::vector<std::shared_ptr<Expression>> && elems)
{
  return std::make_shared<InitializerList>(std::move(elems));
}



BindExpression::BindExpression(std::string && n, const Context & con, const std::shared_ptr<program::Expression> & val)
  : name(std::move(n))
  , context(con)
  , value(val)
{

}

Type BindExpression::type() const
{
  return this->value->type();
}

std::shared_ptr<BindExpression> BindExpression::New(std::string && name, const Context & con, const std::shared_ptr<program::Expression> & val)
{
  return std::make_shared<program::BindExpression>(std::move(name), con, val);
}



FunctionVariableCall::FunctionVariableCall(const std::shared_ptr<Expression> & fv, const Type & rt, std::vector<std::shared_ptr<Expression>> && args)
  : callee(fv)
  , return_type(rt)
  , arguments(std::move(args))
{

}

Type FunctionVariableCall::type() const
{
  return return_type;
}

std::shared_ptr<FunctionVariableCall> FunctionVariableCall::New(const std::shared_ptr<Expression> & fv, const Type & rt, std::vector<std::shared_ptr<Expression>> && args)
{
  return std::make_shared<FunctionVariableCall>(fv, rt, std::move(args));
}

} // namespace program

} // namespace script
