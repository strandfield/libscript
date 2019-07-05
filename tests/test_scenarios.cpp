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

TEST(Scenarios, manual_construction_and_delegate_ctor) {
  using namespace script;

  const char *source =
    "  class Foo                  "
    "  {                          "
    "  public:                    "
    "    int n;                   "
    "  public:                    "
    "    Foo(int a) : n(a) { }    "
    "    Foo() : Foo(10) { }      "
    "    ~Foo() { }               "
    "  };                         ";

  Engine engine;
  engine.setup();

  Script s = engine.newScript(SourceFile::fromString(source));
  bool success = s.compile();
  ASSERT_TRUE(success);

  ASSERT_ANY_THROW(engine.typeId("Foo"));

  Type foo_type = engine.typeId("Foo", Scope{ s });
  Class foo = engine.typeSystem()->getClass(foo_type);

  // We create a value of type Foo without initializing it
  Value val = engine.allocate(foo_type);

  Function default_ctor = foo.defaultConstructor();
  ASSERT_FALSE(default_ctor.isNull());

  // We manually call Foo's constructor
  engine.invoke(default_ctor, { val });

  Object obj = val.toObject();
  ASSERT_EQ(obj.size(), 1);

  Value n = obj.at(0);
  ASSERT_EQ(n.toInt(), 10);

  Function dtor = foo.destructor();
  ASSERT_FALSE(dtor.isNull());

  // We call the destructor manually
  engine.invoke(dtor, { val });

  // We free the memory
  engine.free(val);
}

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
  ASSERT_FALSE(ast.hasErrors());
  ASSERT_EQ(ast.messages().size(), 0);

  ASSERT_EQ(ast.expression(), nullptr);

  ASSERT_EQ(ast.statements().size(), 3);
  ASSERT_EQ(ast.declarations().size(), 2);

  auto decl = ast.declarations().back();
  ASSERT_TRUE(decl->is<ast::FunctionDecl>());
  ASSERT_EQ(decl->as<ast::FunctionDecl>().parameterName(0), "n");

  // The Ast is no longer needed, we may request the Script to forget about it
  s.clearAst();
}

