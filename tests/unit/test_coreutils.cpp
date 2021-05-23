// Copyright (C) 2018-2021 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include <gtest/gtest.h>

#include <cstdio>
#include <fstream>

#include "script/engine.h"

#include "script/array.h"
#include "script/cast.h"
#include "script/castbuilder.h"
#include "script/class.h"
#include "script/classbuilder.h"
#include "script/constructorbuilder.h"
#include "script/datamember.h"
#include "script/diagnosticmessage.h"
#include "script/enum.h"
#include "script/enumbuilder.h"
#include "script/enumerator.h"
#include "script/errors.h"
#include "script/functionbuilder.h"
#include "script/functiontype.h"
#include "script/literals.h"
#include "script/literaloperatorbuilder.h"
#include "script/name.h"
#include "script/namelookup.h"
#include "script/namespace.h"
#include "script/namespacealias.h"
#include "script/operatorbuilder.h"
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

  Class A = Symbol{ e.rootNamespace() }.newClass("A").get();
  Function foo = FunctionBuilder(A, "foo").setProtected().get();
  Function bar = FunctionBuilder(A, "bar").setPrivate().get();
  Function qux = FunctionBuilder(A, "qux").get();

  ASSERT_EQ(foo.accessibility(), AccessSpecifier::Protected);
  ASSERT_EQ(bar.accessibility(), AccessSpecifier::Private);
  ASSERT_EQ(qux.accessibility(), AccessSpecifier::Public);

  Class B = Symbol{ e.rootNamespace() }.newClass("B").setBase(A).get();
  Function slurm = FunctionBuilder(B, "slurm").get();
  Function bender = FunctionBuilder(B, "bender").get();

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

  ClassBuilder builder = Symbol{e.rootNamespace()}.newClass("A")
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

  Name a{ "foo" };
  Name b{ "bar" };

  ASSERT_EQ(a.kind(), Name::StringName);
  ASSERT_FALSE(a == b);

  a = AssignmentOperator; // operator=
  ASSERT_EQ(a.kind(), Name::OperatorName);
  ASSERT_FALSE(a == b);

  ASSERT_TRUE(a == AssignmentOperator);

  a = "foo";
  b = Name{ Name::LiteralOperatorTag{}, "foo" }; // operator"" foo;
  ASSERT_FALSE(a == b);

  a = Name{};
  b = Name{};
  ASSERT_TRUE(a == b);

  a = Name{ Name::CastTag{}, Type::Int }; // operator int
  b = Name{ Name::CastTag{}, Type::Int };
  ASSERT_EQ(a.kind(), Name::CastName);
  ASSERT_TRUE(a == b);

  a = "foo";
  b = "foo";
  ASSERT_TRUE(a == b);

  a = b;
  ASSERT_TRUE(b.kind() != Name::InvalidName);
  a = std::move(b);
  ASSERT_EQ(b.kind(), Name::InvalidName);

  Name c{std::move(a)}; // operator""km
  ASSERT_EQ(a.kind(), Name::InvalidName);
};


TEST(CoreUtilsTests, function_names) {
  using namespace script;

  Engine e;
  e.setup();

  Class A = Symbol{ e.rootNamespace() }.newClass("A").get();

  Function foo = FunctionBuilder(A, "foo").get();
  Function eq = OperatorBuilder(Symbol(A), EqualOperator).params(Type::Int).get();
  Function to_int = CastBuilder(A).setReturnType(Type::Int).get();
  Function ctor = ConstructorBuilder(A).get();
  Function a = FunctionBuilder(A, "A").get();

  Function km = LiteralOperatorBuilder(e.rootNamespace(), "km").get();

  ASSERT_NE(foo.getName(), eq.getName());
  ASSERT_NE(eq.getName(), a.getName());
  ASSERT_NE(km.getName(), to_int.getName());
  ASSERT_NE(to_int.getName(), eq.getName());

  // Still some limitations ?
  ASSERT_EQ(a.getName(), ctor.getName());
  
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

  s = Symbol{ string };
  Function length = s.newFunction("length").returns(Type::Int).setConst().get();
  ASSERT_TRUE(length.isMemberFunction());
  ASSERT_EQ(length.memberOf(), string);

  Function assign = s.newOperator(AssignmentOperator).returns(Type::ref(string.id())).params(Type::Int).get();
  ASSERT_TRUE(assign.isMemberFunction());
  ASSERT_EQ(assign.prototype().count(), 2);
  ASSERT_EQ(assign.memberOf(), string);

  s = Symbol{ ns };
  Function max = s.newFunction("max").returns(Type::Int).params(Type::Int, Type::Int).get();
  ASSERT_FALSE(max.isMemberFunction());
  ASSERT_EQ(max.enclosingNamespace(), ns);

  Function eq = s.newOperator(EqualOperator).returns(Type::Boolean).params(Type::String, Type::String).get();
  ASSERT_FALSE(eq.isMemberFunction());
  ASSERT_EQ(eq.enclosingNamespace(), ns);
  ASSERT_EQ(eq.prototype().count(), 2);
};


static script::Value incr_callback(script::FunctionCall *c)
{
  int n = c->arg(1).toInt();
  script::get<int>(c->arg(0)) += n;
  return c->arg(0);
}

#include "script/program/expression.h"

TEST(CoreUtilsTests, default_arguments) {
  using namespace script;

  Engine e;
  e.setup();

  Value default_arg = e.newInt(1);

  Function incr = Symbol{ e.rootNamespace() }.newFunction("incr")
    .returns(Type::ref(Type::Int))
    .params(Type::ref(Type::Int), Type::cref(Type::Int))
    .addDefaultArgument(program::VariableAccess::New(default_arg))
    .setCallback(incr_callback).get();

  const char *src =
    "  int a = 0;     \n"
    "  incr(a, 2);    \n"
    "  incr(a);       \n";

  Script s = e.newScript(SourceFile::fromString(src));
  const bool success = s.compile();
  ASSERT_TRUE(success);

  s.run();

  ASSERT_EQ(s.globals().size(), 1);
  
  Value a = s.globals().front();
  ASSERT_EQ(a.type(), Type::Int);
  ASSERT_EQ(a.toInt(), 3);
};
