// Copyright (C) 2019-2021 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include <gtest/gtest.h>

#include "script/cast.h"
#include "script/castbuilder.h"
#include "script/class.h"
#include "script/classbuilder.h"
#include "script/classtemplate.h"
#include "script/constructorbuilder.h"
#include "script/conversions.h"
#include "script/engine.h"
#include "script/enum.h"
#include "script/enumbuilder.h"
#include "script/functiontype.h"
#include "script/initializerlist.h"
#include "script/name.h"
#include "script/namespace.h"
#include "script/prototype.h"
#include "script/symbol.h"
#include "script/typesystem.h"
#include "script/typesystemtransaction.h"

TEST(TypeSystemTests, Types) {
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

  Type function_type = e.typeSystem()->getFunctionType(Prototype{ Type::Void }).type();
  ASSERT_EQ(function_type.category(), Type::PrototypeFlag);

  ASSERT_TRUE(e.typeSystem()->exists(Type::Int));
  ASSERT_TRUE(e.typeSystem()->exists(Type::String));
  ASSERT_FALSE(e.typeSystem()->exists(Type::Auto));
  ASSERT_FALSE(e.typeSystem()->exists(Type::String + 66));
  ASSERT_TRUE(e.typeSystem()->exists(function_type));
}

TEST(TypeSystemTests, TypeRegistration) {
  using namespace script;

  struct MyTestStruct { };
  struct NotExposedTest { };

  Engine e;
  e.setup();

  ASSERT_EQ(e.getType<void>(), Type::Void);
  ASSERT_EQ(e.getType<bool>(), Type::Boolean);
  ASSERT_EQ(e.getType<script::String>(), Type::String);

  Type t = e.registerType<MyTestStruct>("MyTestStruct");
  ASSERT_EQ(t, e.getType<MyTestStruct>());

  ASSERT_ANY_THROW(e.getType<NotExposedTest>());
}

TEST(TypeSystemTests, MakeType) {
  using namespace script;

  struct MyTestStruct { };
  struct NotExposedTest { };

  Engine e;
  e.setup();

  ASSERT_EQ(e.makeType<void>(), Type::Void);
  ASSERT_EQ(e.makeType<bool&>(), Type::ref(Type::Boolean));
  ASSERT_EQ(e.makeType<const int>(), Type(Type::Int).withConst());
  ASSERT_EQ(e.makeType<const double&>(), Type::cref(Type::Double));
}

TEST(Conversions, fundamentals) {
  using namespace script;

  Engine e;
  e.setup();

  StandardConversion conv{ Type::Int, Type::cref(Type::Int) };
  ASSERT_TRUE(conv.isReferenceConversion());
  ASSERT_TRUE(conv.hasQualificationAdjustment());
  ASSERT_FALSE(conv.isNarrowing());
  ASSERT_FALSE(conv.isNumericConversion());
  ASSERT_FALSE(conv.isNumericPromotion());
  ASSERT_FALSE(conv.isDerivedToBaseConversion());
  ASSERT_EQ(conv.rank(), ConversionRank::ExactMatch);

  conv = StandardConversion{ Type::Int, Type::Int };
  ASSERT_EQ(conv, StandardConversion::Copy());
  ASSERT_TRUE(conv.isCopy());
  conv = StandardConversion{ Type::Int, Type{Type::Int}.withFlag(Type::ConstFlag) };
  ASSERT_EQ(conv, StandardConversion::Copy().with(ConstQualification));
  ASSERT_TRUE(conv.isCopy());

  conv = StandardConversion{ Type::Int, Type::Boolean };
  ASSERT_FALSE(conv.isReferenceConversion());
  ASSERT_TRUE(conv.isNarrowing());
  ASSERT_TRUE(conv.isNumericConversion());
  ASSERT_FALSE(conv.isNumericPromotion());
  ASSERT_FALSE(conv.isDerivedToBaseConversion());
  ASSERT_EQ(conv.numericConversion(), BooleanConversion);
  ASSERT_EQ(conv.srcType().baseType(), Type::Int);
  ASSERT_EQ(conv.destType().baseType(), Type::Boolean);
  ASSERT_EQ(conv.rank(), ConversionRank::Conversion);

  conv = StandardConversion{ Type::Int, Type::Float };
  ASSERT_FALSE(conv.isReferenceConversion());
  ASSERT_FALSE(conv.isNarrowing());
  ASSERT_FALSE(conv.isNumericConversion());
  ASSERT_TRUE(conv.isNumericPromotion());
  ASSERT_FALSE(conv.isDerivedToBaseConversion());
  ASSERT_EQ(conv.numericPromotion(), FloatingPointPromotion);
  ASSERT_EQ(conv.srcType().baseType(), Type::Int);
  ASSERT_EQ(conv.destType().baseType(), Type::Float);
  ASSERT_EQ(conv.rank(), ConversionRank::Promotion);

  conv = StandardConversion{ Type::Float, Type::Boolean };
  ASSERT_FALSE(conv.isReferenceConversion());
  ASSERT_TRUE(conv.isNarrowing());
  ASSERT_TRUE(conv.isNumericConversion());
  ASSERT_FALSE(conv.isNumericPromotion());
  ASSERT_FALSE(conv.isDerivedToBaseConversion());
  ASSERT_EQ(conv.numericConversion(), BooleanConversion);
  ASSERT_EQ(conv.srcType().baseType(), Type::Float);
  ASSERT_EQ(conv.destType().baseType(), Type::Boolean);
  ASSERT_EQ(conv.rank(), ConversionRank::Conversion);

  conv = StandardConversion{ Type::Float, Type::Double };
  ASSERT_FALSE(conv.isReferenceConversion());
  ASSERT_FALSE(conv.isNarrowing());
  ASSERT_FALSE(conv.isNumericConversion());
  ASSERT_TRUE(conv.isNumericPromotion());
  ASSERT_FALSE(conv.isDerivedToBaseConversion());
  ASSERT_EQ(conv.numericPromotion(), FloatingPointPromotion);
  ASSERT_EQ(conv.srcType().baseType(), Type::Float);
  ASSERT_EQ(conv.destType().baseType(), Type::Double);

  conv = StandardConversion{ Type::Int, Type::ref(Type::Int) };
  ASSERT_FALSE(conv == StandardConversion::NotConvertible());
  ASSERT_TRUE(conv.isReferenceConversion());
  ASSERT_FALSE(conv.isCopy());
  ASSERT_FALSE(conv.isNarrowing());
  ASSERT_FALSE(conv.isNumericConversion());
  ASSERT_FALSE(conv.isNumericPromotion());
  ASSERT_FALSE(conv.isDerivedToBaseConversion());
  ASSERT_FALSE(conv.hasQualificationAdjustment());

  conv = StandardConversion{ Type::cref(Type::Int), Type::ref(Type::Int) };
  ASSERT_TRUE(conv == StandardConversion::NotConvertible());
  ASSERT_EQ(conv.rank(), ConversionRank::NotConvertible);

  Conversion c = Conversion::compute(Type::Float, Type::Double, &e);
  ASSERT_EQ(c.rank(), ConversionRank::Promotion);
  ASSERT_EQ(c.firstStandardConversion(), StandardConversion(Type::Float, Type::Double));
  ASSERT_FALSE(c.isNarrowing());
  c = Conversion::compute(Type::Double, Type::Float, &e);
  ASSERT_TRUE(c.isNarrowing());
}

TEST(Conversions, comparisons) {
  using namespace script;

  Engine e;
  e.setup();

  ASSERT_TRUE(StandardConversion(Type::Int, Type::ref(Type::Int)) < StandardConversion(Type::Int, Type::cref(Type::Int)));
  ASSERT_TRUE(StandardConversion(Type::Int, Type::Double) < StandardConversion(Type::Float, Type::Int));
  ASSERT_FALSE(StandardConversion(Type::Float, Type::Int) < StandardConversion(Type::Int, Type::Double));

  ASSERT_FALSE(StandardConversion(Type::Float, Type::Int) < StandardConversion::compute(Type::Float, Type::Int, &e));
  ASSERT_FALSE(StandardConversion::compute(Type::Float, Type::Int, &e) < StandardConversion(Type::Float, Type::Int));

  ASSERT_TRUE(StandardConversion(Type::Int, Type::ref(Type::Int)) < StandardConversion(Type::Int, Type::Int));
  ASSERT_FALSE(StandardConversion::Copy() < StandardConversion(Type::Int, Type::ref(Type::Int)));

  std::vector<Conversion> convs{
    Conversion::compute(Type::Float, Type::Double, &e),
    Conversion::compute(Type::Double, Type::Float, &e),
    Conversion::compute(Type::Int, Type::Int, &e),
  };
  ASSERT_EQ(ranking::worstRank(convs), ConversionRank::Conversion);

  Class A = Symbol{ e.rootNamespace() }.newClass("A").get();
  ConstructorBuilder(A).params(Type::Float).create();
  convs.push_back(Conversion::compute(Type::Float, A.id(), &e));
  ASSERT_EQ(ranking::worstRank(convs), ConversionRank::UserDefinedConversion);

  ASSERT_TRUE(Conversion::comp(Conversion::compute(Type::Float, Type::Double, &e), Conversion::compute(Type::Double, Type::Float, &e)) < 0);
  ASSERT_TRUE(Conversion::comp(Conversion::compute(Type::Double, Type::Float, &e), Conversion::compute(Type::Float, Type::Double, &e)) > 0);

  ASSERT_TRUE(Conversion::comp(Conversion::compute(Type::Double, Type::Float, &e), Conversion::compute(Type::Float, Type::Int, &e)) == 0);

  ASSERT_TRUE(Conversion::comp(Conversion::compute(Type::Double, Type::Float, &e), Conversion::compute(Type::Float, A.id(), &e)) < 0);
}

TEST(Conversions, std_conv_enums) {
  using namespace script;

  Engine e;
  e.setup();

  Enum A = e.rootNamespace().newEnum("A").get();

  StandardConversion conv = StandardConversion::compute(A.id(), Type::Int, &e);
  ASSERT_EQ(conv, StandardConversion::EnumToInt());

  conv = StandardConversion::compute(A.id(), A.id(), &e);
  ASSERT_EQ(conv, StandardConversion::Copy());

  conv = StandardConversion::compute(A.id(), Type::ref(A.id()), &e);
  ASSERT_TRUE(conv.isReferenceConversion());

  conv = StandardConversion::compute(A.id(), Type::Boolean, &e);
  ASSERT_EQ(conv, StandardConversion::NotConvertible());

  conv = StandardConversion::compute(A.id(), Type::Double, &e);
  ASSERT_EQ(conv, StandardConversion::NotConvertible());
}

TEST(Conversions, std_conv_classes) {
  using namespace script;

  Engine e;
  e.setup();

  Class A = e.rootNamespace().newClass("A").get();
  ConstructorBuilder(A).params(Type::cref(A.id())).create();
  Class B = e.rootNamespace().newClass("B").setBase(A.id()).get();
  Class C = e.rootNamespace().newClass("C").setBase(B.id()).get();

  StandardConversion conv = StandardConversion::compute(A.id(), Type::Int, &e);
  ASSERT_EQ(conv, StandardConversion::NotConvertible());

  StandardConversion b_to_a = StandardConversion::compute(B.id(), A.id(), &e);
  ASSERT_TRUE(b_to_a.isDerivedToBaseConversion());
  ASSERT_EQ(b_to_a.derivedToBaseConversionDepth(), 1);

  StandardConversion c_to_a = StandardConversion::compute(C.id(), A.id(), &e);
  ASSERT_FALSE(c_to_a.isReferenceConversion());
  ASSERT_TRUE(c_to_a.isDerivedToBaseConversion());
  ASSERT_EQ(c_to_a.derivedToBaseConversionDepth(), 2);

  ASSERT_TRUE(b_to_a < c_to_a);
  ASSERT_FALSE(c_to_a < b_to_a);

  StandardConversion c_to_a_ref = StandardConversion::compute(C.id(), Type::ref(A.id()), &e);
  ASSERT_TRUE(c_to_a_ref.isReferenceConversion());
  ASSERT_TRUE(c_to_a_ref.isDerivedToBaseConversion());
  ASSERT_EQ(c_to_a_ref.derivedToBaseConversionDepth(), 2);

  StandardConversion c_to_b = StandardConversion::compute(C.id(), B.id(), &e);
  ASSERT_EQ(c_to_b, StandardConversion::NotConvertible()); // B does not have a copy ctor

  StandardConversion c_to_b_ref = StandardConversion::compute(C.id(), Type::ref(B.id()), &e);
  ASSERT_TRUE(c_to_b_ref.isReferenceConversion());
  ASSERT_TRUE(c_to_b_ref.isDerivedToBaseConversion());
  ASSERT_EQ(c_to_b_ref.derivedToBaseConversionDepth(), 1);

  StandardConversion string_to_a = StandardConversion::compute(Type::String, A.id(), &e);
  ASSERT_EQ(string_to_a, StandardConversion::NotConvertible());

  StandardConversion ref_conv = StandardConversion::compute(Type::ref(Type::String), Type::cref(Type::String), &e);
  ASSERT_TRUE(ref_conv.isReferenceConversion());
  ASSERT_TRUE(ref_conv.hasQualificationAdjustment());
}

TEST(Conversions, user_defined_conv_cast) {
  using namespace script;

  Engine e;
  e.setup();

  Class A = Symbol{ e.rootNamespace() }.newClass("A").get();
  Cast to_int = CastBuilder(A).setReturnType(Type::Int).setConst().get();

  Conversion conv = Conversion::compute(A.id(), Type::Int, &e);
  ASSERT_FALSE(conv == Conversion::NotConvertible());
  ASSERT_TRUE(conv.isUserDefinedConversion());
  ASSERT_EQ(conv.userDefinedConversion(), to_int);
  ASSERT_EQ(conv.srcType(), A.id());
  ASSERT_EQ(conv.destType(), Type::Int);
  ASSERT_EQ(conv.rank(), ConversionRank::UserDefinedConversion);
}

TEST(Conversions, user_defined_converting_constructor) {
  using namespace script;

  Engine e;
  e.setup();

  Class A = Symbol{ e.rootNamespace() }.newClass("A").get();
  Function ctor = ConstructorBuilder(A).params(Type::Float).get();

  Conversion conv = Conversion::compute(Type::Float, A.id(), &e);
  ASSERT_FALSE(conv == Conversion::NotConvertible());
  ASSERT_TRUE(conv.isUserDefinedConversion());
  ASSERT_EQ(conv.userDefinedConversion(), ctor);
  ASSERT_EQ(conv.srcType(), Type::Float);
  ASSERT_EQ(conv.destType(), A.id());
}

TEST(Conversions, converting_constructor_selection) {
  using namespace script;

  Engine e;
  e.setup();

  Class A = Symbol{ e.rootNamespace() }.newClass("A").get();
  ConstructorBuilder(A).params(Type::Int).create();
  Function ctor_bool = ConstructorBuilder(A).params(Type::Boolean).get();

  Conversion conv = Conversion::compute(Type::Boolean, A.id(), &e);
  ASSERT_FALSE(conv == Conversion::NotConvertible());
  ASSERT_TRUE(conv.isUserDefinedConversion());
  ASSERT_EQ(conv.userDefinedConversion(), ctor_bool);
  ASSERT_EQ(conv.srcType(), Type::Boolean);
  ASSERT_EQ(conv.destType(), A.id());
}

TEST(Conversions, function_type) {
  using namespace script;

  Engine e;
  e.setup();

  auto ft = e.typeSystem()->getFunctionType(DynamicPrototype{ Type::Void, {Type::Int} });

  Conversion conv = Conversion::compute(ft.type(), ft.type(), &e);
  ASSERT_FALSE(conv == Conversion::NotConvertible());
  ASSERT_FALSE(conv.isUserDefinedConversion());
  ASSERT_EQ(conv.firstStandardConversion(), StandardConversion::Copy());

  conv = Conversion::compute(ft.type(), ft.type().withFlag(Type::ReferenceFlag), &e);
  ASSERT_FALSE(conv == Conversion::NotConvertible());
  ASSERT_FALSE(conv.isUserDefinedConversion());
  ASSERT_TRUE(conv.firstStandardConversion().isReferenceConversion());

  auto ft2 = e.typeSystem()->getFunctionType(DynamicPrototype{ Type::Void, { Type::Float } });

  conv = Conversion::compute(ft.type(), ft2.type(), &e);
  ASSERT_TRUE(conv == Conversion::NotConvertible());
  ASSERT_TRUE(conv.isInvalid());
}

TEST(Conversions, no_converting_constructor) {
  using namespace script;

  Engine e;
  e.setup();

  Class A = Symbol{ e.rootNamespace() }.newClass("A").get();

  Conversion conv = Conversion::compute(Type::Float, A.id(), &e);
  ASSERT_TRUE(conv == Conversion::NotConvertible());
}

TEST(Conversions, explicit_ctor) {
  using namespace script;

  Engine e;
  e.setup();

  Class A = Symbol{ e.rootNamespace() }.newClass("A").get();
  Function ctor_int = ConstructorBuilder(A).setExplicit().params(Type::Int).get();

  Conversion conv = Conversion::compute(Type::Int, A.id(), &e);
  ASSERT_TRUE(conv == Conversion::NotConvertible());

  Function ctor_bool = ConstructorBuilder(A).params(Type::Boolean).get();
  conv = Conversion::compute(Type::Int, A.id(), &e);
  ASSERT_EQ(conv.userDefinedConversion(), ctor_bool);

  conv = Conversion::compute(Type::Int, A.id(), &e, Conversion::AllowExplicitConversions);
  ASSERT_EQ(conv.userDefinedConversion(), ctor_int);
}

TEST(Conversions, engine_functions) {
  using namespace script;

  Engine e;
  e.setup();

  ASSERT_TRUE(e.canConvert(Type::Int, Type::Float));
  ASSERT_FALSE(e.canConvert(Type::String, Type::Int));

  Namespace ns = e.rootNamespace();
  Class A = ns.newClass("A").get();
  ConstructorBuilder(A).params(Type::cref(A.id())).create();
  ASSERT_TRUE(e.canCopy(A.id()));
  ASSERT_TRUE(e.canConvert(A.id(), A.id()));

  Class B = ns.newClass("B").get();
  ASSERT_FALSE(e.canCopy(B.id()));
  ConstructorBuilder(B).params(Type::cref(B.id())).setDeleted().create();
  ASSERT_FALSE(e.canCopy(B.id()));
}


/****************************************************************
Testing Initilization class
****************************************************************/

#include "script/ast/node.h"
#include "script/compiler/compiler.h"
#include "script/compiler/compilesession.h"
#include "script/compiler/expressioncompiler.h"
#include "script/program/expression.h"
#include "script/parser/parser.h"
#include "script/initialization.h"



TEST(Initializations, cref_init) {
  using namespace script;

  Engine e;
  e.setup();

  Initialization init = Initialization::compute(Type::cref(Type::Float), Type::ref(Type::Int), &e);
  ASSERT_TRUE(init.isReferenceInitialization());
  ASSERT_TRUE(init.createsTemporary());
}

std::shared_ptr<script::parser::ParserContext> parser_context(const char* source);

static std::shared_ptr<script::program::Expression> parse_list_expr(script::Engine* e, const std::string& str)
{
  using namespace script;

  auto astlistexpr = script::parser::parseExpression(str);

  compiler::SessionManager session{ e->compiler() };

  compiler::ExpressionCompiler ec{ e->compiler() };
  ec.setScope(Scope{ e->rootNamespace() });
  return ec.generateExpression(astlistexpr);
}

TEST(Initializations, list_initialization_ctor) {
  using namespace script;

  Engine e;
  e.setup();

  auto listexpr = parse_list_expr(&e, "{1, \"Hello\", 3.14}");
  ASSERT_TRUE(listexpr->is<program::InitializerList>());

  Class A = Symbol{ e.rootNamespace() }.newClass("A").get();
  Function ctor = ConstructorBuilder(A).params(Type::Int, Type::String, Type::Double).get();

  Initialization init = Initialization::compute(A.id(), listexpr, &e);
  ASSERT_EQ(init.kind(), Initialization::ListInitialization);
  ASSERT_EQ(init.rank(), ConversionRank::ExactMatch);
  ASSERT_EQ(init.constructor(), ctor);
  ASSERT_TRUE(init.hasInitializations());
  ASSERT_EQ(init.initializations().size(), 3);
  for (size_t i(0); i < init.initializations().size(); ++i)
  {
    ASSERT_EQ(init.initializations().at(i).kind(), Initialization::CopyInitialization);
  }
}

TEST(Initializations, list_initialization_initializer_list) {
  using namespace script;

  Engine e;
  e.setup();

  auto listexpr = parse_list_expr(&e, "{1, 2, 3}");
  ASSERT_TRUE(listexpr->is<program::InitializerList>());

  Type initializer_list_int = ClassTemplate::get<InitializerListTemplate>(&e)
    .getInstance({ TemplateArgument{Type::Int} }).id();

  Initialization init = Initialization::compute(initializer_list_int, listexpr, &e);
  ASSERT_EQ(init.kind(), Initialization::ListInitialization);
  ASSERT_TRUE(init.constructor().isNull());
  ASSERT_EQ(init.destType(), initializer_list_int);
  ASSERT_EQ(init.initializations().size(), 3);
  for (size_t i(0); i < init.initializations().size(); ++i)
  {
    ASSERT_EQ(init.initializations().at(i).kind(), Initialization::CopyInitialization);
  }
}

TEST(Initializations, list_initialization_initializer_list_ctor) {
  using namespace script;

  Engine e;
  e.setup();

  auto listexpr = parse_list_expr(&e, "{1, 2, 3}");
  ASSERT_TRUE(listexpr->is<program::InitializerList>());

  Type initializer_list_int = ClassTemplate::get<InitializerListTemplate>(&e)
    .getInstance({ TemplateArgument{ Type::Int } }).id();

  Class A = Symbol{ e.rootNamespace() }.newClass("A").get();
  Function ctor = ConstructorBuilder(A).params(initializer_list_int).get();

  Initialization init = Initialization::compute(A.id(), listexpr, &e);
  ASSERT_EQ(init.kind(), Initialization::ListInitialization);
  ASSERT_EQ(init.constructor(), ctor);
  ASSERT_EQ(init.initializations().size(), 3);
  for (size_t i(0); i < init.initializations().size(); ++i)
  {
    ASSERT_EQ(init.initializations().at(i).kind(), Initialization::CopyInitialization);
  }
}

TEST(Initializations, list_initialization_empty) {
  using namespace script;

  Engine e;
  e.setup();

  auto listexpr = parse_list_expr(&e, "{ }");
  ASSERT_TRUE(listexpr->is<program::InitializerList>());

  Initialization init = Initialization::compute(Type::String, listexpr, &e);
  ASSERT_FALSE(init.hasInitializations());
  ASSERT_EQ(init.kind(), Initialization::DefaultInitialization);
}

TEST(Initializations, list_initialization_not_convertible) {
  using namespace script;

  Engine e;
  e.setup();

  auto listexpr = parse_list_expr(&e, "{1, \"Hello\", 3.14}");
  ASSERT_TRUE(listexpr->is<program::InitializerList>());

  Initialization init = Initialization::compute(Type::String, listexpr, &e);
  ASSERT_EQ(init.kind(), Initialization::InvalidInitialization);

  init = Initialization::compute(Type::Int, listexpr, &e);
  ASSERT_EQ(init.kind(), Initialization::InvalidInitialization);

  auto initlist = std::static_pointer_cast<program::InitializerList>(listexpr);
  initlist->elements.clear();

  Enum Foo = Symbol{ e.rootNamespace() }.newEnum("Foo").get();

  init = Initialization::compute(Foo.id(), listexpr, &e);
  ASSERT_EQ(init.kind(), Initialization::InvalidInitialization);

  init = Initialization::compute(Type::ref(Type::Int), listexpr, &e);
  ASSERT_EQ(init.kind(), Initialization::InvalidInitialization);
}

TEST(TypeSystemTests, symbolname) {
  using namespace script;

  Engine e;
  e.setup();

  Symbol s{ e.rootNamespace().getNamespace("bar") };
  ASSERT_EQ(s.name().kind(), SymbolKind::Namespace);
  ASSERT_EQ(s.name().string(), "bar");

  s = Symbol{ e.typeSystem()->getClass(Type::String) };
  ASSERT_EQ(s.name().kind(), SymbolKind::Class);
  ASSERT_EQ(s.name().string(), "String");

}

TEST(TypeSystemTests, transaction) {
  using namespace script;


  Engine engine;
  engine.setup();

  Namespace ns = engine.rootNamespace();

  Type A;

  {
    TypeSystemTransaction tr{ engine.typeSystem() };
    A = ns.newClass("A").get().id();
  }

  ASSERT_TRUE(engine.typeSystem()->exists(A));
  ASSERT_FALSE(engine.typeSystem()->hasActiveTransaction());

  Type B;

  try {
    TypeSystemTransaction tr{ engine.typeSystem() };
    ASSERT_TRUE(engine.typeSystem()->hasActiveTransaction());
    B = ns.newClass("B").get().id();
    throw std::runtime_error{ "Nope" };
  }
  catch (...) {

  }

  ASSERT_FALSE(engine.typeSystem()->exists(B));
}
