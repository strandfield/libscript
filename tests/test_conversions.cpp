// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include <gtest/gtest.h>

#include "script/cast.h"
#include "script/class.h"
#include "script/classbuilder.h"
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



TEST(Conversions, comparison) {
  using namespace script;

  Engine e;

  StandardConversion a{ FloatingPointPromotion };
  StandardConversion b;

  ASSERT_TRUE(a.rank() == StandardConversion::Rank::Promotion);
  ASSERT_TRUE(b.rank() == StandardConversion::Rank::ExactMatch);
  ASSERT_TRUE(b < a);
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

TEST(Conversions, no_converting_constructor) {
  using namespace script;

  Engine e;
  e.setup();

  Class A = Symbol{ e.rootNamespace() }.Class("A").get();

  ConversionSequence conv = ConversionSequence::compute(Type::Float, A.id(), &e);
  ASSERT_TRUE(conv == ConversionSequence::NotConvertible());
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