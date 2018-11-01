// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include <gtest/gtest.h>

#include "script/cast.h"
#include "script/castbuilder.h"
#include "script/class.h"
#include "script/classbuilder.h"
#include "script/constructorbuilder.h"
#include "script/destructorbuilder.h"
#include "script/engine.h"
#include "script/literals.h"
#include "script/literaloperatorbuilder.h"
#include "script/namespace.h"
#include "script/operator.h"
#include "script/operatorbuilder.h"

#include "script/program/expression.h"

TEST(Builders, classes) {
  using namespace script;

  Engine e;
  e.setup();

  Symbol s{ e.rootNamespace() };

  Class A = e.rootNamespace().Class("A").get();

  ASSERT_EQ(A.name(), "A");
  ASSERT_EQ(A.enclosingNamespace(), e.rootNamespace());
}

TEST(Builders, operators) {
  using namespace script;

  Engine e;
  e.setup();

  Symbol s{ e.rootNamespace() };

  Class A = e.rootNamespace().Class("A").get();

  OperatorBuilder b{ s, OperatorName::AdditionOperator };
  
  ASSERT_ANY_THROW(b.addDefaultArgument(nullptr));

  b.params(Type::cref(A.id()), Type::cref(A.id()));
  b.returns(A.id());

  Operator op = b.get();

  ASSERT_EQ(op.operatorId(), OperatorName::AdditionOperator);
  ASSERT_EQ(op.firstOperand(), Type::cref(A.id()));
  ASSERT_EQ(op.secondOperand(), Type::cref(A.id()));
  ASSERT_EQ(op.returnType(), A.id());
}

static std::shared_ptr<script::program::Expression> create_default_arg(script::Value val)
{
  using namespace script;
  val.engine()->manage(val);
  return program::VariableAccess::New(val);
}

TEST(Builders, functioncalloperator) {
  using namespace script;

  Engine e;
  e.setup();

  Symbol s{ e.rootNamespace() };

  Class A = e.rootNamespace().Class("A").get();

  FunctionCallOperatorBuilder b{ Symbol{A} };

  Operator op = b.setConst()
   .returns(Type::Int)
   .params(Type::Int, Type::Boolean)
   .addDefaultArgument(create_default_arg(e.newBool(true))).get();

  ASSERT_EQ(op.prototype().count(), 3);
  ASSERT_EQ(op.returnType(), Type::Int);
  ASSERT_EQ(op.defaultArguments().size(), 1);
  ASSERT_TRUE(op.isConst());
}


TEST(Builders, literaloperator) {
  using namespace script;

  Engine e;
  e.setup();

  Symbol s{ e.rootNamespace() };

  LiteralOperatorBuilder b{ s, "s" };

  LiteralOperator op = b.returns(Type::Int)
    .params(Type::Boolean).get();

  ASSERT_EQ(op.prototype().count(), 1);
  ASSERT_EQ(op.returnType(), Type::Int);
  ASSERT_EQ(op.enclosingNamespace(), e.rootNamespace());
}


TEST(Builders, conversionfunction) {
  using namespace script;

  Engine e;
  e.setup();

  Symbol s{ e.rootNamespace() };

  try
  {
    CastBuilder builder{ s, Type::Int };
  }
  catch (...)
  {
    ASSERT_TRUE(true);
  }

  Class A = e.rootNamespace().Class("A").get();

  CastBuilder b{ Symbol{ A }, Type::Int };

  Cast cast = b.setConst().get();

  ASSERT_EQ(cast.prototype().count(), 1);
  ASSERT_EQ(cast.returnType(), Type::Int);
  ASSERT_TRUE(cast.isConst());
  ASSERT_EQ(cast.memberOf(), A);
}



TEST(Builders, constructor) {
  using namespace script;

  Engine e;
  e.setup();

  Symbol s{ e.rootNamespace() };

  try
  {
    ConstructorBuilder builder{ s };
  }
  catch (...)
  {
    ASSERT_TRUE(true);
  }

  Class A = e.rootNamespace().Class("A").get();

  ConstructorBuilder b{ Symbol{ A } };

  ASSERT_ANY_THROW(b.returns(Type::Int));

  b.params(Type::Int, Type::Int)
    .addDefaultArgument(create_default_arg(e.newInt(0)));

  Function ctor = b.get();

  ASSERT_EQ(ctor.prototype().count(), 3);
  ASSERT_EQ(ctor.defaultArguments().size(), 1);
  ASSERT_EQ(ctor.memberOf(), A);
}


TEST(Builders, defaultconstructor) {
  using namespace script;

  Engine e;
  e.setup();

  Class A = e.rootNamespace().Class("A")
    .addMember(DataMember{Type::Int, "n"})
    .get();

  A.Constructor().setDefaulted().compile().get();
  A.Destructor().setDefaulted().compile().get();

  Value a = e.construct(A.id(), {});
  ASSERT_EQ(a.type(), A.id());

  e.destroy(a);
}


TEST(Builders, destructors) {
  using namespace script;

  Engine e;
  e.setup();

  Symbol s{ e.rootNamespace() };

  try
  {
    DestructorBuilder builder{ s };
  }
  catch (...)
  {
    ASSERT_TRUE(true);
  }

  Class A = e.rootNamespace().Class("A").get();

  DestructorBuilder b{ Symbol{ A } };

  ASSERT_ANY_THROW(b.params(Type::Int));
  ASSERT_ANY_THROW(b.returns(Type::Int));

  b.setVirtual();

  Function dtor = b.get();

  ASSERT_EQ(dtor.prototype().count(), 1);
  ASSERT_TRUE(dtor.isVirtual());
  ASSERT_EQ(dtor.memberOf(), A);
}
