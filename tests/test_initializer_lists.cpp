// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include <gtest/gtest.h>

#include "script/class.h"
#include "script/classbuilder.h"
#include "script/classtemplate.h"
#include "script/conversions.h"
#include "script/engine.h"
#include "script/function.h"
#include "script/functionbuilder.h"
#include "script/initializerlist.h"
#include "script/namelookup.h"
#include "script/symbol.h"

TEST(InitializerLists, class_template) {
  using namespace script;

  Engine engine;
  engine.setup();
  
  ClassTemplate ilist_template = engine.getTemplate(Engine::InitializerListTemplate);

  Class ilist_int = ilist_template.getInstance({
    TemplateArgument{ Type{Type::Int} }
    });

  ASSERT_TRUE(engine.isInitializerListType(ilist_int.id()));
  ASSERT_FALSE(engine.isInitializerListType(Type::String));

  ASSERT_EQ(ilist_int.classes().size(), 1);
  Class iter = ilist_int.classes().front();

  NameLookup lookup = NameLookup::member("begin", ilist_int);
  ASSERT_EQ(lookup.functions().size(), 1);

  Function begin = lookup.functions().front();
  ASSERT_EQ(begin.returnType(), iter.id());

  lookup = NameLookup::member("end", ilist_int);
  ASSERT_EQ(lookup.functions().size(), 1);

  Function end = lookup.functions().front();
  ASSERT_EQ(end.returnType(), iter.id());

  lookup = NameLookup::member("get", iter);
  ASSERT_EQ(lookup.functions().size(), 1);

  Function get = lookup.functions().front();
  ASSERT_EQ(get.returnType().baseType(), Type::Int);
}


#include "script/parser/parser.h"
#include "script/ast/node.h"
#include "script/compiler/expressioncompiler.h"

std::shared_ptr<script::parser::ParserData> parser_data(const char *source);

TEST(InitializerLists, initializer_list_creation) {
  using namespace script;

  Engine engine;
  engine.setup();

  const char *source =
    "{1, 2.0, true}";

  parser::ScriptFragment fragment{ parser_data(source) };
  parser::ExpressionParser parser{ &fragment };

  auto astlistexpr = parser.parse();
  ASSERT_TRUE(astlistexpr->is<ast::ListExpression>());

  compiler::ExpressionCompiler ec;
  ec.setScope(Scope{ engine.rootNamespace() });
  auto listexpr = ec.generateExpression(astlistexpr);
  ASSERT_TRUE(listexpr->is<program::InitializerList>());


  ClassTemplate ilist_template = engine.getTemplate(Engine::InitializerListTemplate);

  Class ilist_int = ilist_template.getInstance({
    TemplateArgument{ Type{ Type::Int } }
    });


  ConversionSequence conv = ConversionSequence::compute(listexpr, ilist_int.id(), &engine);
  ASSERT_TRUE(conv.isListInitialization());
  ASSERT_TRUE(conv.listInitialization->kind() == ListInitializationSequence::InitializerListCreation);
  ASSERT_EQ(conv.listInitialization->destType(), ilist_int.id());
  ASSERT_EQ(conv.listInitialization->conversions().size(), 3);
}

TEST(InitializerLists, initializer_list_conversion) {
  using namespace script;

  Engine engine;
  engine.setup();

  const char *source =
    "{1, 2.0, true}";

  parser::ScriptFragment fragment{ parser_data(source) };
  parser::ExpressionParser parser{ &fragment };

  auto astlistexpr = parser.parse();
  ASSERT_TRUE(astlistexpr->is<ast::ListExpression>());

  compiler::ExpressionCompiler ec;
  ec.setScope(Scope{ engine.rootNamespace() });
  auto listexpr = ec.generateExpression(astlistexpr);
  ASSERT_TRUE(listexpr->is<program::InitializerList>());


  ClassTemplate ilist_template = engine.getTemplate(Engine::InitializerListTemplate);

  Class ilist_int = ilist_template.getInstance({
    TemplateArgument{ Type{ Type::Int } }
    });

  Class A = Symbol{ engine.rootNamespace() }.Class("A").get();
  Function ctor = A.Constructor().params(Type::Int, Type::String).create();
  ctor = A.Constructor().params(ilist_int.id()).create();

  ConversionSequence conv = ConversionSequence::compute(listexpr, A.id(), &engine);
  ASSERT_TRUE(conv.isListInitialization());
  ASSERT_TRUE(conv.listInitialization->kind() == ListInitializationSequence::InitializerListInitialization);
  ASSERT_EQ(conv.listInitialization->destType(), A.id());
  ASSERT_EQ(conv.listInitialization->constructor(), ctor);
  ASSERT_EQ(conv.listInitialization->conversions().size(), 3);
}