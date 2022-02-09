// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include <gtest/gtest.h>

#include "script/class.h"
#include "script/engine.h"
#include "script/function.h"
#include "script/object.h"
#include "script/scope.h"
#include "script/script.h"
#include "script/typesystem.h"
#include "script/value.h"

#include "script/ast.h"
#include "script/ast/node.h"

TEST(Scenarios, accessing_ast) {
  using namespace script;

  const char *source =
    "  int a = 5;                      "
    "  a += 2;                         "
    "  int foo(int n) { return n; }    ";

  Engine engine;
  engine.setup();

  Script s = engine.newScript(SourceFile::fromString(source));
  bool success = s.compile();
  ASSERT_TRUE(success);

  Ast ast = s.ast();
  ASSERT_FALSE(ast.isNull());

  ASSERT_TRUE(ast.isScript());
  ASSERT_EQ(s, ast.script());

  ASSERT_FALSE(ast.isExpression());

  ASSERT_EQ(ast.expression(), nullptr);

  ASSERT_EQ(ast.statements().size(), 3);
  ASSERT_EQ(ast.declarations().size(), 2);

  auto decl = ast.declarations().back();
  ASSERT_TRUE(decl->is<ast::FunctionDecl>());
  ASSERT_EQ(decl->as<ast::FunctionDecl>().parameterName(0), "n");

  // The Ast is no longer needed, we may request the Script to forget about it
  s.clearAst();
}

#include "script/classbuilder.h"
#include "script/constructorbuilder.h"
#include "script/destructorbuilder.h"
#include "script/interpreter/executioncontext.h"

struct SmallObject
{
  bool data;
};

struct LargeObject
{
  int data[1024];
};

script::Value smallobject_default_ctor(script::FunctionCall* c)
{
  c->thisObject().init<SmallObject>();
  return c->arg(0);
}

script::Value smallobject_dtor(script::FunctionCall* c)
{
  c->thisObject().destroy<SmallObject>();
  return script::Value::Void;
}

script::Value largeobject_default_ctor(script::FunctionCall* c)
{
  c->thisObject().init<LargeObject>();
  return c->arg(0);
}

script::Value largeobject_dtor(script::FunctionCall* c)
{
  c->thisObject().destroy<LargeObject>();
  return script::Value::Void;
}


TEST(Scenarios, custom_type) {
  using namespace script;

  Engine engine;
  engine.setup();
  
  Class largeobject = engine.rootNamespace()
    .newClass("LargeObject").setId(engine.registerType<LargeObject>("LargeObject").data()).get();
  ConstructorBuilder(largeobject).setCallback(largeobject_default_ctor).create();
  DestructorBuilder(largeobject).setCallback(largeobject_dtor).create();

  Value val = engine.construct(largeobject.id(), {});
  ASSERT_EQ(val.type(), largeobject.id());
  engine.destroy(val);

  Class smallobject = engine.rootNamespace()
    .newClass("SmallObject").setId(engine.registerType<SmallObject>("SmallObject").data()).get();
  ConstructorBuilder(smallobject).setCallback(smallobject_default_ctor).create();
  DestructorBuilder(smallobject).setCallback(smallobject_dtor).create();

  val = engine.construct(smallobject.id(), {});
  ASSERT_EQ(val.type(), smallobject.id());
  engine.destroy(val);
}