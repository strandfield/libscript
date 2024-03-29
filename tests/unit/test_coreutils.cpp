// Copyright (C) 2018-2022 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include <gtest/gtest.h>

#include <cstdio>
#include <fstream>

#include "script/engine.h"

#include "script/array.h"
#include "script/cast.h"
#include "script/class.h"
#include "script/classbuilder.h"
#include "script/datamember.h"
#include "script/diagnosticmessage.h"
#include "script/enum.h"
#include "script/enumbuilder.h"
#include "script/enumerator.h"
#include "script/errors.h"
#include "script/functionbuilder.h"
#include "script/functiontype.h"
#include "script/literals.h"
#include "script/name.h"
#include "script/namelookup.h"
#include "script/namespace.h"
#include "script/namespacealias.h"
#include "script/script.h"
#include "script/sourcefile.h"
#include "script/staticdatamember.h"
#include "script/symbol.h"
#include "script/typesystem.h"

#include "script/interpreter/executioncontext.h"

#include "script/private/value_p.h"

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



TEST(CoreUtilsTests, array_creation) {
  using namespace script;

  Engine e;
  e.setup();

  Array a = e.newArray(Engine::ElementType{ Type::Int });
  ASSERT_EQ(a.elementTypeId(), Type::Int);
  ASSERT_EQ(a.size(), 0);
  
  Type array_int = a.typeId();

  ASSERT_ANY_THROW(e.newArray(Engine::ElementType{ Type::Float }, Engine::FailIfNotInstantiated));

  Array b = e.newArray(Engine::ArrayType{ array_int });
  ASSERT_EQ(b.typeId(), a.typeId());
}

TEST(CoreUtilsTests, operator_names) {
  using namespace script;

  ASSERT_EQ(Operator::getFullName(PostIncrementOperator), "operator++");
  ASSERT_EQ(Operator::getFullName(PreIncrementOperator), "operator++");
  ASSERT_EQ(Operator::getFullName(LogicalNotOperator), "operator!");
  ASSERT_EQ(Operator::getFullName(BitwiseNot), "operator~");
  ASSERT_EQ(Operator::getFullName(MultiplicationOperator), "operator*");
  ASSERT_EQ(Operator::getFullName(DivisionOperator), "operator/");
  ASSERT_EQ(Operator::getFullName(AdditionOperator), "operator+");
  ASSERT_EQ(Operator::getFullName(SubstractionOperator), "operator-");
  ASSERT_EQ(Operator::getFullName(LeftShiftOperator), "operator<<");
  ASSERT_EQ(Operator::getFullName(RightShiftOperator), "operator>>");
  ASSERT_EQ(Operator::getFullName(LessOperator), "operator<");
  ASSERT_EQ(Operator::getFullName(LessEqualOperator), "operator<=");
  ASSERT_EQ(Operator::getFullName(GreaterOperator), "operator>");
  ASSERT_EQ(Operator::getFullName(GreaterEqualOperator), "operator>=");
  ASSERT_EQ(Operator::getFullName(EqualOperator), "operator==");
  ASSERT_EQ(Operator::getFullName(InequalOperator), "operator!=");
  ASSERT_EQ(Operator::getFullName(AssignmentOperator), "operator=");
  ASSERT_EQ(Operator::getFullName(MultiplicationAssignmentOperator), "operator*=");
  ASSERT_EQ(Operator::getFullName(DivisionAssignmentOperator), "operator/=");
  ASSERT_EQ(Operator::getFullName(AdditionAssignmentOperator), "operator+=");
  ASSERT_EQ(Operator::getFullName(SubstractionAssignmentOperator), "operator-=");

  ASSERT_EQ(Operator::getSymbol(AssignmentOperator), "=");
  ASSERT_EQ(Operator::getSymbol(PostIncrementOperator), "++");
  ASSERT_EQ(Operator::getSymbol(PreIncrementOperator), "++");
  ASSERT_EQ(Operator::getSymbol(LeftShiftAssignmentOperator), "<<=");
  ASSERT_EQ(Operator::getSymbol(LogicalAndOperator), "&&");
};

TEST(CoreUtilsTests, access_specifiers) {
  using namespace script;

  Engine e;
  e.setup();

  Class A = ClassBuilder(Symbol(e.rootNamespace()), "A").get();
  Function foo = FunctionBuilder::Fun(A, "foo").setProtected().get();
  Function bar = FunctionBuilder::Fun(A, "bar").setPrivate().get();
  Function qux = FunctionBuilder::Fun(A, "qux").get();

  ASSERT_EQ(foo.accessibility(), AccessSpecifier::Protected);
  ASSERT_EQ(bar.accessibility(), AccessSpecifier::Private);
  ASSERT_EQ(qux.accessibility(), AccessSpecifier::Public);

  Class B = ClassBuilder(Symbol(e.rootNamespace()), "B").setBase(A).get();
  Function slurm = FunctionBuilder::Fun(B, "slurm").get();
  Function bender = FunctionBuilder::Fun(B, "bender").get();

  ASSERT_TRUE(Accessibility::check(slurm, qux));
  ASSERT_TRUE(Accessibility::check(slurm, foo));
  ASSERT_FALSE(Accessibility::check(slurm, bar));
  ASSERT_FALSE(Accessibility::check(bender, bar));

  A.addFriend(slurm);
  ASSERT_TRUE(Accessibility::check(slurm, bar));
  ASSERT_FALSE(Accessibility::check(bender, bar));

  A.addFriend(B);
  ASSERT_TRUE(Accessibility::check(slurm, bar));
  ASSERT_TRUE(Accessibility::check(bender, bar));
};

TEST(CoreUtilsTests, access_specifiers_data_members) {
  using namespace script;

  Engine e;
  e.setup();

  ClassBuilder builder = ClassBuilder(Symbol(e.rootNamespace()), "A")
    .addMember(Class::DataMember{ Type::Double, "x" })
    .addMember(Class::DataMember{ Type::Double, "y", AccessSpecifier::Protected })
    .addMember(Class::DataMember{ Type::Double, "z", AccessSpecifier::Private });

  Class A = builder.get();

  ASSERT_EQ(A.dataMembers().front().accessibility(), AccessSpecifier::Public);
  ASSERT_EQ(A.dataMembers().at(1).accessibility(), AccessSpecifier::Protected);
  ASSERT_EQ(A.dataMembers().back().accessibility(), AccessSpecifier::Private);

  Value a = e.newInt(0);
  Value b = e.newInt(1);
  Value c = e.newInt(2);
  A.addStaticDataMember("a", a);
  A.addStaticDataMember("b", b, AccessSpecifier::Protected);
  A.addStaticDataMember("c", c, AccessSpecifier::Private);

  ASSERT_EQ(A.staticDataMembers().at("a").accessibility(), AccessSpecifier::Public);
  ASSERT_EQ(A.staticDataMembers().at("b").accessibility(), AccessSpecifier::Protected);
  ASSERT_EQ(A.staticDataMembers().at("c").accessibility(), AccessSpecifier::Private);

  ASSERT_TRUE(A.staticDataMembers().at("b").value.type().testFlag(Type::ProtectedFlag));
  ASSERT_TRUE(A.staticDataMembers().at("c").value.type().testFlag(Type::PrivateFlag));
};


TEST(CoreUtilsTests, test_names) {
  using namespace script;

  Name a{ SymbolKind::Function, "foo" };
  Name b{ SymbolKind::Function, "bar" };

  ASSERT_EQ(a.kind(), SymbolKind::Function);
  ASSERT_FALSE(a == b);

  a = AssignmentOperator; // operator=
  ASSERT_EQ(a.kind(), SymbolKind::Operator);
  ASSERT_FALSE(a == b);

  ASSERT_TRUE(a == AssignmentOperator);

  a = Name(SymbolKind::Function, "foo");
  b = Name(SymbolKind::LiteralOperator, "foo"); // operator"" foo;
  ASSERT_FALSE(a == b);

  a = Name{};
  b = Name{};
  ASSERT_TRUE(a == b);

  a = Name(SymbolKind::Cast, Type::Int); // operator int
  b = Name(SymbolKind::Cast, Type::Int);
  ASSERT_EQ(a.kind(), SymbolKind::Cast);
  ASSERT_TRUE(a == b);

  a = Name(SymbolKind::Function, "foo");
  b = Name(SymbolKind::Function, "foo");
  ASSERT_TRUE(a == b);

  a = b;
  ASSERT_TRUE(b.kind() != SymbolKind::NotASymbol);
  a = std::move(b);
  ASSERT_EQ(b.kind(), SymbolKind::NotASymbol);

  Name c{ std::move(a) };
  ASSERT_EQ(a.kind(), SymbolKind::NotASymbol);
};


TEST(CoreUtilsTests, function_names) {
  using namespace script;

  Engine e;
  e.setup();

  Class A = ClassBuilder(Symbol(e.rootNamespace()), "A").get();

  Function foo = FunctionBuilder::Fun(A, "foo").get();
  Function eq = FunctionBuilder::Op(A, EqualOperator).params(Type::Int).get();
  Function to_int = FunctionBuilder::Cast(A).setReturnType(Type::Int).get();
  Function ctor = FunctionBuilder::Constructor(A).get();
  Function a = FunctionBuilder::Fun(A, "A").get();

  Function km = FunctionBuilder::LiteralOp(e.rootNamespace(), "km").get();

  ASSERT_NE(foo.getName(), eq.getName());
  ASSERT_NE(eq.getName(), a.getName());
  ASSERT_NE(km.getName(), to_int.getName());
  ASSERT_NE(to_int.getName(), eq.getName());

  // Class name & function name can be distinguished
  ASSERT_NE(a.getName(), ctor.getName());
  
  /// TODO : make this work !
  Function dtor = e.typeSystem()->getClass(Type::String).destructor();
  ASSERT_ANY_THROW(dtor.getName());
};


TEST(CoreUtilsTests, symbols) {
  using namespace script;

  Engine e;
  e.setup();

  Class string = e.typeSystem()->getClass(Type::String);
  Namespace ns = e.rootNamespace();

  Symbol s{ string };
  ASSERT_FALSE(s.isNull());
  ASSERT_TRUE(s.isClass());
  ASSERT_FALSE(s.isNamespace());

  ASSERT_EQ(s.toClass(), string);

  s = Symbol{ ns };
  ASSERT_FALSE(s.isNull());
  ASSERT_FALSE(s.isClass());
  ASSERT_TRUE(s.isNamespace());

  ASSERT_EQ(s.toNamespace(), ns);

  s = Symbol{};
  ASSERT_TRUE(s.isNull());
  ASSERT_FALSE(s.isClass());
  ASSERT_FALSE(s.isNamespace());

  /* Testing builder functions */

  Function length = FunctionBuilder::Fun(string, "length").returns(Type::Int).setConst().get();
  ASSERT_TRUE(length.isMemberFunction());
  ASSERT_EQ(length.memberOf(), string);

  Function assign = FunctionBuilder::Op(string, AssignmentOperator).returns(Type::ref(string.id())).params(Type::Int).get();
  ASSERT_TRUE(assign.isMemberFunction());
  ASSERT_EQ(assign.prototype().count(), 2);
  ASSERT_EQ(assign.memberOf(), string);

  Function max = FunctionBuilder::Fun(ns, "max").returns(Type::Int).params(Type::Int, Type::Int).get();
  ASSERT_FALSE(max.isMemberFunction());
  ASSERT_EQ(max.enclosingNamespace(), ns);

  Function eq = FunctionBuilder::Op(ns, EqualOperator).returns(Type::Boolean).params(Type::String, Type::String).get();
  ASSERT_FALSE(eq.isMemberFunction());
  ASSERT_EQ(eq.enclosingNamespace(), ns);
  ASSERT_EQ(eq.prototype().count(), 2);
};
