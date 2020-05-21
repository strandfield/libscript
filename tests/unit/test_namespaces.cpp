// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include <gtest/gtest.h>

#include "script/engine.h"
#include "script/functionbuilder.h"
#include "script/literals.h"
#include "script/literaloperatorbuilder.h"
#include "script/namespace.h"


TEST(NamespaceTest, user_defined_literals) {
  using namespace script;

  Engine engine;
  engine.setup();

  Namespace ns = engine.rootNamespace();

  LiteralOperator lop = ns.newUserDefinedLiteral("s", Type::Int, Type::Double).get();
  ASSERT_EQ(lop.suffix(), "s");
  ASSERT_EQ(lop.returnType(), Type::Double);
  ASSERT_EQ(lop.input(), Type::Int);
}

