// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_PROGRAM_STATEMENT_H
#define LIBSCRIPT_PROGRAM_STATEMENT_H

#include "script/program/expression.h"

namespace script
{

class ScriptImpl;

namespace compiler
{
class DebugInfoBlock;
} // namespace compiler

namespace program
{

class StatementVisitor;

class LIBSCRIPT_API Statement
{
public:
  Statement() = default;
  Statement(const Statement &) = delete;
  virtual ~Statement() = default;
  Statement & operator=(const Statement &) = delete;

  virtual void accept(StatementVisitor &) = 0;

  template<typename T>
  bool is() const
  {
    return dynamic_cast<const T*>(this) != nullptr;
  }
};


struct LIBSCRIPT_API PushGlobal : public Statement
{
  int script_index;
  int global_index;

public:
  PushGlobal(int si, int gi);
  ~PushGlobal() = default;

  static std::shared_ptr<PushGlobal> New(int si, int gi);

  void accept(StatementVisitor &) override;
};

struct LIBSCRIPT_API PushValue : public Statement
{
  Type type; // unused, for debugging purpose ?
  std::string name;
  int stackIndex; // unused, for debugging purpose ?
  std::shared_ptr<Expression> value;

public:
  PushValue(const Type & t, const std::string & name, const std::shared_ptr<Expression> & val, int si);
  ~PushValue() = default;

  static std::shared_ptr<PushValue> New(const Type & t, const std::string & name, const std::shared_ptr<Expression> & val, int si = -1);

  void accept(StatementVisitor &) override;
};

struct LIBSCRIPT_API PushStaticValue : public Statement
{
  std::string name;
  size_t script_index;
  size_t static_index;
  std::shared_ptr<Expression> expr;

public:
  PushStaticValue(std::string n, size_t script_id, size_t static_id, const std::shared_ptr<Expression>& val);
  ~PushStaticValue() = default;

  static std::shared_ptr<PushStaticValue> New(std::string n, size_t script_id, size_t static_id, const std::shared_ptr<Expression>& val);

  void accept(StatementVisitor&) override;
};

class LIBSCRIPT_API PopValue : public Statement
{
public:
  int stackIndex;// unused, for debugging purpose ?
  bool destroy;
  Function destructor;
public:
  PopValue(bool destroy, const Function & dtor, int si);
  ~PopValue() = default;

  static std::shared_ptr<PopValue> New(bool destroy, const Function & dtor, int si);

  void accept(StatementVisitor &) override;
};

class LIBSCRIPT_API ExpressionStatement : public Statement
{
public:
  std::shared_ptr<Expression> expr;

public:
  ExpressionStatement(const std::shared_ptr<Expression> & e);
  ~ExpressionStatement() = default;

  static std::shared_ptr<ExpressionStatement> New(const std::shared_ptr<Expression> & e);

  void accept(StatementVisitor &) override;
};

class LIBSCRIPT_API CompoundStatement : public Statement
{
public:
  std::vector<std::shared_ptr<Statement>> statements;

public:
  CompoundStatement() = default;
  ~CompoundStatement() = default;
  CompoundStatement(std::vector<std::shared_ptr<Statement>> && list);

  static std::shared_ptr<CompoundStatement> New();
  static std::shared_ptr<CompoundStatement> New(std::vector<std::shared_ptr<Statement>> && list);

  void accept(StatementVisitor &) override;
};

class LIBSCRIPT_API JumpStatement : public Statement
{
public:
  std::vector<std::shared_ptr<Statement>> destruction;

public:
  JumpStatement() = default;
  ~JumpStatement() = default;

  JumpStatement(std::vector<std::shared_ptr<Statement>> && des);
};

struct LIBSCRIPT_API BreakStatement : public JumpStatement
{
public:
  BreakStatement() = default;
  ~BreakStatement() = default;

  BreakStatement(std::vector<std::shared_ptr<Statement>> && des);

  static std::shared_ptr<BreakStatement> New();
  static std::shared_ptr<BreakStatement> New(std::vector<std::shared_ptr<Statement>> && des);

  void accept(StatementVisitor &) override;
};

struct LIBSCRIPT_API ContinueStatement : public JumpStatement
{
public:
  ContinueStatement() = default;
  ~ContinueStatement() = default;

  ContinueStatement(std::vector<std::shared_ptr<Statement>> && des);

  static std::shared_ptr<ContinueStatement> New();
  static std::shared_ptr<ContinueStatement> New(std::vector<std::shared_ptr<Statement>> && des);

  void accept(StatementVisitor &) override;
};

struct LIBSCRIPT_API ReturnStatement : public JumpStatement
{
  std::shared_ptr<Expression> returnValue;

public:
  ReturnStatement(const std::shared_ptr<Expression> & e);
  ReturnStatement(const std::shared_ptr<Expression> & e, std::vector<std::shared_ptr<Statement>> && des);
  ~ReturnStatement() = default;

  static std::shared_ptr<ReturnStatement> New(const std::shared_ptr<Expression> & e);
  static std::shared_ptr<ReturnStatement> New(const std::shared_ptr<Expression> & e, std::vector<std::shared_ptr<Statement>> && des);

  void accept(StatementVisitor &) override;
};

class LIBSCRIPT_API CppReturnStatement : public Statement
{
public:
  NativeFunctionSignature native_fun;

  explicit CppReturnStatement(NativeFunctionSignature natfun);
  ~CppReturnStatement() = default;

  void accept(StatementVisitor&) override;
};

class LIBSCRIPT_API SelectionStatement : public Statement
{
public:
  SelectionStatement() = default;
  ~SelectionStatement() = default;
};

struct LIBSCRIPT_API IfStatement : public SelectionStatement
{
  std::shared_ptr<Expression> condition;
  std::shared_ptr<Statement> body;
  std::shared_ptr<Statement> elseClause;

public:
  IfStatement(const std::shared_ptr<Expression> & cond, const std::shared_ptr<Statement> & bod);
  ~IfStatement() = default;

  static std::shared_ptr<IfStatement> New(const std::shared_ptr<Expression> & cond, const std::shared_ptr<Statement> & bod);

  void accept(StatementVisitor &) override;
};

class LIBSCRIPT_API IterationStatement : public Statement
{
public:
  std::shared_ptr<Statement> body;

public:
  IterationStatement(const std::shared_ptr<Statement> & bod);
  ~IterationStatement() = default;
};

struct LIBSCRIPT_API WhileLoop : public IterationStatement
{
  std::shared_ptr<Expression> condition;

public:
  WhileLoop(const std::shared_ptr<Expression> & cond, const std::shared_ptr<Statement> & bod);
  ~WhileLoop() = default;

  static std::shared_ptr<WhileLoop> New(const std::shared_ptr<Expression> & cond, const std::shared_ptr<Statement> & bod);

  void accept(StatementVisitor &) override;
};

struct LIBSCRIPT_API ForLoop : public IterationStatement
{
  std::shared_ptr<Statement> init;
  std::shared_ptr<Expression> cond;
  std::shared_ptr<Expression> loop;
  std::shared_ptr<Statement> destroy;

public:
  ForLoop(const std::shared_ptr<Statement> & initialization, const std::shared_ptr<Expression> & condition, const std::shared_ptr<Expression> & loopIncr, const std::shared_ptr<Statement> & body, const std::shared_ptr<Statement> & destruction);
  ~ForLoop() = default;

  static std::shared_ptr<ForLoop> New(const std::shared_ptr<Statement> & initialization, const std::shared_ptr<Expression> & condition, const std::shared_ptr<Expression> & loopIncr, const std::shared_ptr<Statement> & body, const std::shared_ptr<Statement> & destruction);

  void accept(StatementVisitor &) override;
};

struct LIBSCRIPT_API InitObjectStatement : public Statement
{
  Type objectType;
public:
  InitObjectStatement(Type t);
  ~InitObjectStatement() = default;

  static std::shared_ptr<InitObjectStatement> New(Type t);

  void accept(StatementVisitor &) override;
};

// represent a call to a base constructor or a delegate constructor
struct LIBSCRIPT_API ConstructionStatement : public Statement
{
  Type object_type;
  Function constructor;
  std::vector<std::shared_ptr<Expression>> arguments;
public:
  ConstructionStatement(Type obj_type, const Function & ctor, std::vector<std::shared_ptr<Expression>> && args);
  ~ConstructionStatement() = default;

  static std::shared_ptr<ConstructionStatement> New(Type obj_type, const Function & ctor, std::vector<std::shared_ptr<Expression>> && args);

  void accept(StatementVisitor &) override;
};

struct LIBSCRIPT_API PushDataMember : public Statement
{
  std::shared_ptr<Expression> value;

public:
  PushDataMember(const std::shared_ptr<Expression> & val);
  ~PushDataMember() = default;

  static std::shared_ptr<PushDataMember> New(const std::shared_ptr<Expression> & val);

  void accept(StatementVisitor &) override;
};

struct LIBSCRIPT_API PopDataMember : public Statement
{
  Function destructor;

public:
  PopDataMember(const Function & dtor);
  ~PopDataMember() = default;

  static std::shared_ptr<PopDataMember> New(const Function & dtor);

  void accept(StatementVisitor &) override;
};

struct LIBSCRIPT_API Breakpoint : public Statement
{
  const int line = -1;
  bool leading = false; // @TODO: what is the purpose of this variable ?
  const std::shared_ptr<compiler::DebugInfoBlock> debug_info;
  int status = 0; // user-variable

public:
  Breakpoint(int l, std::shared_ptr<compiler::DebugInfoBlock> dbg);
  ~Breakpoint() = default;

  void accept(StatementVisitor&) override;
};

class LIBSCRIPT_API StatementVisitor
{
public:
  StatementVisitor() = default;
  virtual ~StatementVisitor() = default;

  virtual void visit(const BreakStatement &) = 0;
  virtual void visit(const CompoundStatement &) = 0;
  virtual void visit(const ContinueStatement &) = 0;
  virtual void visit(const PopDataMember &) = 0;
  virtual void visit(const InitObjectStatement &) = 0;
  virtual void visit(const ConstructionStatement &) = 0;
  virtual void visit(const ExpressionStatement &) = 0;
  virtual void visit(const ForLoop &) = 0;
  virtual void visit(const IfStatement &) = 0;
  virtual void visit(const PushDataMember &) = 0;
  virtual void visit(const PushGlobal &) = 0;
  virtual void visit(const PushValue&) = 0;
  virtual void visit(const PushStaticValue&) = 0;
  virtual void visit(const ReturnStatement &) = 0;
  virtual void visit(const CppReturnStatement&) = 0;
  virtual void visit(const PopValue &) = 0;
  virtual void visit(const WhileLoop&) = 0;
  virtual void visit(const Breakpoint&) = 0;
};

} // namespace program

} // namespace script

#endif // LIBSCRIPT_PROGRAM_STATEMENT_H
