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

  Class A = e.rootNamespace().newClass("A").get();

  ASSERT_EQ(A.name(), "A");
  ASSERT_EQ(A.enclosingNamespace(), e.rootNamespace());
}

TEST(Builders, operators) {
  using namespace script;

  Engine e;
  e.setup();

  Symbol s{ e.rootNamespace() };

  Class A = e.rootNamespace().newClass("A").get();

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
  return program::VariableAccess::New(val);
}

TEST(Builders, functioncalloperator) {
  using namespace script;

  Engine e;
  e.setup();

  Symbol s{ e.rootNamespace() };

  Class A = e.rootNamespace().newClass("A").get();

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

  Namespace ns = e.rootNamespace();

  LiteralOperator op = ns.newUserDefinedLiteral("s", Type::Boolean, Type::Int).get();

  ASSERT_EQ(op.suffix(), "s");
  ASSERT_EQ(op.prototype().count(), 1);
  ASSERT_EQ(op.returnType(), Type::Int);
  ASSERT_EQ(op.input(), Type::Boolean);
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

  Class A = e.rootNamespace().newClass("A").get();

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

  Class A = e.rootNamespace().newClass("A").get();

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

  Class A = e.rootNamespace().newClass("A")
    .addMember(DataMember{Type::Int, "n"})
    .get();

  A.newConstructor().setDefaulted().compile().get();
  A.newDestructor().setDefaulted().compile().get();

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

  Class A = e.rootNamespace().newClass("A").get();

  DestructorBuilder b{ Symbol{ A } };

  ASSERT_ANY_THROW(b.params(Type::Int));
  ASSERT_ANY_THROW(b.returns(Type::Int));

  b.setVirtual();

  Function dtor = b.get();

  ASSERT_EQ(dtor.prototype().count(), 1);
  ASSERT_TRUE(dtor.isVirtual());
  ASSERT_EQ(dtor.memberOf(), A);
}


TEST(Builders, builder_functions) {
  using namespace script;

  Engine engine;
  engine.setup();

  Class A = Symbol{ engine.rootNamespace() }.newClass("A").get();
  Type A_type = A.id();

  /* Constructors */

  Function default_ctor = A.newConstructor().get();
  ASSERT_TRUE(default_ctor.isConstructor());
  ASSERT_EQ(default_ctor.memberOf(), A);
  ASSERT_EQ(default_ctor, A.defaultConstructor());

  Function copy_ctor = A.newConstructor().params(Type::cref(A_type)).get();
  ASSERT_TRUE(copy_ctor.isConstructor());
  ASSERT_EQ(copy_ctor.memberOf(), A);
  ASSERT_EQ(copy_ctor, A.copyConstructor());

  Function ctor_1 = A.newConstructor().params(Type::Int).get();
  ASSERT_TRUE(ctor_1.isConstructor());
  ASSERT_EQ(ctor_1.memberOf(), A);
  ASSERT_EQ(ctor_1.prototype().count(), 2);
  ASSERT_EQ(ctor_1.parameter(1), Type::Int);
  ASSERT_FALSE(ctor_1.isExplicit());

  Function ctor_2 = A.newConstructor().setExplicit().params(Type::Boolean).get();
  ASSERT_TRUE(ctor_2.isConstructor());
  ASSERT_EQ(ctor_2.memberOf(), A);
  ASSERT_EQ(ctor_2.prototype().count(), 2);
  ASSERT_EQ(ctor_2.parameter(1), Type::Boolean);
  ASSERT_TRUE(ctor_2.isExplicit());

  ASSERT_EQ(A.constructors().size(), 4);

  /* Conversion functions */

  Cast cast_1 = A.newConversion(Type::cref(Type::Int)).setConst().get();
  ASSERT_TRUE(cast_1.isMemberFunction());
  ASSERT_EQ(cast_1.memberOf(), A);
  ASSERT_TRUE(cast_1.isConst());
  ASSERT_EQ(cast_1.destType(), Type::cref(Type::Int));
  ASSERT_EQ(cast_1.destType(), cast_1.returnType());
  ASSERT_FALSE(cast_1.isExplicit());

  Cast cast_2 = A.newConversion(Type::ref(Type::Int)).setExplicit().get();
  ASSERT_TRUE(cast_2.isMemberFunction());
  ASSERT_EQ(cast_2.memberOf(), A);
  ASSERT_FALSE(cast_2.isConst());
  ASSERT_EQ(cast_2.destType(), Type::ref(Type::Int));
  ASSERT_TRUE(cast_2.isExplicit());
}


TEST(Builders, datamembers) {
  using namespace script;

  Engine engine;
  engine.setup();

  auto b = Symbol{ engine.rootNamespace() }.newClass("A")
    .addMember(Class::DataMember{ Type::Int, "a" });

  Class A = b.get();

  ASSERT_EQ(A.dataMembers().size(), 1);
  ASSERT_EQ(A.dataMembers().front().type, Type::Int);
  ASSERT_EQ(A.dataMembers().front().name, "a");

  ASSERT_EQ(A.cumulatedDataMemberCount(), 1);
  ASSERT_EQ(A.attributesOffset(), 0);

  b = Symbol{ engine.rootNamespace() }.newClass("B")
    .setBase(A.id())
    .addMember(Class::DataMember{ Type::Boolean, "b" })
    .setFinal();

  Class B = b.get();

  ASSERT_EQ(B.parent(), A);

  ASSERT_EQ(B.dataMembers().size(), 1);
  ASSERT_EQ(B.dataMembers().front().type, Type::Boolean);
  ASSERT_EQ(B.dataMembers().front().name, "b");

  ASSERT_EQ(B.cumulatedDataMemberCount(), 2);
  ASSERT_EQ(B.attributesOffset(), 1);

  ASSERT_TRUE(B.isFinal());
}


TEST(Builders, virtual_members) {
  using namespace script;

  Engine engine;
  engine.setup();

  auto b = Symbol{ engine.rootNamespace() }.newClass("A");

  Class A = b.get();

  ASSERT_FALSE(A.isAbstract());
  ASSERT_EQ(A.vtable().size(), 0);

  Function foo = A.newMethod("foo").setPureVirtual().get();

  ASSERT_TRUE(foo.isVirtual());
  ASSERT_TRUE(foo.isPureVirtual());

  ASSERT_TRUE(A.isAbstract());
  ASSERT_EQ(A.vtable().size(), 1);


  b = Symbol{ engine.rootNamespace() }.newClass("B")
    .setBase(A.id());

  Class B = b.get();

  ASSERT_TRUE(B.isAbstract());
  ASSERT_EQ(B.vtable().size(), 1);
  ASSERT_EQ(B.vtable().front(), foo);

  Function foo_B = B.newMethod("foo").get();

  ASSERT_TRUE(foo_B.isVirtual());
  ASSERT_FALSE(foo_B.isPureVirtual());

  ASSERT_FALSE(B.isAbstract());
  ASSERT_EQ(B.vtable().size(), 1);
  ASSERT_EQ(B.vtable().front(), foo_B);
}


TEST(Builders, static_member_functions) {
  using namespace script;

  Engine engine;
  engine.setup();

  Class A = Symbol{ engine.rootNamespace() }.newClass("A").get();

  Function foo = A.newMethod("foo")
    .setStatic()
    .params(Type::Int).get();

  ASSERT_TRUE(foo.isMemberFunction());
  ASSERT_FALSE(foo.isNonStaticMemberFunction());
  ASSERT_FALSE(foo.hasImplicitObject());
  ASSERT_TRUE(foo.isStatic());

  ASSERT_EQ(foo.prototype().count(), 1);
}

TEST(Builders, inheritance) {
  using namespace script;

  Engine engine;
  engine.setup();

  Namespace ns = engine.rootNamespace();

  Class A = ns.newClass("A").get();
  Class B = ns.newClass("B").setBase(A).get();
  Class C = ns.newClass("C").setBase(B).get();
  Class D = ns.newClass("D").setBase(C).get();

  ASSERT_EQ(A.parent(), Class{});
  ASSERT_EQ(D.parent(), C);
  ASSERT_EQ(C.parent(), B);
  ASSERT_EQ(D.indirectBase(0), D);
  ASSERT_EQ(D.indirectBase(1), C);
  ASSERT_EQ(D.indirectBase(2), B);
}
