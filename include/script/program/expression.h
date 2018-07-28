// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_PROGRAM_EXPRESSION_H
#define LIBSCRIPT_PROGRAM_EXPRESSION_H

#include "script/program/expression.h"

#include "script/function.h"
#include "script/types.h"
#include "script/context.h"

namespace script
{

namespace program
{

class ExpressionVisitor;

/// TODO : should we code the concept of prvalue, xvalue and lvalue ?
class LIBSCRIPT_API Expression
{
public:
  Expression() = default;
  Expression(const Expression &) = delete;
  ~Expression() = default;
  Expression & operator=(const Expression &) = delete;

  virtual Type type() const = 0;
  virtual Value accept(ExpressionVisitor &) = 0;

  template<typename T>
  bool is() const
  {
    return dynamic_cast<const T*>(this) != nullptr;
  }
};

struct LIBSCRIPT_API StackValue : public Expression
{
  int stackIndex;
  Type valueType;

public:
  StackValue(int si, const Type & t);
  ~StackValue() = default;

  Type type() const override;

  static std::shared_ptr<StackValue> New(int si, const Type & t);

  Value accept(ExpressionVisitor &) override;
};

struct LIBSCRIPT_API FetchGlobal : public Expression
{
  int script_index;
  int global_index;
  Type value_type;

public:
  FetchGlobal(int si, int gi, const Type & t);
  ~FetchGlobal() = default;

  Type type() const override;

  static std::shared_ptr<FetchGlobal> New(int si, int gi, const Type & t);

  Value accept(ExpressionVisitor &) override;
};



struct LIBSCRIPT_API Literal : public Expression
{
  Value value;

public:
  Literal(const Value & val);
  ~Literal() = default;

  Type type() const override;

  static std::shared_ptr<Literal> New(const Value & val);

  Value accept(ExpressionVisitor &) override;
};

struct LIBSCRIPT_API VariableAccess : public Expression
{
  Value value;

public:
  VariableAccess(const Value & val);
  ~VariableAccess() = default;

  Type type() const override;

  static std::shared_ptr<VariableAccess> New(const Value & val);

  Value accept(ExpressionVisitor &) override;
};

struct LIBSCRIPT_API LogicalOperation : public Expression
{
  std::shared_ptr<Expression> lhs;
  std::shared_ptr<Expression> rhs;

public:
  LogicalOperation(const std::shared_ptr<Expression> & a, const std::shared_ptr<Expression> & b);
  ~LogicalOperation() = default;
};

struct LIBSCRIPT_API LogicalAnd : public LogicalOperation
{
public:
  LogicalAnd(const std::shared_ptr<Expression> & a, const std::shared_ptr<Expression> & b);
  ~LogicalAnd() = default;

  Type type() const override;

  static std::shared_ptr<LogicalAnd> New(const std::shared_ptr<Expression> & a, const std::shared_ptr<Expression> & b);

  Value accept(ExpressionVisitor &) override;
};

struct LIBSCRIPT_API LogicalOr : public LogicalOperation
{
public:
  LogicalOr(const std::shared_ptr<Expression> & a, const std::shared_ptr<Expression> & b);
  ~LogicalOr() = default;

  Type type() const override;

  static std::shared_ptr<LogicalOr> New(const std::shared_ptr<Expression> & a, const std::shared_ptr<Expression> & b);

  Value accept(ExpressionVisitor &) override;
};

struct LIBSCRIPT_API ConditionalExpression : public Expression
{
  std::shared_ptr<Expression> cond;
  std::shared_ptr<Expression> onTrue;
  std::shared_ptr<Expression> onFalse;

public:
  ConditionalExpression(const std::shared_ptr<Expression> & condi, const std::shared_ptr<Expression> & ifTrue, const std::shared_ptr<Expression> & ifFalse);
  ~ConditionalExpression() = default;

  Type type() const override;

  static std::shared_ptr<ConditionalExpression> New(const std::shared_ptr<Expression> & condi, const std::shared_ptr<Expression> & ifTrue, const std::shared_ptr<Expression> & isFalse);

  Value accept(ExpressionVisitor &) override;
};

struct LIBSCRIPT_API ConstructorCall : public Expression
{
  Function constructor;
  std::vector<std::shared_ptr<Expression>> arguments;

public:
  ConstructorCall(const Function & ctor, std::vector<std::shared_ptr<Expression>> && args);
  ~ConstructorCall() = default;

  Type type() const override;

  static std::shared_ptr<ConstructorCall> New(const Function & ctor, std::vector<std::shared_ptr<Expression>> && args);

  Value accept(ExpressionVisitor &) override;
};

struct LIBSCRIPT_API CommaExpression : public Expression
{
  std::shared_ptr<Expression> lhs;
  std::shared_ptr<Expression> rhs;

public:
  CommaExpression(const std::shared_ptr<Expression> & a, const std::shared_ptr<Expression> & b);
  ~CommaExpression() = default;

  Type type() const override;

  static std::shared_ptr<CommaExpression> New(const std::shared_ptr<Expression> & a, const std::shared_ptr<Expression> & b);

  Value accept(ExpressionVisitor &) override;
};

struct LIBSCRIPT_API FunctionCall : public Expression
{
  Function callee;
  std::vector<std::shared_ptr<Expression>> args;

public: 
  FunctionCall(const Function & f, std::vector<std::shared_ptr<Expression>> && arguments);
  ~FunctionCall() = default;

  Type type() const override;

  static std::shared_ptr<FunctionCall> New(const Function & f, std::vector<std::shared_ptr<Expression>> && arguments);

  Value accept(ExpressionVisitor &) override;
};

// copies a fundamental value
struct LIBSCRIPT_API Copy : public Expression
{
  Type value_type;
  std::shared_ptr<Expression> argument;

public:
  Copy(const Type & t, const std::shared_ptr<Expression> & arg);
  ~Copy() = default;

  Type type() const override;

  static std::shared_ptr<Copy> New(const Type & t, const std::shared_ptr<Expression> & arg);

  Value accept(ExpressionVisitor &) override;
};

struct LIBSCRIPT_API FundamentalConversion : public Expression
{
  Type dest_type;
  std::shared_ptr<Expression> argument;

public:
  FundamentalConversion(const Type & t, const std::shared_ptr<Expression> & arg);
  ~FundamentalConversion() = default;

  Type type() const override;

  static std::shared_ptr<FundamentalConversion> New(const Type & t, const std::shared_ptr<Expression> & arg);

  Value accept(ExpressionVisitor &) override;
};

struct LIBSCRIPT_API VirtualCall : public Expression
{
  std::shared_ptr<Expression> object;
  int vtableIndex;
  Type returnValueType;
  std::vector<std::shared_ptr<Expression>> args;

public:
  VirtualCall(const std::shared_ptr<Expression> & obj, int methodIndex, const Type & t, std::vector<std::shared_ptr<Expression>> && arguments);
  ~VirtualCall() = default;

  Type type() const override;

  static std::shared_ptr<VirtualCall> New(const std::shared_ptr<Expression> & obj, int methodIndex, const Type & t, std::vector<std::shared_ptr<Expression>> && arguments);

  Value accept(ExpressionVisitor &) override;
};

struct LIBSCRIPT_API ArrayExpression : public Expression
{
  Type arrayType;
  std::vector<std::shared_ptr<Expression>> elements;
public:
  ArrayExpression(const Type & arrayType, std::vector<std::shared_ptr<Expression>> && elems);
  ~ArrayExpression() = default;

  Type type() const override;

  static std::shared_ptr<ArrayExpression> New(const Type & arrayType, std::vector<std::shared_ptr<Expression>> && elems);

  Value accept(ExpressionVisitor &) override;
};

struct LIBSCRIPT_API MemberAccess : public Expression
{
  Type memberType;
  std::shared_ptr<Expression> object;
  int offset;
public:
  MemberAccess(const Type & mt, const std::shared_ptr<Expression> & obj, int index);
  ~MemberAccess() = default;

  Type type() const override;

  static std::shared_ptr<MemberAccess> New(const Type & mt, const std::shared_ptr<Expression> & obj, int index);

  Value accept(ExpressionVisitor &) override;
};

class LIBSCRIPT_API LambdaExpression : public Expression
{
public:
  Type closureType;
  std::vector<std::shared_ptr<Expression>> captures;

public:
  LambdaExpression(const Type & ct, std::vector<std::shared_ptr<Expression>> && caps);
  ~LambdaExpression() = default;

  Type type() const override;

  static std::shared_ptr<LambdaExpression> New(const Type & ct, std::vector<std::shared_ptr<Expression>> && caps);

  Value accept(ExpressionVisitor &) override;
};

class LIBSCRIPT_API CaptureAccess : public Expression
{
public:
  Type captureType;
  std::shared_ptr<Expression> lambda; /// TODO : is this necessary ?
  // shouldn't this 'lambda' always be a StackValue at index 1 ? 
  int offset;

public:
  CaptureAccess(const Type & ct, const std::shared_ptr<Expression> & lam, int offset);
  ~CaptureAccess() = default;

  Type type() const override;

  static std::shared_ptr<CaptureAccess> New(const Type & ct, const std::shared_ptr<Expression> & lam, int offset);

  Value accept(ExpressionVisitor &) override;
};

struct LIBSCRIPT_API InitializerList : public Expression
{
  std::vector<std::shared_ptr<Expression>> elements;
  Type initializer_list_type;

public:
  InitializerList(std::vector<std::shared_ptr<Expression>> && elems);
  ~InitializerList() = default;

  Type type() const override;

  static std::shared_ptr<InitializerList> New(std::vector<std::shared_ptr<Expression>> && elems);

  Value accept(ExpressionVisitor &) override;
};

class LIBSCRIPT_API BindExpression : public Expression
{
public:
  std::string name;
  Context context;
  std::shared_ptr<program::Expression> value;

  BindExpression(std::string && name, const Context & con, const std::shared_ptr<program::Expression> & val);
  ~BindExpression() = default;

  Type type() const override;

  static std::shared_ptr<BindExpression> New(std::string && name, const Context & con, const std::shared_ptr<program::Expression> & val);

  Value accept(ExpressionVisitor &) override;
};

class LIBSCRIPT_API FunctionVariableCall : public Expression
{
public:
  std::shared_ptr<Expression> callee;
  Type return_type;
  std::vector<std::shared_ptr<Expression>> arguments;

  FunctionVariableCall(const std::shared_ptr<Expression> & fv, const Type & rt, std::vector<std::shared_ptr<Expression>> && args);
  ~FunctionVariableCall() = default;

  Type type() const override;

  static std::shared_ptr<FunctionVariableCall> New(const std::shared_ptr<Expression> & fv, const Type & rt, std::vector<std::shared_ptr<Expression>> && args);

  Value accept(ExpressionVisitor &) override;
};

class LIBSCRIPT_API ExpressionVisitor
{
public:
  ExpressionVisitor() = default;
  virtual ~ExpressionVisitor() = default;

  virtual Value visit(const ArrayExpression &) = 0;
  virtual Value visit(const BindExpression &) = 0;
  virtual Value visit(const CaptureAccess &) = 0;
  virtual Value visit(const CommaExpression &) = 0;
  virtual Value visit(const ConditionalExpression &) = 0;
  virtual Value visit(const ConstructorCall &) = 0;
  virtual Value visit(const Copy &) = 0;
  virtual Value visit(const FetchGlobal &) = 0;
  virtual Value visit(const FunctionCall &) = 0;
  virtual Value visit(const FunctionVariableCall &) = 0;
  virtual Value visit(const FundamentalConversion &) = 0;
  virtual Value visit(const InitializerList &) = 0;
  virtual Value visit(const LambdaExpression &) = 0;
  virtual Value visit(const Literal &) = 0;
  virtual Value visit(const LogicalAnd &) = 0;
  virtual Value visit(const LogicalOr &) = 0;
  virtual Value visit(const MemberAccess &) = 0;
  virtual Value visit(const StackValue &) = 0;
  virtual Value visit(const VariableAccess &) = 0;
  virtual Value visit(const VirtualCall &) = 0;
};

} // namespace program

} // namespace script

#endif // LIBSCRIPT_PROGRAM_EXPRESSION_H
