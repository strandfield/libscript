// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include <gtest/gtest.h>

#include "script/engine.h"
#include "script/script.h"

#include "script/interpreter/interpreter.h"
#include "script/interpreter/debug-handler.h"
#include "script/interpreter/workspace.h"

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


TEST(DebugMode, compilation) {
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
