// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/program/statements.h"

namespace script
{

namespace program
{

void BreakStatement::accept(StatementVisitor & visitor)
{
  visitor.visit(*this);
}

void CompoundStatement::accept(StatementVisitor & visitor)
{
  visitor.visit(*this);
}

void ContinueStatement::accept(StatementVisitor & visitor)
{
  visitor.visit(*this);
}

void PopDataMember::accept(StatementVisitor & visitor)
{
  visitor.visit(*this);
}

void InitObjectStatement::accept(StatementVisitor & visitor)
{
  visitor.visit(*this);
}

void ConstructionStatement::accept(StatementVisitor & visitor)
{
  visitor.visit(*this);
}


void ExpressionStatement::accept(StatementVisitor & visitor)
{
  visitor.visit(*this);
}

void ForLoop::accept(StatementVisitor & visitor)
{
  visitor.visit(*this);
}

void IfStatement::accept(StatementVisitor & visitor)
{
  visitor.visit(*this);
}

void PushDataMember::accept(StatementVisitor & visitor)
{
  visitor.visit(*this);
}

void PushGlobal::accept(StatementVisitor & visitor)
{
  visitor.visit(*this);
}

void ReturnStatement::accept(StatementVisitor & visitor)
{
  visitor.visit(*this);
}

void PushValue::accept(StatementVisitor & visitor)
{
  visitor.visit(*this);
}

void PushStaticValue::accept(StatementVisitor& visitor)
{
  visitor.visit(*this);
}

void PopValue::accept(StatementVisitor & visitor)
{
  visitor.visit(*this);
}

void WhileLoop::accept(StatementVisitor & visitor)
{
  visitor.visit(*this);
}



PushValue::PushValue(const Type & t, const std::string & name, const std::shared_ptr<Expression> & val, int si)
  : type(t)
  , name(name)
  , stackIndex(si)
  , value(val)
{

}

std::shared_ptr<PushValue> PushValue::New(const Type & t, const std::string & name, const std::shared_ptr<Expression> & val, int si)
{
  return std::make_shared<PushValue>(t, name, val, si);
}



PushStaticValue::PushStaticValue(std::string n, size_t script_id, size_t static_id, const std::shared_ptr<Expression>& val)
  : name(n),
    script_index(script_id),
    static_index(static_id),
    expr(val)
{

}

std::shared_ptr<PushStaticValue> PushStaticValue::New(std::string n, size_t script_id, size_t static_id, const std::shared_ptr<Expression>& val)
{
  return std::make_shared<PushStaticValue>(std::move(n), script_id, static_id, val);
}



PushGlobal::PushGlobal(int si, int gi)
  : script_index(si)
  , global_index(gi)
{

}

std::shared_ptr<PushGlobal> PushGlobal::New(int si, int gi)
{
  return std::make_shared<PushGlobal>(si, gi);
}



PopValue::PopValue(bool des, const Function & dtor, int si)
  : stackIndex(si)
  , destroy(des)
  , destructor(dtor)
{

}

std::shared_ptr<PopValue> PopValue::New(bool destroy, const Function & dtor, int si)
{
  return std::make_shared<PopValue>(destroy, dtor, si);
}



ExpressionStatement::ExpressionStatement(const std::shared_ptr<Expression> & e)
  : expr(e)
{

}

std::shared_ptr<ExpressionStatement> ExpressionStatement::New(const std::shared_ptr<Expression> & e)
{
  return std::make_shared<ExpressionStatement>(e);
}



CompoundStatement::CompoundStatement(std::vector<std::shared_ptr<Statement>> && list)
  : statements(std::move(list))
{

}

std::shared_ptr<CompoundStatement> CompoundStatement::New()
{
  return std::make_shared<CompoundStatement>();
}

std::shared_ptr<CompoundStatement> CompoundStatement::New(std::vector<std::shared_ptr<Statement>> && list)
{
  return std::make_shared<CompoundStatement>(std::move(list));
}



JumpStatement::JumpStatement(std::vector<std::shared_ptr<Statement>> && des)
  : destruction(des)
{

}



BreakStatement::BreakStatement(std::vector<std::shared_ptr<Statement>> && des)
  : JumpStatement(std::move(des))
{

}

std::shared_ptr<BreakStatement> BreakStatement::New()
{
  return std::make_shared<BreakStatement>();
}

std::shared_ptr<BreakStatement> BreakStatement::New(std::vector<std::shared_ptr<Statement>> && des)
{
  return std::make_shared<BreakStatement>(std::move(des));
}



ContinueStatement::ContinueStatement(std::vector<std::shared_ptr<Statement>> && des)
  : JumpStatement(std::move(des))
{

}

std::shared_ptr<ContinueStatement> ContinueStatement::New()
{
  return std::make_shared<ContinueStatement>();
}

std::shared_ptr<ContinueStatement> ContinueStatement::New(std::vector<std::shared_ptr<Statement>> && des)
{
  return std::make_shared<ContinueStatement>(std::move(des));
}



ReturnStatement::ReturnStatement(const std::shared_ptr<Expression> & e)
  : returnValue(e)
{

}

ReturnStatement::ReturnStatement(const std::shared_ptr<Expression> & e, std::vector<std::shared_ptr<Statement>> && des)
  : JumpStatement(std::move(des))
  , returnValue(e)
{

}

std::shared_ptr<ReturnStatement> ReturnStatement::New(const std::shared_ptr<Expression> & e)
{
  return std::make_shared<ReturnStatement>(e);
}

std::shared_ptr<ReturnStatement> ReturnStatement::New(const std::shared_ptr<Expression> & e, std::vector<std::shared_ptr<Statement>> && des)
{
  return std::make_shared<ReturnStatement>(e, std::move(des));
}



IfStatement::IfStatement(const std::shared_ptr<Expression> & cond, const std::shared_ptr<Statement> & bod)
  : condition(cond)
  , body(bod)
{

}

std::shared_ptr<IfStatement> IfStatement::New(const std::shared_ptr<Expression> & cond, const std::shared_ptr<Statement> & bod)
{
  return std::make_shared<IfStatement>(cond, bod);
}



IterationStatement::IterationStatement(const std::shared_ptr<Statement> & bod)
  : body(bod)
{

}



WhileLoop::WhileLoop(const std::shared_ptr<Expression> & cond, const std::shared_ptr<Statement> & bod)
  : IterationStatement(bod)
  , condition(cond)
{

}

std::shared_ptr<WhileLoop> WhileLoop::New(const std::shared_ptr<Expression> & cond, const std::shared_ptr<Statement> & bod)
{
  return std::make_shared<WhileLoop>(cond, bod);
}



ForLoop::ForLoop(const std::shared_ptr<Statement> & initialization, const std::shared_ptr<Expression> & condition, const std::shared_ptr<Expression> & loopIncr, const std::shared_ptr<Statement> & body, const std::shared_ptr<Statement> & destruction)
  : IterationStatement(body)
  , init(initialization)
  , cond(condition)
  , loop(loopIncr)
  , destroy(destruction)
{

}

std::shared_ptr<ForLoop> ForLoop::New(const std::shared_ptr<Statement> & initialization, const std::shared_ptr<Expression> & condition, const std::shared_ptr<Expression> & loopIncr, const std::shared_ptr<Statement> & body, const std::shared_ptr<Statement> & destruction)
{
  return std::make_shared<ForLoop>(initialization, condition, loopIncr, body, destruction);
}



InitObjectStatement::InitObjectStatement(Type t)
  : objectType(t)
{

}

std::shared_ptr<InitObjectStatement> InitObjectStatement::New(Type t)
{
  return std::make_shared<InitObjectStatement>(t);
}



ConstructionStatement::ConstructionStatement(Type obj_type, const Function & ctor, std::vector<std::shared_ptr<Expression>> && args)
  : object_type(obj_type),
    constructor(ctor), 
    arguments(std::move(args))
{

}

std::shared_ptr<ConstructionStatement> ConstructionStatement::New(Type obj_type, const Function & ctor, std::vector<std::shared_ptr<Expression>> && args)
{
  return std::make_shared<ConstructionStatement>(obj_type, ctor, std::move(args));
}



PushDataMember::PushDataMember(const std::shared_ptr<Expression> & val)
  : value(val)
{

}

std::shared_ptr<PushDataMember> PushDataMember::New(const std::shared_ptr<Expression> & val)
{
  return std::make_shared<PushDataMember>(val);
}



PopDataMember::PopDataMember(const Function & dtor)
  : destructor(dtor)
{

}

std::shared_ptr<PopDataMember> PopDataMember::New(const Function & dtor)
{
  return std::make_shared<PopDataMember>(dtor);
}

} // namespace program

} // namespace script
