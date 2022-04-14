// Copyright (C) 2018-2021 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include <gtest/gtest.h>

#include "script/cast.h"
#include "script/class.h"
#include "script/classbuilder.h"
#include "script/classtemplate.h"
#include "script/engine.h"
#include "script/enum.h"
#include "script/enumbuilder.h"
#include "script/enumerator.h"
#include "script/functionbuilder.h"
#include "script/literals.h"
#include "script/namespace.h"
#include "script/operator.h"
#include "script/templatebuilder.h"
#include "script/typesystem.h"

#include "script/program/expression.h"

TEST(Builders, namespaces) {
  using namespace script;

  Engine e;
  e.setup();

  Namespace foo = e.rootNamespace().getNamespace("foo");

  Namespace foo_2 = e.rootNamespace().getNamespace("foo");
  ASSERT_EQ(foo, foo_2);

  Namespace foo_3 = e.rootNamespace().newNamespace("foo");
  ASSERT_NE(foo, foo_3);
}

TEST(Builders, functions) {
  using namespace script;

  Engine e;
  e.setup();

  Namespace root = e.rootNamespace();
  Class A = ClassBuilder(Symbol(e.rootNamespace()), "A").get();

  FunctionBuilder fbuild{ Symbol(A) };

  Function foo = fbuild("foo").get();
  ASSERT_EQ(foo.name(), "foo");
  ASSERT_TRUE(foo.isMemberFunction());
  ASSERT_EQ(foo.memberOf(), A);
  ASSERT_EQ(A.memberFunctions().size(), 1);
  ASSERT_EQ(foo.returnType(), Type::Void);
  ASSERT_EQ(foo.prototype().count(), 1);
  ASSERT_TRUE(foo.prototype().at(0).testFlag(Type::ThisFlag));

  Function bar = fbuild("bar").setConst().get();
  ASSERT_EQ(bar.name(), "bar");
  ASSERT_EQ(A.memberFunctions().size(), 2);
  ASSERT_TRUE(bar.isConst());

  fbuild = FunctionBuilder(Symbol(root));

  foo = fbuild("foo").returns(Type::Int).params(Type::Int, Type::Boolean).get();
  ASSERT_EQ(foo.name(), "foo");
  ASSERT_FALSE(foo.isMemberFunction());
  ASSERT_EQ(root.functions().size(), 1);
  ASSERT_EQ(foo.returnType(), Type::Int);
  ASSERT_EQ(foo.prototype().count(), 2);
  ASSERT_EQ(foo.prototype().at(0), Type::Int);
  ASSERT_EQ(foo.prototype().at(1), Type::Boolean);

  Operator assign = FunctionBuilder::Op(A, AssignmentOperator).returns(Type::ref(A.id())).params(Type::cref(A.id())).setDeleted().get().toOperator();
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
  assign = FunctionBuilder::Op(ops, AdditionOperator).returns(A.id()).params(Type::cref(A.id()), Type::cref(A.id())).get().toOperator();
  ASSERT_EQ(assign.operatorId(), AdditionOperator);
  ASSERT_FALSE(assign.isMemberFunction());
  ASSERT_EQ(ops.operators().size(), 1);
  ASSERT_EQ(assign.returnType(), A.id());
  ASSERT_EQ(assign.prototype().count(), 2);
  ASSERT_EQ(assign.prototype().at(0), Type::cref(A.id()));
  ASSERT_EQ(assign.prototype().at(1), Type::cref(A.id()));

  Cast to_int = FunctionBuilder::Cast(A).setReturnType(Type::Int).setConst().get().toCast();
  ASSERT_EQ(to_int.destType(), Type::Int);
  ASSERT_EQ(to_int.sourceType(), Type::cref(A.id()));
  ASSERT_TRUE(to_int.isMemberFunction());
  ASSERT_EQ(to_int.memberOf(), A);
  ASSERT_EQ(A.casts().size(), 1);
}

TEST(Builders, classes) {
  using namespace script;

  Engine e;
  e.setup();

  Symbol s{ e.rootNamespace() };

  Class A = e.rootNamespace().newClass("A").get();

  ASSERT_EQ(A.name(), "A");
  ASSERT_EQ(A.enclosingNamespace(), e.rootNamespace());
}


TEST(Builders, datamember) {
  using namespace script;

  Engine e;
  e.setup();

  size_t off = e.typeSystem()->reserve(Type::ObjectFlag, 1);

  auto builder = ClassBuilder(Symbol(e.rootNamespace()), "MyClass").setId(Type::ObjectFlag | off);
  builder.setFinal();
  builder.addMember(Class::DataMember{ Type::Int, "n" });

  Class my_class = builder.get();
  ASSERT_EQ(my_class.id(), (Type::ObjectFlag | off));
  ASSERT_EQ(my_class, e.rootNamespace().classes().back());

  ASSERT_EQ(my_class.name(), "MyClass");
  ASSERT_TRUE(my_class.parent().isNull());
  ASSERT_TRUE(my_class.isFinal());

  ASSERT_EQ(my_class.dataMembers().size(), 1);
  ASSERT_EQ(my_class.dataMembers().front().name, "n");
  ASSERT_EQ(my_class.dataMembers().front().type, Type::Int);
}

TEST(Builders, operators) {
  using namespace script;

  Engine e;
  e.setup();

  Class A = e.rootNamespace().newClass("A").get();

  FunctionBuilder b = FunctionBuilder::Op(e.rootNamespace(), OperatorName::AdditionOperator);
  
  b.params(Type::cref(A.id()), Type::cref(A.id()));
  b.returns(A.id());

  Operator op = b.get().toOperator();

  ASSERT_EQ(op.operatorId(), OperatorName::AdditionOperator);
  ASSERT_EQ(op.firstOperand(), Type::cref(A.id()));
  ASSERT_EQ(op.secondOperand(), Type::cref(A.id()));
  ASSERT_EQ(op.returnType(), A.id());
}

TEST(Builders, functioncalloperator) {
  using namespace script;

  Engine e;
  e.setup();

  Symbol s{ e.rootNamespace() };

  Class A = e.rootNamespace().newClass("A").get();

  FunctionBuilder b = FunctionBuilder::Op(A, OperatorName::FunctionCallOperator);

  Operator op = b.setConst()
   .returns(Type::Int)
   .params(Type::Int, Type::Boolean)
   .get()
   .toOperator();

  ASSERT_EQ(op.prototype().count(), 3);
  ASSERT_EQ(op.returnType(), Type::Int);
  ASSERT_TRUE(op.isConst());
}

TEST(Builders, literaloperator) {
  using namespace script;

  Engine e;
  e.setup();

  Namespace ns = e.rootNamespace();

  LiteralOperator op = FunctionBuilder::LiteralOp(ns, "s").returns(Type::Int).params(Type::Boolean)
    .get().toLiteralOperator();

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

  Class A = e.rootNamespace().newClass("A").get();

  FunctionBuilder b = FunctionBuilder::Cast(A).returns(Type::Int);

  Cast cast = b.setConst().get().toCast();

  ASSERT_EQ(cast.prototype().count(), 1);
  ASSERT_EQ(cast.returnType(), Type::Int);
  ASSERT_TRUE(cast.isConst());
  ASSERT_EQ(cast.memberOf(), A);
}



TEST(Builders, constructor) {
  using namespace script;

  Engine e;
  e.setup();

  Class A = e.rootNamespace().newClass("A").get();

  FunctionBuilder b = FunctionBuilder::Constructor(A);

  b.params(Type::Int, Type::Int);

  Function ctor = b.get();

  ASSERT_EQ(ctor.prototype().count(), 3);
  ASSERT_EQ(ctor.memberOf(), A);
}

TEST(Builders, destructors) {
  using namespace script;

  Engine e;
  e.setup();

  Class A = e.rootNamespace().newClass("A").get();

  FunctionBuilder b = FunctionBuilder::Destructor(A);

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

  Class A = ClassBuilder(Symbol(engine.rootNamespace()), "A").get();
  Type A_type = A.id();

  /* Constructors */

  Function default_ctor = FunctionBuilder::Constructor(A).get();
  ASSERT_TRUE(default_ctor.isConstructor());
  ASSERT_EQ(default_ctor.memberOf(), A);
  ASSERT_EQ(default_ctor, A.defaultConstructor());

  Function copy_ctor = FunctionBuilder::Constructor(A).params(Type::cref(A_type)).get();
  ASSERT_TRUE(copy_ctor.isConstructor());
  ASSERT_EQ(copy_ctor.memberOf(), A);
  ASSERT_EQ(copy_ctor, A.copyConstructor());

  Function ctor_1 = FunctionBuilder::Constructor(A).params(Type::Int).get();
  ASSERT_TRUE(ctor_1.isConstructor());
  ASSERT_EQ(ctor_1.memberOf(), A);
  ASSERT_EQ(ctor_1.prototype().count(), 2);
  ASSERT_EQ(ctor_1.parameter(1), Type::Int);
  ASSERT_FALSE(ctor_1.isExplicit());

  Function ctor_2 = FunctionBuilder::Constructor(A).setExplicit().params(Type::Boolean).get();
  ASSERT_TRUE(ctor_2.isConstructor());
  ASSERT_EQ(ctor_2.memberOf(), A);
  ASSERT_EQ(ctor_2.prototype().count(), 2);
  ASSERT_EQ(ctor_2.parameter(1), Type::Boolean);
  ASSERT_TRUE(ctor_2.isExplicit());

  ASSERT_EQ(A.constructors().size(), 4);

  /* Conversion functions */

  Cast cast_1 = FunctionBuilder::Cast(A).setReturnType(Type::cref(Type::Int)).setConst().get().toCast();
  ASSERT_TRUE(cast_1.isMemberFunction());
  ASSERT_EQ(cast_1.memberOf(), A);
  ASSERT_TRUE(cast_1.isConst());
  ASSERT_EQ(cast_1.destType(), Type::cref(Type::Int));
  ASSERT_EQ(cast_1.destType(), cast_1.returnType());
  ASSERT_FALSE(cast_1.isExplicit());

  Cast cast_2 = FunctionBuilder::Cast(A).setReturnType(Type::ref(Type::Int)).setExplicit().get().toCast();
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

  auto b = ClassBuilder(Symbol(engine.rootNamespace()), "A")
    .addMember(Class::DataMember{ Type::Int, "a" });

  Class A = b.get();

  ASSERT_EQ(A.dataMembers().size(), 1);
  ASSERT_EQ(A.dataMembers().front().type, Type::Int);
  ASSERT_EQ(A.dataMembers().front().name, "a");

  ASSERT_EQ(A.cumulatedDataMemberCount(), 1);
  ASSERT_EQ(A.attributesOffset(), 0);

  b = ClassBuilder(Symbol(engine.rootNamespace()), "B")
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

  auto b = ClassBuilder(Symbol(engine.rootNamespace()), "A");

  Class A = b.get();

  ASSERT_FALSE(A.isAbstract());
  ASSERT_EQ(A.vtable().size(), 0);

  Function foo = FunctionBuilder::Fun(A, "foo").setPureVirtual().get();

  ASSERT_TRUE(foo.isVirtual());
  ASSERT_TRUE(foo.isPureVirtual());

  ASSERT_TRUE(A.isAbstract());
  ASSERT_EQ(A.vtable().size(), 1);


  b = ClassBuilder(Symbol(engine.rootNamespace()), "B")
    .setBase(A.id());

  Class B = b.get();

  ASSERT_TRUE(B.isAbstract());
  ASSERT_EQ(B.vtable().size(), 1);
  ASSERT_EQ(B.vtable().front(), foo);

  Function foo_B = FunctionBuilder::Fun(B, "foo").get();

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

  Class A = ClassBuilder(Symbol(engine.rootNamespace()), "A").get();

  Function foo = FunctionBuilder::Fun(A, "foo")
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

TEST(Builders, inheritance2) {
  using namespace script;

  Engine e;
  e.setup();

  auto builder = ClassBuilder(Symbol(e.rootNamespace()), "Base");
  builder.addMember(Class::DataMember{ Type::Int, "n" });

  Class base = builder.get();

  ASSERT_FALSE(base.isFinal());

  ASSERT_EQ(base.dataMembers().size(), 1);
  ASSERT_EQ(base.dataMembers().front().name, "n");
  ASSERT_EQ(base.dataMembers().front().type, Type::Int);
  ASSERT_EQ(base.attributesOffset(), 0);

  builder = ClassBuilder(Symbol(e.rootNamespace()), "Derived").setBase(base).addMember(Class::DataMember{ Type::Boolean, "b" });
  Class derived = builder.get();

  ASSERT_EQ(derived.parent(), base);

  ASSERT_EQ(derived.dataMembers().size(), 1);
  ASSERT_EQ(derived.dataMembers().front().name, "b");
  ASSERT_EQ(derived.dataMembers().front().type, Type::Boolean);
  ASSERT_EQ(derived.attributesOffset(), 1);
}


TEST(Builders, enums) {
  using namespace script;

  Engine e;
  e.setup();

  size_t nb_enums = e.rootNamespace().enums().size();

  Enum A = EnumBuilder(Symbol(e.rootNamespace()), "A")
    .setEnumClass(true).get();
  A.addValue("A1", 1);
  A.addValue("A2", 2);
  A.addValue("A3", 3);

  ASSERT_EQ(A.name(), "A");
  ASSERT_TRUE(A.isEnumClass());
  ASSERT_EQ(e.rootNamespace().enums().size(), nb_enums + 1);

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


class DummyClassTemplateBackend : public script::ClassTemplateNativeBackend
{
  script::Class instantiate(script::ClassTemplateInstanceBuilder&)
  {
    throw std::runtime_error{ "dummy" };
  }
};

class DummyFunctionTemplateBackend : public script::FunctionTemplateNativeBackend
{
  void deduce(script::TemplateArgumentDeduction& deduction, const std::vector<script::TemplateArgument>& targs, const std::vector<script::Type>& itypes) override
  {
    throw std::runtime_error{ "dummy" };
  }

  void substitute(script::FunctionBlueprint& blueprint, const std::vector<script::TemplateArgument>& targs) override
  {
    throw std::runtime_error{ "dummy" };
  }

  std::pair<script::NativeFunctionSignature, std::shared_ptr<script::UserData>> instantiate(script::Function& function) override
  {
    throw std::runtime_error{ "dummy" };
  }
};

TEST(Builders, function_template_create) {
  using namespace script;

  Engine e;
  e.setup();

  Symbol s{ e.rootNamespace() };

  const auto nb_templates = e.rootNamespace().templates().size();

  // We cannot use get() here because FunctionTemplate has not been defined yet
  FunctionTemplateBuilder(s, "foo").params(TemplateParameter{ TemplateParameter::TypeParameter{}, "T" })
    .withBackend<DummyFunctionTemplateBackend>()
    .setScope(e.rootNamespace()).create();

  ASSERT_EQ(e.rootNamespace().templates().size(), nb_templates + 1);
}

#include "script/functiontemplate.h"

TEST(Builders, function_template_get) {
  using namespace script;

  Engine e;
  e.setup();

  Symbol s{ e.rootNamespace() };

  FunctionTemplate foo = FunctionTemplateBuilder(s, "foo").params(
    TemplateParameter{ TemplateParameter::TypeParameter{}, "T" },
    TemplateParameter{ TemplateParameter::TypeParameter{}, "U" })
    .withBackend<DummyFunctionTemplateBackend>()
    .setScope(e.rootNamespace()).get();

  ASSERT_EQ(foo.name(), "foo");
  ASSERT_EQ(foo.enclosingSymbol().toNamespace(), e.rootNamespace());
  ASSERT_EQ(foo.parameters().size(), 2);
  ASSERT_EQ(foo.parameters().at(0).name(), "T");
  ASSERT_EQ(foo.parameters().at(1).name(), "U");
}

TEST(Builders, class_template_get) {
  using namespace script;

  Engine e;
  e.setup();

  Symbol s{ e.rootNamespace() };

  ClassTemplate Bar = ClassTemplateBuilder(s, "Bar").params(
    TemplateParameter{ TemplateParameter::TypeParameter{}, "T" },
    TemplateParameter{ TemplateParameter::TypeParameter{}, "U" })
    .withBackend<DummyClassTemplateBackend>()
    .setScope(e.rootNamespace()).get();

  ASSERT_EQ(Bar.name(), "Bar");
  ASSERT_EQ(Bar.enclosingSymbol().toNamespace(), e.rootNamespace());
  ASSERT_EQ(Bar.parameters().size(), 2);
  ASSERT_EQ(Bar.parameters().at(0).name(), "T");
  ASSERT_EQ(Bar.parameters().at(1).name(), "U");
}
