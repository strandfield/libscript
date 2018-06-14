// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include <gtest/gtest.h>

#include "script/engine.h"
#include "script/class.h"
#include "script/functionbuilder.h"


TEST(ClassTest, builder_functions) {
  using namespace script;

  Engine engine;
  engine.setup();

  Class A = engine.newClass(ClassBuilder::New("A"));
  Type A_type = A.id();

  /* Constructors */

  Function default_ctor = A.Constructor().create();
  ASSERT_TRUE(default_ctor.isConstructor());
  ASSERT_EQ(default_ctor.memberOf(), A);
  ASSERT_EQ(default_ctor, A.defaultConstructor());

  Function copy_ctor = A.Constructor().params(Type::cref(A_type)).create();
  ASSERT_TRUE(copy_ctor.isConstructor());
  ASSERT_EQ(copy_ctor.memberOf(), A);
  ASSERT_EQ(copy_ctor, A.copyConstructor());

  Function ctor_1 = A.Constructor().params(Type::Int).create();
  ASSERT_TRUE(ctor_1.isConstructor());
  ASSERT_EQ(ctor_1.memberOf(), A);
  ASSERT_EQ(ctor_1.prototype().argc(), 1);
  ASSERT_EQ(ctor_1.parameter(0), Type::Int);
  ASSERT_FALSE(ctor_1.isExplicit());

  Function ctor_2 = A.Constructor().setExplicit().params(Type::Boolean).create();
  ASSERT_TRUE(ctor_2.isConstructor());
  ASSERT_EQ(ctor_2.memberOf(), A);
  ASSERT_EQ(ctor_2.prototype().argc(), 1);
  ASSERT_EQ(ctor_2.parameter(0), Type::Boolean);
  ASSERT_TRUE(ctor_2.isExplicit());

  ASSERT_EQ(A.constructors().size(), 4);

  /* Conversion functions */

  Cast cast_1 = A.Conversion(Type::cref(Type::Int)).setConst().create().toCast();
  ASSERT_TRUE(cast_1.isMemberFunction());
  ASSERT_EQ(cast_1.memberOf(), A);
  ASSERT_TRUE(cast_1.isConst());
  ASSERT_EQ(cast_1.destType(), Type::cref(Type::Int));
  ASSERT_EQ(cast_1.destType(), cast_1.returnType());
  ASSERT_FALSE(cast_1.isExplicit());

  Cast cast_2 = A.Conversion(Type::ref(Type::Int)).setExplicit().create().toCast();
  ASSERT_TRUE(cast_2.isMemberFunction());
  ASSERT_EQ(cast_2.memberOf(), A);
  ASSERT_FALSE(cast_2.isConst());
  ASSERT_EQ(cast_2.destType(), Type::ref(Type::Int));
  ASSERT_TRUE(cast_2.isExplicit());
}


TEST(ClassTest, datamembers) {
  using namespace script;

  Engine engine;
  engine.setup();

  auto b = ClassBuilder::New("A")
    .addMember(Class::DataMember{ Type::Int, "a" });

  Class A = engine.newClass(b);

  ASSERT_EQ(A.dataMembers().size(), 1);
  ASSERT_EQ(A.dataMembers().front().type, Type::Int);
  ASSERT_EQ(A.dataMembers().front().name, "a");

  ASSERT_EQ(A.cumulatedDataMemberCount(), 1);
  ASSERT_EQ(A.attributesOffset(), 0);

  b = ClassBuilder::New("B")
    .setParent(A)
    .addMember(Class::DataMember{ Type::Boolean, "b" })
    .setFinal();

  Class B = engine.newClass(b);

  ASSERT_EQ(B.parent(), A);

  ASSERT_EQ(B.dataMembers().size(), 1);
  ASSERT_EQ(B.dataMembers().front().type, Type::Boolean);
  ASSERT_EQ(B.dataMembers().front().name, "b");

  ASSERT_EQ(B.cumulatedDataMemberCount(), 2);
  ASSERT_EQ(B.attributesOffset(), 1);

  ASSERT_TRUE(B.isFinal());
}


TEST(ClassTest, virtual_members) {
  using namespace script;

  Engine engine;
  engine.setup();

  auto b = ClassBuilder::New("A");

  Class A = engine.newClass(b);

  ASSERT_FALSE(A.isAbstract());
  ASSERT_EQ(A.vtable().size(), 0);

  auto fb = FunctionBuilder::Method(A, "foo")
    .setPureVirtual();
  Function foo = A.newMethod(fb);

  ASSERT_TRUE(foo.isVirtual());
  ASSERT_TRUE(foo.isPureVirtual());

  ASSERT_TRUE(A.isAbstract());
  ASSERT_EQ(A.vtable().size(), 1);


  b = ClassBuilder::New("B")
    .setParent(A);

  Class B = engine.newClass(b);

  ASSERT_TRUE(B.isAbstract());
  ASSERT_EQ(B.vtable().size(), 1);
  ASSERT_EQ(B.vtable().front(), foo);

  fb = FunctionBuilder::Method(B, "foo");
  Function foo_B = B.newMethod(fb);

  ASSERT_TRUE(foo_B.isVirtual());
  ASSERT_FALSE(foo_B.isPureVirtual());

  ASSERT_FALSE(B.isAbstract());
  ASSERT_EQ(B.vtable().size(), 1);
  ASSERT_EQ(B.vtable().front(), foo_B);
}


TEST(ClassTest, static_member_functions) {
  using namespace script;

  Engine engine;
  engine.setup();

  Class A = engine.newClass(ClassBuilder::New("A"));

  Function foo = A.Method("foo")
    .setStatic()
    .params(Type::Int).create();

  ASSERT_TRUE(foo.isMemberFunction());
  ASSERT_FALSE(foo.isNonStaticMemberFunction());
  ASSERT_TRUE(foo.isStatic());

  ASSERT_EQ(foo.prototype().argc(), 1);
}
