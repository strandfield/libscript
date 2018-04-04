// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include <gtest/gtest.h>

#include <cstdio>
#include <fstream>

#include "script/engine.h"

#include "script/class.h"
#include "script/diagnosticmessage.h"
#include "script/enum.h"
#include "script/sourcefile.h"
#include "script/types.h"


TEST(CoreUtilsTests, SourceFile) {
  using namespace script;

  SourceFile s = SourceFile::fromString("int a = 5;");
  ASSERT_TRUE(s.isLoaded());
  s.unload();
  ASSERT_ANY_THROW(s.load());

  const char * content = "int a = 5; int foo(int a, int b) { return a + b; }";
  std::ofstream file{ "temp.txt" }; 
  file << content;
  file.close();

  s = SourceFile{ "temp.txt" };
  ASSERT_NO_THROW(s.load());
  ASSERT_STREQ(content, s.data());
  s.unload();
  ASSERT_NO_THROW(s.load());
  s.unload();

  std::remove("temp.txt");

  s = SourceFile{ "temp.txt" };
  ASSERT_ANY_THROW(s.load());
}

TEST(CoreUtilsTests, Types) {
  using namespace script;

  Type t1{ Type::Int };

  ASSERT_FALSE(t1.isReference());
  ASSERT_FALSE(t1.isConst());
  ASSERT_FALSE(t1.isEnumType());
  ASSERT_FALSE(t1.isObjectType());
  ASSERT_FALSE(t1.isClosureType());
  ASSERT_TRUE(t1.isFundamentalType());

  Type reft1 = Type::ref(t1);

  ASSERT_NE(reft1, t1);
  ASSERT_EQ(reft1.baseType(), t1);
  ASSERT_EQ(t1.withFlag(Type::ReferenceFlag), reft1);
  ASSERT_TRUE(reft1.isReference());
  ASSERT_FALSE(reft1.isRefRef());
  ASSERT_FALSE(reft1.isConst());
  ASSERT_EQ(reft1.withoutFlag(Type::ReferenceFlag), t1);
  ASSERT_TRUE(reft1.isFundamentalType());

  Type const_t1 = t1.withFlag(Type::ConstFlag);

  ASSERT_TRUE(const_t1.isConst());
  ASSERT_FALSE(const_t1.isConstRef());
  ASSERT_FALSE(const_t1.isReference());
  ASSERT_TRUE(const_t1.isFundamentalType());

  Type str = Type::String;

  ASSERT_TRUE(str.isObjectType());
  ASSERT_FALSE(str.isReference());
  ASSERT_FALSE(str.isConst());
  ASSERT_FALSE(str.isEnumType());
  ASSERT_FALSE(str.isClosureType());
  ASSERT_FALSE(str.isFundamentalType());

  ASSERT_NE(Type{ Type::String }, Type{ Type::Int });
  ASSERT_NE(Type{ Type::Int }, Type{ Type::Boolean });
}


TEST(CoreUtilsTests, ClassConstruction) {
  using namespace script;

  Engine e;

  ClassBuilder builder{ "MyClass" };
  builder.setFinal();
  builder.addMember(Class::DataMember{Type::Int, "n"});

  Class my_class = e.newClass(builder);

  ASSERT_EQ(my_class.name(), "MyClass");
  ASSERT_TRUE(my_class.parent().isNull());
  ASSERT_TRUE(my_class.isFinal());

  ASSERT_EQ(my_class.dataMembers().size(), 1);
  ASSERT_EQ(my_class.dataMembers().front().name, "n");
  ASSERT_EQ(my_class.dataMembers().front().type, Type::Int);
}

TEST(CoreUtilsTests, ClassInheritance) {
  using namespace script;

  Engine e;

  ClassBuilder builder{ "Base" };
  builder.addMember(Class::DataMember{ Type::Int, "n" });

  Class base = e.newClass(builder);

  ASSERT_FALSE(base.isFinal());

  ASSERT_EQ(base.dataMembers().size(), 1);
  ASSERT_EQ(base.dataMembers().front().name, "n");
  ASSERT_EQ(base.dataMembers().front().type, Type::Int);
  ASSERT_EQ(base.attributesOffset(), 0);

  builder = ClassBuilder::New("Derived").setParent(base).addMember(Class::DataMember{ Type::Boolean, "b" });
  Class derived = e.newClass(builder);

  ASSERT_EQ(derived.parent(), base);

  ASSERT_EQ(derived.dataMembers().size(), 1);
  ASSERT_EQ(derived.dataMembers().front().name, "b");
  ASSERT_EQ(derived.dataMembers().front().type, Type::Boolean);
  ASSERT_EQ(derived.attributesOffset(), 1);
}



TEST(CoreUtilsTests, diagnostic) {
  using namespace script;

  diagnostic::Message mssg = diagnostic::info() << "Test 1";
  ASSERT_EQ(mssg.to_string(), "[info] Test 1");
  ASSERT_EQ(mssg.severity(), diagnostic::Info);
  ASSERT_EQ(mssg.line(), -1);
  ASSERT_EQ(mssg.column(), -1);

  mssg = diagnostic::warning() << "Test 2";
  ASSERT_EQ(mssg.to_string(), "[warning] Test 2");
  ASSERT_EQ(mssg.severity(), diagnostic::Warning);
  ASSERT_EQ(mssg.line(), -1);
  ASSERT_EQ(mssg.column(), -1);

  mssg = diagnostic::error() << "Test 3";
  ASSERT_EQ(mssg.to_string(), "[error] Test 3");
  ASSERT_EQ(mssg.severity(), diagnostic::Error);
  ASSERT_EQ(mssg.line(), -1);
  ASSERT_EQ(mssg.column(), -1);

  mssg = diagnostic::error() << "Error message" << diagnostic::line(10);
  ASSERT_EQ(mssg.to_string(), "[error]10: Error message");
  ASSERT_EQ(mssg.severity(), diagnostic::Error);
  ASSERT_EQ(mssg.line(), 10);
  ASSERT_EQ(mssg.column(), -1);

  mssg = diagnostic::error() << "Error message" << diagnostic::pos(10, 2);
  ASSERT_EQ(mssg.to_string(), "[error]10:2: Error message");
  ASSERT_EQ(mssg.severity(), diagnostic::Error);
  ASSERT_EQ(mssg.line(), 10);
  ASSERT_EQ(mssg.column(), 2);

  mssg = diagnostic::format("Message %1 : this %2 a %3 test", "#1", "is", "great");
  ASSERT_EQ(mssg.to_string(), "Message #1 : this is a great test");
  ASSERT_EQ(mssg.severity(), diagnostic::Info);
}


TEST(CoreUtilsTests, scopes) {
  using namespace script;

  Engine e;
  e.setup();

  Class A = e.rootNamespace().newClass(ClassBuilder::New("A"));
  Enum E = e.rootNamespace().newEnum("E");

  Namespace foo = e.rootNamespace().newNamespace("foo");
  Namespace bar = e.rootNamespace().newNamespace("bar");
  Namespace foobar = foo.newNamespace("bar");

  Scope s{ e.rootNamespace() };

  s = s.child("A");
  ASSERT_FALSE(s.isNull());
  ASSERT_EQ(s.type(), Scope::ClassScope);
  ASSERT_EQ(s.asClass(), A);
  {
    const auto & ns = s.namespaces();
    ASSERT_TRUE(ns.empty());
    const auto & lops = s.literalOperators();
    ASSERT_TRUE(lops.empty());
  }

  ASSERT_TRUE(s.hasParent());
  s = s.parent();
  ASSERT_EQ(s.type(), Scope::NamespaceScope);
  ASSERT_EQ(s.asNamespace(), e.rootNamespace());
  {
    const auto & ns = s.namespaces();
    ASSERT_EQ(ns.size(), 2);
  }

  s = s.child("foo");
  ASSERT_EQ(s.type(), Scope::NamespaceScope);
  ASSERT_EQ(s.asNamespace(), foo);
  s = s.child("bar");
  ASSERT_EQ(s.type(), Scope::NamespaceScope);
  ASSERT_EQ(s.asNamespace(), foobar);

  s = s.parent().parent().child("bar");
  ASSERT_EQ(s.type(), Scope::NamespaceScope);
  ASSERT_EQ(s.asNamespace(), bar);

  s = s.parent().child("E");
  ASSERT_EQ(s.type(), Scope::EnumClassScope);
  ASSERT_EQ(s.asEnum(), E);

  s = s.parent().parent();
  ASSERT_TRUE(s.isNull());
}



