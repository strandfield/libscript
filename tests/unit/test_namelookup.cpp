// Copyright (C) 2018-2021 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include <gtest/gtest.h>

#include "script/arraytemplate.h"
#include "script/class.h"
#include "script/classbuilder.h"
#include "script/classtemplate.h"
#include "script/engine.h"
#include "script/enum.h"
#include "script/enumbuilder.h"
#include "script/function.h"
#include "script/functionbuilder.h"
#include "script/namelookup.h"
#include "script/namespace.h"
#include "script/namespacealias.h"
#include "script/scope.h"
#include "script/value.h"

TEST(NameLookup, simple_function) {
  using namespace script;

  Engine e;
  e.setup();

  Symbol{ e.rootNamespace() }.newFunction("foo").create();

  NameLookup lookup = NameLookup::resolve("foo", e.rootNamespace());
  ASSERT_EQ(lookup.resultType(), NameLookup::FunctionName);
  ASSERT_EQ(lookup.functions().size(), 1);

  lookup = NameLookup::resolve("bar", e.rootNamespace());
  ASSERT_EQ(lookup.resultType(), NameLookup::UnknownName);

  Symbol{ e.rootNamespace() }.newFunction("foo").params(Type::Int).create();

  lookup = NameLookup::resolve("foo", e.rootNamespace());
  ASSERT_EQ(lookup.resultType(), NameLookup::FunctionName);
  ASSERT_EQ(lookup.functions().size(), 2);
}

TEST(NameLookup, variable) {
  using namespace script;

  Engine e;
  e.setup();

  Value n = e.newInt(3);
  e.rootNamespace().addValue("n", n);

  NameLookup lookup = NameLookup::resolve("n", e.rootNamespace());
  ASSERT_EQ(lookup.resultType(), NameLookup::VariableName);
  ASSERT_EQ(lookup.variable().toInt(), 3);
}

TEST(NameLookup, builtin_types) {
  using namespace script;

  Engine e;
  e.setup();

  NameLookup lookup = NameLookup::resolve("void", e.rootNamespace());
  ASSERT_EQ(lookup.resultType(), NameLookup::TypeName);
  ASSERT_EQ(lookup.typeResult(), Type{ Type::Void });

  lookup = NameLookup::resolve("bool", e.rootNamespace());
  ASSERT_EQ(lookup.resultType(), NameLookup::TypeName);
  ASSERT_EQ(lookup.typeResult(), Type{ Type::Boolean });

  lookup = NameLookup::resolve("char", e.rootNamespace());
  ASSERT_EQ(lookup.resultType(), NameLookup::TypeName);
  ASSERT_EQ(lookup.typeResult(), Type{ Type::Char });

  lookup = NameLookup::resolve("int", e.rootNamespace());
  ASSERT_EQ(lookup.resultType(), NameLookup::TypeName);
  ASSERT_EQ(lookup.typeResult(), Type{ Type::Int });

  lookup = NameLookup::resolve("float", e.rootNamespace());
  ASSERT_EQ(lookup.resultType(), NameLookup::TypeName);
  ASSERT_EQ(lookup.typeResult(), Type{ Type::Float });

  lookup = NameLookup::resolve("double", e.rootNamespace());
  ASSERT_EQ(lookup.resultType(), NameLookup::TypeName);
  ASSERT_EQ(lookup.typeResult(), Type{ Type::Double });

  lookup = NameLookup::resolve("auto", e.rootNamespace());
  ASSERT_EQ(lookup.resultType(), NameLookup::TypeName);
  ASSERT_EQ(lookup.typeResult(), Type{ Type::Auto });
}


TEST(NameLookup, nested) {
  using namespace script;

  Engine e;
  e.setup();

  Namespace nested = e.rootNamespace().newNamespace("nested");
  Scope nested_scope{ nested, e.rootNamespace() };

  Value n = e.newInt(3);
  e.rootNamespace().addValue("n", n);

  NameLookup lookup = NameLookup::resolve("n", nested_scope);
  ASSERT_EQ(lookup.resultType(), NameLookup::VariableName);
  ASSERT_EQ(lookup.variable().toInt(), 3);

  n = e.newInt(4);
  nested.addValue("n", n);

  lookup = NameLookup::resolve("n", nested_scope);
  ASSERT_EQ(lookup.resultType(), NameLookup::VariableName);
  ASSERT_EQ(lookup.variable().toInt(), 4);
}

TEST(NameLookup, scope_lookup) {
  using namespace script;

  Engine e;
  e.setup();

  Namespace nested = e.rootNamespace().newNamespace("nested");

  NameLookup lookup = NameLookup::resolve("nested", Scope{ e.rootNamespace() });
  ASSERT_EQ(lookup.resultType(), NameLookup::NamespaceName);
  Scope scp = lookup.scopeResult();
  ASSERT_EQ(scp.type(), Scope::NamespaceScope);
  ASSERT_EQ(scp.asNamespace(), nested);
}

TEST(NameLookup, array_template) {
  using namespace script;

  Engine e;
  e.setup();

  NameLookup lookup = NameLookup::resolve("Array", e.rootNamespace());
  ASSERT_EQ(lookup.resultType(), NameLookup::TemplateName);
  ASSERT_EQ(lookup.classTemplateResult(), ClassTemplate::get<ArrayTemplate>(&e));
}

TEST(NameLookup, operators) {
  using namespace script;

  Engine e;
  e.setup();

  NameLookup lookup = NameLookup::resolve(AssignmentOperator, e.rootNamespace());
  ASSERT_EQ(lookup.resultType(), NameLookup::FunctionName);
  ASSERT_EQ(lookup.functions().size(), 5);
}

TEST(NameLookup, parsing_operator_name) {
  using namespace script;

  Engine e;
  e.setup();

  // this is less interesting than passing directly the operator name 
  // because it can be ambiguous (that is the case here)
  NameLookup lookup = NameLookup::resolve("operator++", e.rootNamespace());
  ASSERT_EQ(lookup.resultType(), NameLookup::FunctionName);
  ASSERT_EQ(lookup.functions().size(), 4);
}

TEST(NameLookup, parsing_nested_name) {
  using namespace script;

  Engine e;
  e.setup();

  Namespace nested = e.rootNamespace().newNamespace("nested");
  Value n = e.newInt(3);
  nested.addValue("n", n);

  NameLookup lookup = NameLookup::resolve("nested::n", e.rootNamespace());
  ASSERT_EQ(lookup.resultType(), NameLookup::VariableName);
  ASSERT_EQ(lookup.variable().toInt(), 3);
}

TEST(NameLookup, member_lookup) {
  using namespace script;

  Engine e;
  e.setup();

  Symbol gns{ e.rootNamespace() };

  Class foo = gns.newClass("foo").get();
  FunctionBuilder(foo, "f").create();

  Class bar = gns.newClass("bar").setBase(foo).get();
  FunctionBuilder(bar, "g").create();

  NameLookup lookup = NameLookup::member("g", bar);
  ASSERT_EQ(lookup.resultType(), NameLookup::FunctionName);
  ASSERT_EQ(lookup.functions().size(), 1);

  lookup = NameLookup::member("f", bar);
  ASSERT_EQ(lookup.resultType(), NameLookup::FunctionName);
  ASSERT_EQ(lookup.functions().size(), 1);

  lookup = NameLookup::member("k", bar);
  ASSERT_EQ(lookup.resultType(), NameLookup::UnknownName);
  ASSERT_EQ(lookup.scope().asClass(), bar);
}

TEST(NameLookup, scopes) {
  using namespace script;

  Engine e;
  e.setup();

  Class A = Symbol{ e.rootNamespace() }.newClass("A").get();
  Enum E = e.rootNamespace().newEnum("E").get();

  Namespace foo = e.rootNamespace().newNamespace("foo");
  Namespace bar = e.rootNamespace().newNamespace("bar");
  Namespace foobar = foo.newNamespace("bar");

  Scope s{ e.rootNamespace() };

  s = s.child("A");
  ASSERT_FALSE(s.isNull());
  ASSERT_EQ(s.type(), Scope::ClassScope);
  ASSERT_EQ(s.asClass(), A);
  {
    const auto& ns = s.namespaces();
    ASSERT_TRUE(ns.empty());
    const auto& lops = s.literalOperators();
    ASSERT_TRUE(lops.empty());
  }

  ASSERT_TRUE(s.hasParent());
  s = s.parent();
  ASSERT_EQ(s.type(), Scope::NamespaceScope);
  ASSERT_EQ(s.asNamespace(), e.rootNamespace());
  {
    const auto& ns = s.namespaces();
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

TEST(NameLookup, scope_type_alias_injection) {
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

TEST(NameLookup, scope_class_injection) {
  // This test simulates the effect of a 'using foo::C' inside a namespace 'bar'
  using namespace script;

  Engine e;
  e.setup();

  Namespace foo = e.rootNamespace().newNamespace("foo");
  Class foo_C = Symbol{ foo }.newClass("C").get();

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

TEST(NameLookup, scope_namespace_injection) {
  // This test simulates the effect of a 'using namespace foo' inside a namespace 'bar'
  using namespace script;

  Engine e;
  e.setup();

  Namespace foo = e.rootNamespace().newNamespace("foo");
  Class foo_A = Symbol{ foo }.newClass("A").get();
  Class foo_B = Symbol{ foo }.newClass("B").get();
  Function foo_max_int = FunctionBuilder(foo, "max").returns(Type::Int).params(Type::Int, Type::Int).get();
  Function foo_max_double = FunctionBuilder(foo, "max").returns(Type::Double).params(Type::Double, Type::Double).get();

  Namespace bar = e.rootNamespace().newNamespace("bar");
  Function bar_max_float = FunctionBuilder(bar, "max").returns(Type::Float).params(Type::Float, Type::Float).get();

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

TEST(NameLookup, scope_merge) {
  // This test simulates the effect of 'importing' a namespace hierarchy into another.
  using namespace script;

  Engine e;
  e.setup();

  Namespace anon_1 = e.rootNamespace().newNamespace("anon1");
  Namespace anon_1_bar = anon_1.newNamespace("bar");
  Function anon_1_bar_func = FunctionBuilder(anon_1_bar, "func").get();

  Namespace anon_2 = e.rootNamespace().newNamespace("anon2");
  Namespace anon_2_bar = anon_2.newNamespace("bar");
  Function anon_2_bar_func = FunctionBuilder(anon_2_bar, "func").get();

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

TEST(NameLookup, scope_namespace_alias) {
  // This test simulates the effect of 'namespace fbq = foo::bar::qux'.
  using namespace script;

  Engine e;
  e.setup();

  Namespace foo = e.rootNamespace().newNamespace("foo");
  Namespace bar = foo.newNamespace("bar");
  Namespace qux = bar.newNamespace("qux");

  Function func = FunctionBuilder(qux, "func").get();

  Scope base{ e.rootNamespace() };
  Scope s = base.child("foo");

  s.inject(NamespaceAlias{ "fbz", {"foo", "bar", "qux"} });

  NameLookup lookup = NameLookup::resolve("fbz::func", s);
  ASSERT_EQ(lookup.resultType(), NameLookup::FunctionName);
  ASSERT_EQ(lookup.functions().size(), 1);
  ASSERT_EQ(lookup.functions().front(), func);

  ASSERT_ANY_THROW(s.inject(NamespaceAlias{ "b", { "bla" } }));
}
