// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include <gtest/gtest.h>

#include <cstdio>
#include <fstream>

#include "script/engine.h"

#include "script/array.h"
#include "script/class.h"
#include "script/diagnosticmessage.h"
#include "script/enum.h"
#include "script/functionbuilder.h"
#include "script/functiontype.h"
#include "script/namelookup.h"
#include "script/namespacealias.h"
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

  ASSERT_EQ(Type::cref(Type::Int).withoutRef(), Type(Type::Int, Type::ConstFlag));
  ASSERT_EQ(Type(Type::Int).withConst(), Type(Type::Int, Type::ConstFlag));
  ASSERT_EQ(Type(Type::Int).withConst().withoutConst(), Type::Int);

  Engine e;
  e.setup();
  ASSERT_TRUE(e.hasType(Type::Int));
  ASSERT_TRUE(e.hasType(Type::String));
  ASSERT_FALSE(e.hasType(Type::Auto));
  ASSERT_FALSE(e.hasType(Type::String + 66));
  ASSERT_TRUE(e.hasType(e.newFunctionType(Prototype{ Type::Void }).type()));
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
  Class foo_C = foo.newClass(ClassBuilder::New("C"));

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
  Class foo_A = foo.newClass(ClassBuilder::New("A"));
  Class foo_B = foo.newClass(ClassBuilder::New("B"));
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
  Class A = root.newClass(ClassBuilder::New("A"));

  Function foo = A.Method("foo").create();
  ASSERT_EQ(foo.name(), "foo");
  ASSERT_TRUE(foo.isMemberFunction());
  ASSERT_EQ(foo.memberOf(), A);
  ASSERT_EQ(A.memberFunctions().size(), 1);
  ASSERT_EQ(foo.returnType(), Type::Void);
  ASSERT_EQ(foo.prototype().argc(), 1);
  ASSERT_TRUE(foo.prototype().argv(0).testFlag(Type::ThisFlag));

  Function bar = A.Method("bar").setConst().create();
  ASSERT_EQ(bar.name(), "bar");
  ASSERT_EQ(A.memberFunctions().size(), 2);
  ASSERT_TRUE(bar.isConst());

  foo = root.Function("foo").returns(Type::Int).params(Type::Int, Type::Boolean).create();
  ASSERT_EQ(foo.name(), "foo");
  ASSERT_FALSE(foo.isMemberFunction());
  ASSERT_EQ(root.functions().size(), 1);
  ASSERT_EQ(foo.returnType(), Type::Int);
  ASSERT_EQ(foo.prototype().argc(), 2);
  ASSERT_EQ(foo.prototype().argv(0), Type::Int);
  ASSERT_EQ(foo.prototype().argv(1), Type::Boolean);

  Operator assign = A.Operation(Operator::AssignmentOperator).returns(Type::ref(A.id())).params(Type::cref(A.id())).setDeleted().create().toOperator();
  ASSERT_EQ(assign.operatorId(), Operator::AssignmentOperator);
  ASSERT_TRUE(assign.isMemberFunction());
  ASSERT_EQ(assign.memberOf(), A);
  ASSERT_EQ(A.operators().size(), 1);
  ASSERT_EQ(assign.returnType(), Type::ref(A.id()));
  ASSERT_EQ(assign.prototype().argc(), 2);
  ASSERT_EQ(assign.prototype().argv(0), Type::ref(A.id()));
  ASSERT_EQ(assign.prototype().argv(1), Type::cref(A.id()));
  ASSERT_TRUE(assign.isDeleted());

  Namespace ops = root.newNamespace("ops");
  assign = ops.Operation(Operator::AdditionOperator).returns(A.id()).params(Type::cref(A.id()), Type::cref(A.id())).create().toOperator();
  ASSERT_EQ(assign.operatorId(), Operator::AdditionOperator);
  ASSERT_FALSE(assign.isMemberFunction());
  ASSERT_EQ(ops.operators().size(), 1);
  ASSERT_EQ(assign.returnType(), A.id());
  ASSERT_EQ(assign.prototype().argc(), 2);
  ASSERT_EQ(assign.prototype().argv(0), Type::cref(A.id()));
  ASSERT_EQ(assign.prototype().argv(1), Type::cref(A.id()));

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

  ASSERT_EQ(Operator::getFullName(Operator::PostIncrementOperator), "operator++");
  ASSERT_EQ(Operator::getFullName(Operator::PreIncrementOperator), "operator++");
  ASSERT_EQ(Operator::getFullName(Operator::LogicalNotOperator), "operator!");
  ASSERT_EQ(Operator::getFullName(Operator::BitwiseNot), "operator~");
  ASSERT_EQ(Operator::getFullName(Operator::MultiplicationOperator), "operator*");
  ASSERT_EQ(Operator::getFullName(Operator::DivisionOperator), "operator/");
  ASSERT_EQ(Operator::getFullName(Operator::AdditionOperator), "operator+");
  ASSERT_EQ(Operator::getFullName(Operator::SubstractionOperator), "operator-");
  ASSERT_EQ(Operator::getFullName(Operator::LeftShiftOperator), "operator<<");
  ASSERT_EQ(Operator::getFullName(Operator::RightShiftOperator), "operator>>");
  ASSERT_EQ(Operator::getFullName(Operator::LessOperator), "operator<");
  ASSERT_EQ(Operator::getFullName(Operator::LessEqualOperator), "operator<=");
  ASSERT_EQ(Operator::getFullName(Operator::GreaterOperator), "operator>");
  ASSERT_EQ(Operator::getFullName(Operator::GreaterEqualOperator), "operator>=");
  ASSERT_EQ(Operator::getFullName(Operator::EqualOperator), "operator==");
  ASSERT_EQ(Operator::getFullName(Operator::InequalOperator), "operator!=");
  ASSERT_EQ(Operator::getFullName(Operator::AssignmentOperator), "operator=");
  ASSERT_EQ(Operator::getFullName(Operator::MultiplicationAssignmentOperator), "operator*=");
  ASSERT_EQ(Operator::getFullName(Operator::DivisionAssignmentOperator), "operator/=");
  ASSERT_EQ(Operator::getFullName(Operator::AdditionAssignmentOperator), "operator+=");
  ASSERT_EQ(Operator::getFullName(Operator::SubstractionAssignmentOperator), "operator-=");
};

TEST(CoreUtilsTests, access_specifiers) {
  using namespace script;

  Engine e;
  e.setup();

  Class A = e.rootNamespace().newClass(ClassBuilder::New("A"));
  Function foo = A.Method("foo").setProtected().create();
  Function bar = A.Method("bar").setPrivate().create();
  Function qux = A.Method("qux").create();

  ASSERT_EQ(foo.accessibility(), AccessSpecifier::Protected);
  ASSERT_EQ(bar.accessibility(), AccessSpecifier::Private);
  ASSERT_EQ(qux.accessibility(), AccessSpecifier::Public);

  Class B = e.rootNamespace().newClass(ClassBuilder::New("B").setParent(A));
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

  ClassBuilder builder = ClassBuilder::New("A")
    .addMember(Class::DataMember{ Type::Double, "x" })
    .addMember(Class::DataMember{ Type::Double, "y", AccessSpecifier::Protected })
    .addMember(Class::DataMember{ Type::Double, "z", AccessSpecifier::Private });

  Class A = e.rootNamespace().newClass(builder);

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



