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

#include "script/interpreter/interpreter.h"
#include "script/interpreter/debug-handler.h"
#include "script/interpreter/workspace.h"

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

  FunctionBuilder::Fun(engine.rootNamespace(), "throwing_function").setCallback(FunctionBuilder::throwing_body).create();

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


class CustomDebugHandler : public script::interpreter::DebugHandler
{
public:
  size_t size = 0;
  script::Type type;
  std::string name;
  int value;

  void interrupt(script::FunctionCall& call, script::program::Breakpoint& info)
  {
    if (info.status)
    {
      script::interpreter::Workspace w{ &call };
      size = w.size();
      type = w.varTypeAt(0);
      name = w.nameAt(0);
      value = w.valueAt(0).toInt();
    }
  }
};


TEST(TestRuntime, debugcompilation) {
  using namespace script;

  const char* source =
    "                  \n"
    "  void main()     \n"
    "  {               \n"
    "    int a = 5;    \n"
    "    if(a > 2)     \n"
    "    {             \n"
    "      int b = 2;  \n"
    "  	   b = a + b;  \n"
    "    }             \n"
    "  }               \n"
    "                  \n";

  Engine engine;
  engine.setup();

  Script s = engine.newScript(SourceFile::fromString(source));
  bool success = s.compile(CompileMode::Debug);
  const auto& errors = s.messages();
  ASSERT_TRUE(success);

  auto bps = s.breakpoints(4);
  ASSERT_TRUE(bps.size() == 1);

  bps.front().second->status = 1;

  auto debug_handler = std::make_shared<CustomDebugHandler>();

  engine.interpreter()->setDebugHandler(debug_handler);

  ASSERT_EQ(s.functions().size(), 1);

  s.functions().front().invoke({});

  ASSERT_TRUE(debug_handler->size == 1);
  ASSERT_TRUE(debug_handler->type == script::Type::Int);
  ASSERT_TRUE(debug_handler->name == "a");
  ASSERT_TRUE(debug_handler->value == 5);
}
