// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include <gtest/gtest.h>

#include "script/cast.h"
#include "script/class.h"
#include "script/classbuilder.h"
#include "script/classtemplate.h"
#include "script/conversions.h"
#include "script/engine.h"
#include "script/enum.h"
#include "script/enumbuilder.h"
#include "script/functionbuilder.h"
#include "script/functiontype.h"
#include "script/namespace.h"

TEST(Conversions, standard) {
  using namespace script;

  Engine e;

  StandardConversion conv = StandardConversion::compute(Type::Int, Type::cref(Type::Int), &e);
  ASSERT_TRUE(conv.isReferenceInitialization());
  ASSERT_FALSE(conv.isCopyInitialization());
  ASSERT_FALSE(conv.isNarrowing());
  ASSERT_FALSE(conv.isNumericConversion());
  ASSERT_FALSE(conv.isNumericPromotion());
  ASSERT_FALSE(conv.isDerivedToBaseConversion());

  conv = StandardConversion::compute(Type::Int, Type::Boolean, &e);
  ASSERT_FALSE(conv.isReferenceInitialization());
  ASSERT_TRUE(conv.isCopyInitialization());
  ASSERT_TRUE(conv.isNarrowing());
  ASSERT_TRUE(conv.isNumericConversion());
  ASSERT_FALSE(conv.isNumericPromotion());
  ASSERT_FALSE(conv.isDerivedToBaseConversion());
  ASSERT_EQ(conv.numericConversion(), BooleanConversion);

  conv = StandardConversion::compute(Type::Int, Type::Float, &e);
  ASSERT_FALSE(conv.isReferenceInitialization());
  ASSERT_TRUE(conv.isCopyInitialization());
  ASSERT_FALSE(conv.isNarrowing());
  ASSERT_FALSE(conv.isNumericConversion());
  ASSERT_TRUE(conv.isNumericPromotion());
  ASSERT_FALSE(conv.isDerivedToBaseConversion());
  ASSERT_EQ(conv.numericPromotion(), FloatingPointPromotion);


  conv = StandardConversion::compute(Type::Int, Type::ref(Type::Int), &e);
  ASSERT_FALSE(conv == StandardConversion::NotConvertible());
  ASSERT_TRUE(conv.isReferenceInitialization());
  ASSERT_FALSE(conv.isCopyInitialization());
  ASSERT_FALSE(conv.isNarrowing());
  ASSERT_FALSE(conv.isNumericConversion());
  ASSERT_FALSE(conv.isNumericPromotion());
  ASSERT_FALSE(conv.isDerivedToBaseConversion());
}

TEST(Conversions, fundamentals) {
  using namespace script;

  Engine e;

  StandardConversion2 conv{ Type::Int, Type::cref(Type::Int) };
  ASSERT_TRUE(conv.isReferenceConversion());
  ASSERT_TRUE(conv.hasQualificationAdjustment());
  ASSERT_FALSE(conv.isNarrowing());
  ASSERT_FALSE(conv.isNumericConversion());
  ASSERT_FALSE(conv.isNumericPromotion());
  ASSERT_FALSE(conv.isDerivedToBaseConversion());
  ASSERT_EQ(conv.rank(), ConversionRank::ExactMatch);

  conv = StandardConversion2{ Type::Int, Type::Int };
  ASSERT_EQ(conv, StandardConversion2::Copy());
  conv = StandardConversion2{ Type::Int, Type{Type::Int}.withFlag(Type::ConstFlag) };
  ASSERT_EQ(conv, StandardConversion2::Copy().with(ConstQualification));

  conv = StandardConversion2{ Type::Int, Type::Boolean };
  ASSERT_FALSE(conv.isReferenceConversion());
  ASSERT_TRUE(conv.isNarrowing());
  ASSERT_TRUE(conv.isNumericConversion());
  ASSERT_FALSE(conv.isNumericPromotion());
  ASSERT_FALSE(conv.isDerivedToBaseConversion());
  ASSERT_EQ(conv.numericConversion(), BooleanConversion);
  ASSERT_EQ(conv.srcType().baseType(), Type::Int);
  ASSERT_EQ(conv.destType().baseType(), Type::Boolean);
  ASSERT_EQ(conv.rank(), ConversionRank::Conversion);

  conv = StandardConversion2{ Type::Int, Type::Float };
  ASSERT_FALSE(conv.isReferenceConversion());
  ASSERT_FALSE(conv.isNarrowing());
  ASSERT_FALSE(conv.isNumericConversion());
  ASSERT_TRUE(conv.isNumericPromotion());
  ASSERT_FALSE(conv.isDerivedToBaseConversion());
  ASSERT_EQ(conv.numericPromotion(), FloatingPointPromotion);
  ASSERT_EQ(conv.srcType().baseType(), Type::Int);
  ASSERT_EQ(conv.destType().baseType(), Type::Float);
  ASSERT_EQ(conv.rank(), ConversionRank::Promotion);

  conv = StandardConversion2{ Type::Float, Type::Boolean };
  ASSERT_FALSE(conv.isReferenceConversion());
  ASSERT_TRUE(conv.isNarrowing());
  ASSERT_TRUE(conv.isNumericConversion());
  ASSERT_FALSE(conv.isNumericPromotion());
  ASSERT_FALSE(conv.isDerivedToBaseConversion());
  ASSERT_EQ(conv.numericConversion(), BooleanConversion);
  ASSERT_EQ(conv.srcType().baseType(), Type::Float);
  ASSERT_EQ(conv.destType().baseType(), Type::Boolean);
  ASSERT_EQ(conv.rank(), ConversionRank::Conversion);

  conv = StandardConversion2{ Type::Float, Type::Double };
  ASSERT_FALSE(conv.isReferenceConversion());
  ASSERT_FALSE(conv.isNarrowing());
  ASSERT_FALSE(conv.isNumericConversion());
  ASSERT_TRUE(conv.isNumericPromotion());
  ASSERT_FALSE(conv.isDerivedToBaseConversion());
  ASSERT_EQ(conv.numericPromotion(), FloatingPointPromotion);
  ASSERT_EQ(conv.srcType().baseType(), Type::Float);
  ASSERT_EQ(conv.destType().baseType(), Type::Double);

  conv = StandardConversion2{ Type::Int, Type::ref(Type::Int) };
  ASSERT_FALSE(conv == StandardConversion2::NotConvertible());
  ASSERT_TRUE(conv.isReferenceConversion());
  ASSERT_FALSE(conv.isNarrowing());
  ASSERT_FALSE(conv.isNumericConversion());
  ASSERT_FALSE(conv.isNumericPromotion());
  ASSERT_FALSE(conv.isDerivedToBaseConversion());
  ASSERT_FALSE(conv.hasQualificationAdjustment());

  conv = StandardConversion2{ Type::cref(Type::Int), Type::ref(Type::Int) };
  ASSERT_TRUE(conv == StandardConversion2::NotConvertible());
  ASSERT_EQ(conv.rank(), ConversionRank::NotConvertible);

  Conversion c = Conversion::compute(Type::Float, Type::Double, &e);
  ASSERT_EQ(c.rank(), ConversionRank::Promotion);
  ASSERT_EQ(c.firstStandardConversion(), StandardConversion2(Type::Float, Type::Double));
  ASSERT_FALSE(c.isNarrowing());
  c = Conversion::compute(Type::Double, Type::Float, &e);
  ASSERT_TRUE(c.isNarrowing());
}

TEST(Conversions, comparison) {
  using namespace script;

  Engine e;

  StandardConversion a{ FloatingPointPromotion };
  StandardConversion b;

  ASSERT_TRUE(a.rank() == StandardConversion::Rank::Promotion);
  ASSERT_TRUE(b.rank() == StandardConversion::Rank::ExactMatch);
  ASSERT_TRUE(b < a);
}

TEST(Conversions, comparisons) {
  using namespace script;

  Engine e;
  e.setup();

  ASSERT_TRUE(StandardConversion2(Type::Int, Type::ref(Type::Int)) < StandardConversion2(Type::Int, Type::cref(Type::Int)));
  ASSERT_TRUE(StandardConversion2(Type::Int, Type::Double) < StandardConversion2(Type::Float, Type::Int));
  ASSERT_FALSE(StandardConversion2(Type::Float, Type::Int) < StandardConversion2(Type::Int, Type::Double));

  ASSERT_FALSE(StandardConversion2(Type::Float, Type::Int) < StandardConversion2::compute(Type::Float, Type::Int, &e));
  ASSERT_FALSE(StandardConversion2::compute(Type::Float, Type::Int, &e) < StandardConversion2(Type::Float, Type::Int));

  ASSERT_TRUE(StandardConversion2(Type::Int, Type::ref(Type::Int)) < StandardConversion2(Type::Int, Type::Int));
  ASSERT_FALSE(StandardConversion2::Copy() < StandardConversion2(Type::Int, Type::ref(Type::Int)));

  std::vector<Conversion> convs{
    Conversion::compute(Type::Float, Type::Double, &e),
    Conversion::compute(Type::Double, Type::Float, &e),
    Conversion::compute(Type::Int, Type::Int, &e),
  };
  ASSERT_EQ(Conversion::globalRank(convs), ConversionRank::Conversion);

  Class A = Symbol{ e.rootNamespace() }.Class("A").get();
  Function ctor_float = A.Constructor().params(Type::Float).create();
  convs.push_back(Conversion::compute(Type::Float, A.id(), &e));
  ASSERT_EQ(Conversion::globalRank(convs), ConversionRank::UserDefinedConversion);

  ASSERT_TRUE(Conversion::comp(Conversion::compute(Type::Float, Type::Double, &e), Conversion::compute(Type::Double, Type::Float, &e)) < 0);
  ASSERT_TRUE(Conversion::comp(Conversion::compute(Type::Double, Type::Float, &e), Conversion::compute(Type::Float, Type::Double, &e)) > 0);

  ASSERT_TRUE(Conversion::comp(Conversion::compute(Type::Double, Type::Float, &e), Conversion::compute(Type::Float, Type::Int, &e)) == 0);

  ASSERT_TRUE(Conversion::comp(Conversion::compute(Type::Double, Type::Float, &e), Conversion::compute(Type::Float, A.id(), &e)) < 0);
}

TEST(Conversions, enum_to_int) {
  using namespace script;

  Engine e;
  e.setup();

  Enum A = e.rootNamespace().Enum("A").get();
  A.addValue("AA", 0);
  A.addValue("AB", 1);
  A.addValue("AC", 2);

  ConversionSequence conv = ConversionSequence::compute(A.id(), Type::Int, &e);
  ASSERT_FALSE(conv == ConversionSequence::NotConvertible());
  ASSERT_FALSE(conv.isUserDefinedConversion());

  conv = ConversionSequence::compute(A.id(), Type::cref(Type::Int), &e);
  ASSERT_FALSE(conv == ConversionSequence::NotConvertible());
  ASSERT_FALSE(conv.isUserDefinedConversion());

  conv = ConversionSequence::compute(A.id(), Type::ref(Type::Int), &e);
  ASSERT_TRUE(conv == ConversionSequence::NotConvertible());

  conv = ConversionSequence::compute(Type::ref(A.id()), Type::ref(Type::Int), &e);
  ASSERT_TRUE(conv == ConversionSequence::NotConvertible());
}

TEST(Conversions, std_conv_enums) {
  using namespace script;

  Engine e;
  e.setup();

  Enum A = e.rootNamespace().Enum("A").get();

  StandardConversion2 conv = StandardConversion2::compute(A.id(), Type::Int, &e);
  ASSERT_EQ(conv, StandardConversion2::EnumToInt());

  conv = StandardConversion2::compute(A.id(), A.id(), &e);
  ASSERT_EQ(conv, StandardConversion2::Copy());

  conv = StandardConversion2::compute(A.id(), Type::ref(A.id()), &e);
  ASSERT_TRUE(conv.isReferenceConversion());

  conv = StandardConversion2::compute(A.id(), Type::Boolean, &e);
  ASSERT_EQ(conv, StandardConversion2::NotConvertible());

  conv = StandardConversion2::compute(A.id(), Type::Double, &e);
  ASSERT_EQ(conv, StandardConversion2::NotConvertible());
}

TEST(Conversions, std_conv_classes) {
  using namespace script;

  Engine e;
  e.setup();

  Class A = e.rootNamespace().Class("A").get();
  A.Constructor().params(Type::cref(A.id())).create();
  Class B = e.rootNamespace().Class("B").setBase(A.id()).get();
  Class C = e.rootNamespace().Class("C").setBase(B.id()).get();

  StandardConversion2 conv = StandardConversion2::compute(A.id(), Type::Int, &e);
  ASSERT_EQ(conv, StandardConversion2::NotConvertible());

  StandardConversion2 b_to_a = StandardConversion2::compute(B.id(), A.id(), &e);
  ASSERT_TRUE(b_to_a.isDerivedToBaseConversion());
  ASSERT_EQ(b_to_a.derivedToBaseConversionDepth(), 1);

  StandardConversion2 c_to_a = StandardConversion2::compute(C.id(), A.id(), &e);
  ASSERT_FALSE(c_to_a.isReferenceConversion());
  ASSERT_TRUE(c_to_a.isDerivedToBaseConversion());
  ASSERT_EQ(c_to_a.derivedToBaseConversionDepth(), 2);

  ASSERT_TRUE(b_to_a < c_to_a);
  ASSERT_FALSE(c_to_a < b_to_a);

  StandardConversion2 c_to_a_ref = StandardConversion2::compute(C.id(), Type::ref(A.id()), &e);
  ASSERT_TRUE(c_to_a_ref.isReferenceConversion());
  ASSERT_TRUE(c_to_a_ref.isDerivedToBaseConversion());
  ASSERT_EQ(c_to_a_ref.derivedToBaseConversionDepth(), 2);

  StandardConversion2 c_to_b = StandardConversion2::compute(C.id(), B.id(), &e);
  ASSERT_EQ(c_to_b, StandardConversion2::NotConvertible()); // B does not have a copy ctor

  StandardConversion2 c_to_b_ref = StandardConversion2::compute(C.id(), Type::ref(B.id()), &e);
  ASSERT_TRUE(c_to_b_ref.isReferenceConversion());
  ASSERT_TRUE(c_to_b_ref.isDerivedToBaseConversion());
  ASSERT_EQ(c_to_b_ref.derivedToBaseConversionDepth(), 1);

  StandardConversion2 string_to_a = StandardConversion2::compute(Type::String, A.id(), &e);
  ASSERT_EQ(string_to_a, StandardConversion2::NotConvertible());
}

TEST(Conversions, user_defined_cast) {
  using namespace script;

  Engine e;
  e.setup();

  Class A = Symbol{ e.rootNamespace() }.Class("A").get();
  Cast to_int = A.Conversion(Type::Int).setConst().create().toCast();

  ConversionSequence conv = ConversionSequence::compute(A.id(), Type::Int, &e);
  ASSERT_FALSE(conv == ConversionSequence::NotConvertible());
  ASSERT_TRUE(conv.isUserDefinedConversion());
  ASSERT_EQ(conv.function, to_int);
}

TEST(Conversions, user_defined_conv_cast) {
  using namespace script;

  Engine e;
  e.setup();

  Class A = Symbol{ e.rootNamespace() }.Class("A").get();
  Cast to_int = A.Conversion(Type::Int).setConst().create().toCast();

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

  Class A = Symbol{ e.rootNamespace() }.Class("A").get();
  Function ctor = A.Constructor().params(Type::Float).create();

  ConversionSequence conv = ConversionSequence::compute(Type::Float, A.id(), &e);
  ASSERT_FALSE(conv == ConversionSequence::NotConvertible());
  ASSERT_TRUE(conv.isUserDefinedConversion());
  ASSERT_EQ(conv.function, ctor);
}

TEST(Conversions, user_defined_converting_constructor2) {
  using namespace script;

  Engine e;
  e.setup();

  Class A = Symbol{ e.rootNamespace() }.Class("A").get();
  Function ctor = A.Constructor().params(Type::Float).create();

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

  Class A = Symbol{ e.rootNamespace() }.Class("A").get();
  Function ctor_int = A.Constructor().params(Type::Int).create();
  Function ctor_bool = A.Constructor().params(Type::Boolean).create();

  ConversionSequence conv = ConversionSequence::compute(Type::Boolean, A.id(), &e);
  ASSERT_FALSE(conv == ConversionSequence::NotConvertible());
  ASSERT_TRUE(conv.isUserDefinedConversion());
  ASSERT_EQ(conv.function, ctor_bool);
}

TEST(Conversions, converting_constructor_selection2) {
  using namespace script;

  Engine e;
  e.setup();

  Class A = Symbol{ e.rootNamespace() }.Class("A").get();
  Function ctor_int = A.Constructor().params(Type::Int).create();
  Function ctor_bool = A.Constructor().params(Type::Boolean).create();

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

  auto ft = e.getFunctionType(Prototype{ Type::Void, Type::Int });

  ConversionSequence conv = ConversionSequence::compute(ft.type(), ft.type(), &e);
  ASSERT_FALSE(conv == ConversionSequence::NotConvertible());
  ASSERT_FALSE(conv.isUserDefinedConversion());
  ASSERT_TRUE(conv.conv1.isCopyInitialization());

  conv = ConversionSequence::compute(ft.type(), ft.type().withFlag(Type::ReferenceFlag), &e);
  ASSERT_FALSE(conv == ConversionSequence::NotConvertible());
  ASSERT_FALSE(conv.isUserDefinedConversion());
  ASSERT_TRUE(conv.conv1.isReferenceInitialization());

  auto ft2 = e.getFunctionType(Prototype{ Type::Void, Type::Float });

  conv = ConversionSequence::compute(ft.type(), ft2.type(), &e);
  ASSERT_TRUE(conv == ConversionSequence::NotConvertible());
}

TEST(Conversions, function_type2) {
  using namespace script;

  Engine e;
  e.setup();

  auto ft = e.getFunctionType(Prototype{ Type::Void, Type::Int });

  Conversion conv = Conversion::compute(ft.type(), ft.type(), &e);
  ASSERT_FALSE(conv == Conversion::NotConvertible());
  ASSERT_FALSE(conv.isUserDefinedConversion());
  ASSERT_EQ(conv.firstStandardConversion(), StandardConversion2::Copy());

  conv = Conversion::compute(ft.type(), ft.type().withFlag(Type::ReferenceFlag), &e);
  ASSERT_FALSE(conv == Conversion::NotConvertible());
  ASSERT_FALSE(conv.isUserDefinedConversion());
  ASSERT_TRUE(conv.firstStandardConversion().isReferenceConversion());

  auto ft2 = e.getFunctionType(Prototype{ Type::Void, Type::Float });

  conv = Conversion::compute(ft.type(), ft2.type(), &e);
  ASSERT_TRUE(conv == Conversion::NotConvertible());
  ASSERT_TRUE(conv.isInvalid());
}

TEST(Conversions, no_converting_constructor) {
  using namespace script;

  Engine e;
  e.setup();

  Class A = Symbol{ e.rootNamespace() }.Class("A").get();

  ConversionSequence conv = ConversionSequence::compute(Type::Float, A.id(), &e);
  ASSERT_TRUE(conv == ConversionSequence::NotConvertible());
}

TEST(Conversions, no_converting_constructor2) {
  using namespace script;

  Engine e;
  e.setup();

  Class A = Symbol{ e.rootNamespace() }.Class("A").get();

  Conversion conv = Conversion::compute(Type::Float, A.id(), &e);
  ASSERT_TRUE(conv == Conversion::NotConvertible());
}

TEST(Conversions, explicit_ctor) {
  using namespace script;

  Engine e;
  e.setup();

  Class A = Symbol{ e.rootNamespace() }.Class("A").get();
  Function ctor_int = A.Constructor().setExplicit().params(Type::Int).create();

  Conversion conv = Conversion::compute(Type::Int, A.id(), &e);
  ASSERT_TRUE(conv == Conversion::NotConvertible());

  Function ctor_bool = A.Constructor().params(Type::Boolean).create();
  conv = Conversion::compute(Type::Int, A.id(), &e);
  ASSERT_EQ(conv.userDefinedConversion(), ctor_bool);

  conv = Conversion::compute(Type::Int, A.id(), &e, Conversion::AllowExplicitConversions);
  ASSERT_EQ(conv.userDefinedConversion(), ctor_int);
}

/****************************************************************
Testing list initializations
****************************************************************/

#include "script/ast/node.h"
#include "script/compiler/expressioncompiler.h"
#include "script/program/expression.h"

TEST(Conversions, list_initialization_default) {
  using namespace script;

  Engine e;
  e.setup();

  auto astlistexpr = ast::ListExpression::New(parser::Token{ parser::Token::LeftBrace, 0, 1, 0, 0 });

  compiler::ExpressionCompiler ec;
  auto listexpr = ec.generateExpression(astlistexpr);

  ASSERT_TRUE(listexpr->is<program::InitializerList>());

  ConversionSequence conv = ConversionSequence::compute(listexpr, Type::Int, &e);
  ASSERT_TRUE(conv.isListInitialization());
  ASSERT_TRUE(conv.listInitialization->kind() == ListInitializationSequence::DefaultInitialization);
  ASSERT_EQ(conv.listInitialization->destType(), Type::Int);

  conv = ConversionSequence::compute(listexpr, Type::String, &e);
  ASSERT_TRUE(conv.isListInitialization());
  ASSERT_TRUE(conv.listInitialization->kind() == ListInitializationSequence::DefaultInitialization);
  ASSERT_EQ(conv.listInitialization->destType(), Type::String);
}


#include "script/parser/parser.h"

std::shared_ptr<script::parser::ParserData> parser_data(const char *source);

TEST(Conversions, list_initialization_constructor) {
  using namespace script;

  Engine e;
  e.setup();

  const char *source =
    "{1, \"Hello\", 3.14}";

  parser::ScriptFragment fragment{ parser_data(source) };
  parser::ExpressionParser parser{ &fragment };

  auto astlistexpr = parser.parse();
  ASSERT_TRUE(astlistexpr->is<ast::ListExpression>());

  compiler::ExpressionCompiler ec;
  ec.setScope(Scope{ e.rootNamespace() });
  auto listexpr = ec.generateExpression(astlistexpr);
  ASSERT_TRUE(listexpr->is<program::InitializerList>());

  Class A = Symbol{ e.rootNamespace() }.Class("A").get();
  Function ctor = A.Constructor().params(Type::Int, Type::String, Type::Double).create();

  ConversionSequence conv = ConversionSequence::compute(listexpr, A.id(), &e);
  ASSERT_TRUE(conv.isListInitialization());
  ASSERT_TRUE(conv.listInitialization->kind() == ListInitializationSequence::ConstructorListInitialization);
  ASSERT_EQ(conv.listInitialization->constructor(), ctor);
  ASSERT_EQ(conv.listInitialization->conversions().size(), 3);
}


TEST(Conversions, list_initialization_not_convertible) {
  using namespace script;

  Engine e;
  e.setup();

  const char *source =
    "{1, \"Hello\", 3.14}";

  parser::ScriptFragment fragment{ parser_data(source) };
  parser::ExpressionParser parser{ &fragment };

  auto astlistexpr = parser.parse();

  compiler::ExpressionCompiler ec;
  ec.setScope(Scope{ e.rootNamespace() });
  auto listexpr = ec.generateExpression(astlistexpr);

  ConversionSequence conv = ConversionSequence::compute(listexpr, Type::String, &e);
  ASSERT_EQ(conv, ConversionSequence::NotConvertible());

  conv = ConversionSequence::compute(listexpr, Type::Int, &e);
  ASSERT_EQ(conv, ConversionSequence::NotConvertible());

  auto initlist = std::static_pointer_cast<program::InitializerList>(listexpr);
  initlist->elements.clear();

  Enum Foo = Symbol{ e.rootNamespace() }.Enum("Foo").get();

  conv = ConversionSequence::compute(listexpr, Foo.id(), &e);
  ASSERT_EQ(conv, ConversionSequence::NotConvertible());

  conv = ConversionSequence::compute(listexpr, Type::ref(Type::Int), &e);
  ASSERT_EQ(conv, ConversionSequence::NotConvertible());
}


/****************************************************************
Testing Initilization class
****************************************************************/

#include "script/initialization.h"

static std::shared_ptr<script::program::Expression> parse_list_expr(script::Engine *e, const std::string & str)
{
  using namespace script;

  parser::ScriptFragment fragment{ parser_data(str.data()) };
  parser::ExpressionParser parser{ &fragment };

  auto astlistexpr = parser.parse();

  compiler::ExpressionCompiler ec;
  ec.setScope(Scope{ e->rootNamespace() });
  return ec.generateExpression(astlistexpr);
}

TEST(Initializations, list_initialization_ctor) {
  using namespace script;

  Engine e;
  e.setup();

  auto listexpr = parse_list_expr(&e, "{1, \"Hello\", 3.14}");
  ASSERT_TRUE(listexpr->is<program::InitializerList>());

  Class A = Symbol{ e.rootNamespace() }.Class("A").get();
  Function ctor = A.Constructor().params(Type::Int, Type::String, Type::Double).create();

  Initialization init = Initialization::compute(A.id(), listexpr, &e);
  ASSERT_EQ(init.kind(), Initialization::ListInitialization);
  ASSERT_EQ(init.rank(), ConversionRank::ExactMatch);
  ASSERT_EQ(init.constructor(), ctor);
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

  Type initializer_list_int = e.getTemplate(Engine::InitializerListTemplate)
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

  Type initializer_list_int = e.getTemplate(Engine::InitializerListTemplate)
    .getInstance({ TemplateArgument{ Type::Int } }).id();

  Class A = Symbol{ e.rootNamespace() }.Class("A").get();
  Function ctor = A.Constructor().params(initializer_list_int).create();

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
  ASSERT_EQ(init.kind(), Initialization::DefaultInitialization);
}