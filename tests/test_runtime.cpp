// Copyright (C) 2020 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include <gtest/gtest.h>

#include "script/class.h"
#include "script/classbuilder.h"
#include "script/engine.h"
#include "script/function.h"
#include "script/functionbuilder.h"
#include "script/namespace.h"
#include "script/script.h"
#include "script/sourcefile.h"

TEST(TestRuntime, call_undefined_function) {
  using namespace script;

  const char* source =
    "                             \n"
    "  int  f()                   \n"
    "  {                          \n"
    "    int i = 0;               \n"
    "    int j = 1;               \n"
    "    int n = i+j;             \n"
    "    throwing_function();     \n"
    "    return n;                \n"
    "  }                          \n"
    "                             \n"
    "  int g()                    \n"
    "  {                          \n"
    "    return 66;               \n"
    "  }                          \n"
    "                             \n";

  Engine engine;
  engine.setup();

  engine.rootNamespace().newFunction("throwing_function", FunctionBuilder::throwing_body).create();

  Script s = engine.newScript(SourceFile::fromString(source));
  bool success = s.compile();
  const auto& errors = s.messages();
  ASSERT_TRUE(success);

  Function f = s.functions().front();
  Function g = s.functions().back();

  ASSERT_THROW(f.invoke({}), RuntimeError);

  Value n = g.invoke({});
  ASSERT_EQ(n.toInt(), 66);

  engine.destroy(n);
}
