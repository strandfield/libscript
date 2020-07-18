// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include <gtest/gtest.h>

#include "script/class.h"
#include "script/classbuilder.h"
#include "script/classtemplate.h"
#include "script/constructorbuilder.h"
#include "script/conversions.h"
#include "script/engine.h"
#include "script/function.h"
#include "script/functionbuilder.h"
#include "script/initialization.h"
#include "script/initializerlist.h"
#include "script/namelookup.h"
#include "script/namespace.h"
#include "script/symbol.h"
#include "script/typesystem.h"

#include "script/compiler/compiler.h"

TEST(InitializerLists, class_template) {
  using namespace script;

  Engine engine;
  engine.setup();
  
  ClassTemplate ilist_template = ClassTemplate::get<InitializerListTemplate>(&engine);

  Class ilist_int = ilist_template.getInstance({
    TemplateArgument{ Type{Type::Int} }
    });

  ASSERT_TRUE(engine.typeSystem()->isInitializerList(ilist_int.id()));
  ASSERT_FALSE(engine.typeSystem()->isInitializerList(Type::String));

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
#include "script/program/expression.h"

std::shared_ptr<script::parser::ParserContext> parser_context(const char *source);

TEST(InitializerLists, initializer_list_creation) {
  using namespace script;

  Engine engine;
  engine.setup();

  const char *source =
    "{1, 2.0, true}";

  parser::Fragment fragment{ parser_context(source) };
  parser::ExpressionParser parser{ &fragment };

  auto astlistexpr = parser.parse();
  ASSERT_TRUE(astlistexpr->is<ast::ListExpression>());

  compiler::SessionManager session{ engine.compiler() };

  compiler::ExpressionCompiler ec{ engine.compiler() };
  ec.setScope(Scope{ engine.rootNamespace() });
  auto listexpr = ec.generateExpression(astlistexpr);
  ASSERT_TRUE(listexpr->is<program::InitializerList>());


  ClassTemplate ilist_template = ClassTemplate::get<InitializerListTemplate>(&engine);

  Class ilist_int = ilist_template.getInstance({
    TemplateArgument{ Type{ Type::Int } }
    });


  Initialization init = Initialization::compute(ilist_int.id(), listexpr, &engine);
  ASSERT_EQ(init.kind(), Initialization::ListInitialization);
  ASSERT_EQ(init.destType(), ilist_int.id());
  ASSERT_EQ(init.initializations().size(), 3);
}

TEST(InitializerLists, initializer_list_conversion) {
  using namespace script;

  Engine engine;
  engine.setup();

  const char *source =
    "{1, 2.0, true}";

  parser::Fragment fragment{ parser_context(source) };
  parser::ExpressionParser parser{ &fragment };

  auto astlistexpr = parser.parse();
  ASSERT_TRUE(astlistexpr->is<ast::ListExpression>());

  compiler::SessionManager session{ engine.compiler() };

  compiler::ExpressionCompiler ec{ engine.compiler() };
  ec.setScope(Scope{ engine.rootNamespace() });
  auto listexpr = ec.generateExpression(astlistexpr);
  ASSERT_TRUE(listexpr->is<program::InitializerList>());


  ClassTemplate ilist_template = ClassTemplate::get<InitializerListTemplate>(&engine);

  Class ilist_int = ilist_template.getInstance({
    TemplateArgument{ Type{ Type::Int } }
    });

  Class A = Symbol{ engine.rootNamespace() }.newClass("A").get();
  Function ctor = A.newConstructor().params(Type::Int, Type::String).get();
  ctor = A.newConstructor().params(ilist_int.id()).get();

  Initialization init = Initialization::compute(A.id(), listexpr, &engine);
  ASSERT_EQ(init.kind(), Initialization::ListInitialization);
  ASSERT_EQ(init.destType(), A.id());
  ASSERT_EQ(init.constructor(), ctor);
  ASSERT_EQ(init.initializations().size(), 3);
}