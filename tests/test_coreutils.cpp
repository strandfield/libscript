// Copyright (C) 2018 Vincent Chambrin
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
#include "script/functionbuilder.h"
#include "script/functiontype.h"
#include "script/name.h"
#include "script/namelookup.h"
#include "script/namespace.h"
#include "script/namespacealias.h"
#include "script/script.h"
#include "script/sourcefile.h"
#include "script/staticdatamember.h"
#include "script/symbol.h"
#include "script/types.h"

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
  ASSERT_EQ(str.category(), Type::ObjectFlag);

  ASSERT_NE(Type{ Type::String }, Type{ Type::Int });
  ASSERT_NE(Type{ Type::Int }, Type{ Type::Boolean });

  ASSERT_EQ(Type::cref(Type::Int).withoutRef(), Type(Type::Int, Type::ConstFlag));
  ASSERT_EQ(Type(Type::Int).withConst(), Type(Type::Int, Type::ConstFlag));
  ASSERT_EQ(Type(Type::Int).withConst().withoutConst(), Type::Int);

  const Type invalid_type = Type::ObjectFlag | Type::EnumFlag | 1;
  ASSERT_FALSE(invalid_type.isValid());
  ASSERT_TRUE(Type{ Type::Int }.isValid());
  ASSERT_TRUE(str.isValid());

  Engine e;
  e.setup();

  Type function_type = e.newFunctionType(Prototype{ Type::Void }).type();
  ASSERT_EQ(function_type.category(), Type::PrototypeFlag);

  ASSERT_TRUE(e.hasType(Type::Int));
  ASSERT_TRUE(e.hasType(Type::String));
  ASSERT_FALSE(e.hasType(Type::Auto));
  ASSERT_FALSE(e.hasType(Type::String + 66));
  ASSERT_TRUE(e.hasType(function_type));
}

TEST(CoreUtilsTests, TypeReservation) {
  using namespace script;

  Engine e;
  e.setup();

  const int begin = Type::ObjectFlag | 10;
  const int end = Type::ObjectFlag | 11;

  e.reserveTypeRange(begin, end);

  Class A = Symbol{ e.rootNamespace() }.Class("A").setId(Type::ObjectFlag | 10).get();
  ASSERT_EQ(A.id(), Type::ObjectFlag | 10);
}

TEST(CoreUtilsTests, Enums) {
  using namespace script;

  Engine e;
  e.setup();

  Enum A = Symbol{ e.rootNamespace() }.Enum("A").get();
  A.addValue("A1", 1);
  A.addValue("A2", 2);
  A.addValue("A3", 3);

  ASSERT_TRUE(A.hasKey("A1"));
  ASSERT_FALSE(A.hasKey("HK47"));
  ASSERT_TRUE(A.hasValue(2));
  ASSERT_EQ(A.getKey(2), "A2");
  ASSERT_EQ(Enumerator(A, 2).name(), "A2");
  ASSERT_FALSE(A.hasValue(66));
  ASSERT_EQ(A.getValue("A1"), 1);
  ASSERT_EQ(A.getValue("HK47", -1), -1);

  ASSERT_EQ(A.enclosingNamespace(), e.rootNamespace());
}

TEST(CoreUtilsTests, ClassConstruction) {
  using namespace script;

  Engine e;
  e.setup();

  auto builder = Symbol{ e.rootNamespace() }.Class("MyClass");
  builder.setFinal();
  builder.addMember(Class::DataMember{Type::Int, "n"});

  Class my_class = builder.get();
  ASSERT_EQ(my_class, e.rootNamespace().classes().back());

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
  e.setup();

  auto builder = Symbol{ e.rootNamespace() }.Class("Base");
  builder.addMember(Class::DataMember{ Type::Int, "n" });

  Class base = builder.get();

  ASSERT_FALSE(base.isFinal());

  ASSERT_EQ(base.dataMembers().size(), 1);
  ASSERT_EQ(base.dataMembers().front().name, "n");
  ASSERT_EQ(base.dataMembers().front().type, Type::Int);
  ASSERT_EQ(base.attributesOffset(), 0);

  builder = Symbol{ e.rootNamespace() }.Class("Derived").setBase(base).addMember(Class::DataMember{ Type::Boolean, "b" });
  Class derived = builder.get();

  ASSERT_EQ(derived.parent(), base);

  ASSERT_EQ(derived.dataMembers().size(), 1);
  ASSERT_EQ(derived.dataMembers().front().name, "b");
  ASSERT_EQ(derived.dataMembers().front().type, Type::Boolean);
  ASSERT_EQ(derived.attributesOffset(), 1);
}



TEST(CoreUtilsTests, diagnostic) {
  using namespace script;

  diagnostic::Message mssg = diagnostic::info() << "Test 1" << diagnostic::Code{"A39"};
  ASSERT_EQ(mssg.to_string(), "[info](A39) Test 1");
  ASSERT_EQ(mssg.severity(), diagnostic::Info);
  ASSERT_EQ(mssg.code(), "A39");
  ASSERT_EQ(mssg.line(), -1);
  ASSERT_EQ(mssg.column(), -1);

  mssg = diagnostic::warning() << "Test 2";
  ASSERT_EQ(mssg.to_string(), "[warning] Test 2");
  ASSERT_EQ(mssg.severity(), diagnostic::Warning);
  ASSERT_EQ(mssg.code(), std::string());
  ASSERT_EQ(mssg.line(), -1);
  ASSERT_EQ(mssg.column(), -1);

  mssg = diagnostic::error() << "Test 3";
  ASSERT_EQ(mssg.to_string(), "[error] Test 3");
  ASSERT_EQ(mssg.severity(), diagnostic::Error);
  ASSERT_EQ(mssg.code(), std::string());
  ASSERT_EQ(mssg.line(), -1);
  ASSERT_EQ(mssg.column(), -1);

  mssg = diagnostic::error() << "Error message" << diagnostic::line(10);
  ASSERT_EQ(mssg.to_string(), "[error]10: Error message");
  ASSERT_EQ(mssg.severity(), diagnostic::Error);
  ASSERT_EQ(mssg.code(), std::string());
  ASSERT_EQ(mssg.line(), 10);
  ASSERT_EQ(mssg.column(), -1);

  mssg = diagnostic::error() << "Error message" << diagnostic::Code{ "A39" } << diagnostic::pos(10, 2);
  ASSERT_EQ(mssg.to_string(), "[error](A39)10:2: Error message");
  ASSERT_EQ(mssg.severity(), diagnostic::Error);
  ASSERT_EQ(mssg.code(), "A39");
  ASSERT_EQ(mssg.line(), 10);
  ASSERT_EQ(mssg.column(), 2);

  mssg = diagnostic::format("Message %1 : this %2 a %3 test", "#1", "is", "great");
  ASSERT_EQ(mssg.to_string(), "Message #1 : this is a great test");
  ASSERT_EQ(mssg.severity(), diagnostic::Info);
}


TEST(CoreUtilsTests, namespaces) {
  using namespace script;

  Engine e;
  e.setup();

  Namespace foo = e.rootNamespace().getNamespace("foo");

  Namespace foo_2 = e.rootNamespace().getNamespace("foo");
  ASSERT_EQ(foo, foo_2);

  Namespace foo_3 = e.rootNamespace().newNamespace("foo");
  ASSERT_NE(foo, foo_3);
}

TEST(CoreUtilsTests, scopes) {
  using namespace script;

  Engine e;
  e.setup();

  Class A = Symbol{ e.rootNamespace() }.Class("A").get();
  Enum E = e.rootNamespace().Enum("E").get();

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

TEST(CoreUtilsTests, scope_type_alias_injection) {
  // This test simulates the effect of defining a type alias ('using alias = type')
  using namespace script;

  Engine e;
  e.setup();

  Scope s{ e.rootNamespace() };
  s.inject(std::string("Distance"), Type{ Type::Double });

  NameLookup lookup = s.lookup("Distance");
  ASSERT_EQ(lookup.resultType(), NameLookup::TypeName);
  ASSERT_EQ(lookup.typeResult(), Type::Double);
}

TEST(CoreUtilsTests, scope_class_injection) {
  // This test simulates the effect of a 'using foo::C' inside a namespace 'bar'
  using namespace script;

  Engine e;
  e.setup();

  Namespace foo = e.rootNamespace().newNamespace("foo");
  Class foo_C = Symbol{ foo }.Class("C").get();

  Namespace bar = e.rootNamespace().newNamespace("bar");

  Scope s{ e.rootNamespace() };

  s = s.child("bar");
  ASSERT_FALSE(s.isNull());
  ASSERT_EQ(s.type(), Scope::NamespaceScope);
  ASSERT_EQ(s.asNamespace(), bar);
  
  NameLookup lookup = s.lookup("C");
  ASSERT_EQ(lookup.resultType(), NameLookup::UnknownName);

  s.inject(foo_C);
  lookup = s.lookup("C");
  ASSERT_EQ(lookup.resultType(), NameLookup::TypeName);
  ASSERT_EQ(lookup.typeResult(), foo_C.id());

  s = s.parent().child("bar");
  lookup = s.lookup("C");
  ASSERT_EQ(lookup.resultType(), NameLookup::UnknownName);
  lookup = NameLookup::resolve("foo::C", s);
  ASSERT_EQ(lookup.resultType(), NameLookup::TypeName);
  ASSERT_EQ(lookup.typeResult(), foo_C.id());
  s.inject(lookup.impl().get());
  lookup = s.lookup("C");
  ASSERT_EQ(lookup.resultType(), NameLookup::TypeName);
  ASSERT_EQ(lookup.typeResult(), foo_C.id());
}

TEST(CoreUtilsTests, scope_namespace_injection) {
  // This test simulates the effect of a 'using namespace foo' inside a namespace 'bar'
  using namespace script;

  Engine e;
  e.setup();

  Namespace foo = e.rootNamespace().newNamespace("foo");
  Class foo_A = Symbol{ foo }.Class("A").get();
  Class foo_B = Symbol{ foo }.Class("B").get();
  Function foo_max_int = foo.Function("max").returns(Type::Int).params(Type::Int, Type::Int).create();
  Function foo_max_double = foo.Function("max").returns(Type::Double).params(Type::Double, Type::Double).create();

  Namespace bar = e.rootNamespace().newNamespace("bar");
  Function bar_max_float = bar.Function("max").returns(Type::Float).params(Type::Float, Type::Float).create();

  Scope s{ e.rootNamespace() };

  s = s.child("bar");

  NameLookup lookup = s.lookup("A");
  ASSERT_EQ(lookup.resultType(), NameLookup::UnknownName);

  lookup = s.lookup("max");
  ASSERT_EQ(lookup.resultType(), NameLookup::FunctionName);
  ASSERT_EQ(lookup.functions().size(), 1);
  ASSERT_EQ(lookup.functions().front(), bar_max_float);

  s.inject(Scope{ foo });

  lookup = s.lookup("A");
  ASSERT_EQ(lookup.resultType(), NameLookup::TypeName);
  ASSERT_EQ(lookup.typeResult(), foo_A.id());

  lookup = s.lookup("B");
  ASSERT_EQ(lookup.resultType(), NameLookup::TypeName);
  ASSERT_EQ(lookup.typeResult(), foo_B.id());

  lookup = s.lookup("max");
  ASSERT_EQ(lookup.resultType(), NameLookup::FunctionName);
  ASSERT_EQ(lookup.functions().size(), 3);

  s = s.parent().child("bar");

  lookup = s.lookup("max");
  ASSERT_EQ(lookup.resultType(), NameLookup::FunctionName);
  ASSERT_EQ(lookup.functions().size(), 1);
}

TEST(CoreUtilsTests, scope_merge) {
  // This test simulates the effect of 'importing' a namespace hierarchy into another.
  using namespace script;

  Engine e;
  e.setup();

  Namespace anon_1 = e.rootNamespace().newNamespace("anon1");
  Namespace anon_1_bar = anon_1.newNamespace("bar");
  Function anon_1_bar_func = anon_1_bar.Function("func").create();

  Namespace anon_2 = e.rootNamespace().newNamespace("anon2");
  Namespace anon_2_bar = anon_2.newNamespace("bar");
  Function anon_2_bar_func = anon_2_bar.Function("func").create();

  Scope base{ anon_1 };

  Scope s = base.child("bar");

  NameLookup lookup = s.lookup("func");
  ASSERT_EQ(lookup.resultType(), NameLookup::FunctionName);
  ASSERT_EQ(lookup.functions().size(), 1);
  ASSERT_EQ(lookup.functions().front(), anon_1_bar_func);

  s.merge(anon_2);

  lookup = s.lookup("func");
  ASSERT_EQ(lookup.resultType(), NameLookup::FunctionName);
  ASSERT_EQ(lookup.functions().size(), 2);
  ASSERT_EQ(lookup.functions().front(), anon_1_bar_func);
  ASSERT_EQ(lookup.functions().back(), anon_2_bar_func);

  s = s.parent().child("bar");
  lookup = s.lookup("func");
  ASSERT_EQ(lookup.resultType(), NameLookup::FunctionName);
  ASSERT_EQ(lookup.functions().size(), 2);
  ASSERT_EQ(lookup.functions().front(), anon_1_bar_func);
  ASSERT_EQ(lookup.functions().back(), anon_2_bar_func);

  s = base.child("bar");
  lookup = s.lookup("func");
  ASSERT_EQ(lookup.resultType(), NameLookup::FunctionName);
  ASSERT_EQ(lookup.functions().size(), 1);
  ASSERT_EQ(lookup.functions().front(), anon_1_bar_func);
}

TEST(CoreUtilsTests, scope_namespace_alias) {
  // This test simulates the effect of 'namespace fbq = foo::bar::qux'.
  using namespace script;

  Engine e;
  e.setup();

  Namespace foo = e.rootNamespace().newNamespace("foo");
  Namespace bar = foo.newNamespace("bar");
  Namespace qux = bar.newNamespace("qux");

  Function func = qux.Function("func").create();

  Scope base{ e.rootNamespace() };
  Scope s = base.child("foo");

  s.inject(NamespaceAlias{ "fbz", {"foo", "bar", "qux"} });

  NameLookup lookup = NameLookup::resolve("fbz::func", s);
  ASSERT_EQ(lookup.resultType(), NameLookup::FunctionName);
  ASSERT_EQ(lookup.functions().size(), 1);
  ASSERT_EQ(lookup.functions().front(), func);

  ASSERT_ANY_THROW(s.inject(NamespaceAlias{ "b", { "bla" } }));
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

TEST(CoreUtilsTests, function_builder) {
  using namespace script;

  Engine e;
  e.setup();

  Namespace root = e.rootNamespace();
  Class A = Symbol{ root }.Class("A").get();

  Function foo = A.Method("foo").create();
  ASSERT_EQ(foo.name(), "foo");
  ASSERT_TRUE(foo.isMemberFunction());
  ASSERT_EQ(foo.memberOf(), A);
  ASSERT_EQ(A.memberFunctions().size(), 1);
  ASSERT_EQ(foo.returnType(), Type::Void);
  ASSERT_EQ(foo.prototype().count(), 1);
  ASSERT_TRUE(foo.prototype().at(0).testFlag(Type::ThisFlag));

  Function bar = A.Method("bar").setConst().create();
  ASSERT_EQ(bar.name(), "bar");
  ASSERT_EQ(A.memberFunctions().size(), 2);
  ASSERT_TRUE(bar.isConst());

  foo = root.Function("foo").returns(Type::Int).params(Type::Int, Type::Boolean).create();
  ASSERT_EQ(foo.name(), "foo");
  ASSERT_FALSE(foo.isMemberFunction());
  ASSERT_EQ(root.functions().size(), 1);
  ASSERT_EQ(foo.returnType(), Type::Int);
  ASSERT_EQ(foo.prototype().count(), 2);
  ASSERT_EQ(foo.prototype().at(0), Type::Int);
  ASSERT_EQ(foo.prototype().at(1), Type::Boolean);

  Operator assign = A.Operation(AssignmentOperator).returns(Type::ref(A.id())).params(Type::cref(A.id())).setDeleted().create().toOperator();
  ASSERT_EQ(assign.operatorId(), AssignmentOperator);
  ASSERT_TRUE(assign.isMemberFunction());
  ASSERT_EQ(assign.memberOf(), A);
  ASSERT_EQ(A.operators().size(), 1);
  ASSERT_EQ(assign.returnType(), Type::ref(A.id()));
  ASSERT_EQ(assign.prototype().count(), 2);
  ASSERT_EQ(assign.prototype().at(0), Type::ref(A.id()));
  ASSERT_EQ(assign.prototype().at(1), Type::cref(A.id()));
  ASSERT_TRUE(assign.isDeleted());

  Namespace ops = root.newNamespace("ops");
  assign = ops.Operation(AdditionOperator).returns(A.id()).params(Type::cref(A.id()), Type::cref(A.id())).create().toOperator();
  ASSERT_EQ(assign.operatorId(), AdditionOperator);
  ASSERT_FALSE(assign.isMemberFunction());
  ASSERT_EQ(ops.operators().size(), 1);
  ASSERT_EQ(assign.returnType(), A.id());
  ASSERT_EQ(assign.prototype().count(), 2);
  ASSERT_EQ(assign.prototype().at(0), Type::cref(A.id()));
  ASSERT_EQ(assign.prototype().at(1), Type::cref(A.id()));

  Cast to_int = A.Conversion(Type::Int).setConst().create().toCast();
  ASSERT_EQ(to_int.destType(), Type::Int);
  ASSERT_EQ(to_int.sourceType(), Type::cref(A.id()));
  ASSERT_TRUE(to_int.isMemberFunction());
  ASSERT_EQ(to_int.memberOf(), A);
  ASSERT_EQ(A.casts().size(), 1);
}

TEST(CoreUtilsTests, uninitialized) {
  using namespace script;

  Engine e;
  e.setup();

  Value v = e.uninitialized(Type::Int);
  ASSERT_EQ(v.type(), Type::Int);
  ASSERT_FALSE(v.isInitialized());

  e.initialize(v);
  ASSERT_TRUE(v.isInitialized());
  e.destroy(v);
  v = e.uninitialized(Type::Int);

  Value init = e.newInt(3);
  e.uninitialized_copy(init, v);
  e.destroy(init);

  ASSERT_TRUE(v.isInitialized());
  ASSERT_EQ(v.type(), Type::Int);
  ASSERT_EQ(v.toInt(), 3);
  e.destroy(v);
};

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
};

TEST(CoreUtilsTests, access_specifiers) {
  using namespace script;

  Engine e;
  e.setup();

  Class A = Symbol{ e.rootNamespace() }.Class("A").get();
  Function foo = A.Method("foo").setProtected().create();
  Function bar = A.Method("bar").setPrivate().create();
  Function qux = A.Method("qux").create();

  ASSERT_EQ(foo.accessibility(), AccessSpecifier::Protected);
  ASSERT_EQ(bar.accessibility(), AccessSpecifier::Private);
  ASSERT_EQ(qux.accessibility(), AccessSpecifier::Public);

  Class B = Symbol{ e.rootNamespace() }.Class("B").setBase(A).get();
  Function slurm = B.Method("slurm").create();
  Function bender = B.Method("bender").create();

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

  ClassBuilder builder = Symbol{e.rootNamespace()}.Class("A")
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
  ASSERT_TRUE(a == b);

  a = "foo";
  b = "foo";
  ASSERT_TRUE(a == b);

  a = std::move(b);
  ASSERT_EQ(b.kind(), Name::InvalidName);
};


TEST(CoreUtilsTests, function_names) {
  using namespace script;

  Engine e;
  e.setup();

  Class A = Symbol{ e.rootNamespace() }.Class("A").get();

  Function foo = A.Method("foo").create();
  Function eq = A.Operation(EqualOperator).params(Type::Int).create();
  Function to_int = A.Conversion(Type::Int).create();
  Function ctor = A.Constructor().create();
  Function a = A.Method("A").create();

  Function km = e.rootNamespace().UserDefinedLiteral("km").create();

  ASSERT_NE(foo.getName(), eq.getName());
  ASSERT_NE(eq.getName(), a.getName());
  ASSERT_NE(km.getName(), to_int.getName());
  ASSERT_NE(to_int.getName(), eq.getName());

  // Still some limitations ?
  ASSERT_EQ(a.getName(), ctor.getName());
  
  /// TODO : make this work !
  Function dtor = e.getClass(Type::String).destructor();
  ASSERT_ANY_THROW(dtor.getName());
};


TEST(CoreUtilsTests, symbols) {
  using namespace script;

  Engine e;
  e.setup();

  Class string = e.getClass(Type::String);
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
  Function length = s.Function("length").returns(Type::Int).setConst().create();
  ASSERT_TRUE(length.isMemberFunction());
  ASSERT_EQ(length.memberOf(), string);

  Function assign = s.Operation(AssignmentOperator).returns(Type::ref(string.id())).params(Type::Int).create();
  ASSERT_TRUE(assign.isMemberFunction());
  ASSERT_EQ(assign.prototype().count(), 2);
  ASSERT_EQ(assign.memberOf(), string);

  s = Symbol{ ns };
  Function max = s.Function("max").returns(Type::Int).params(Type::Int, Type::Int).create();
  ASSERT_FALSE(max.isMemberFunction());
  ASSERT_EQ(max.enclosingNamespace(), ns);

  Function eq = s.Operation(EqualOperator).returns(Type::Boolean).params(Type::String, Type::String).create();
  ASSERT_FALSE(eq.isMemberFunction());
  ASSERT_EQ(eq.enclosingNamespace(), ns);
  ASSERT_EQ(eq.prototype().count(), 2);
};


static script::Value incr_callback(script::FunctionCall *c)
{
  int n = c->arg(1).toInt();
  c->arg(0).impl()->set_int(c->arg(0).toInt() + n);
  return c->arg(0);
}

#include "script/program/expression.h"

TEST(CoreUtilsTests, default_arguments) {
  using namespace script;

  Engine e;
  e.setup();

  Value default_arg = e.newInt(1);
  e.manage(default_arg);

  Function incr = Symbol{ e.rootNamespace() }.Function("incr")
    .returns(Type::ref(Type::Int))
    .params(Type::ref(Type::Int), Type::cref(Type::Int))
    .addDefaultArgument(program::VariableAccess::New(default_arg))
    .setCallback(incr_callback).create();

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
