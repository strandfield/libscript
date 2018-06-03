// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include <gtest/gtest.h>

#include "script/engine.h"
#include "script/class.h"
#include "script/functionbuilder.h"

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
