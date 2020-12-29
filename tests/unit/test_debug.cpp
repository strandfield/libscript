// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include <gtest/gtest.h>

#include "script/engine.h"
#include "script/script.h"

#include "script/interpreter/interpreter.h"
#include "script/interpreter/debug-handler.h"

class CustomDebugHandler : public script::interpreter::DebugHandler
{
public:
  bool triggerred = false;

  void interrupt(script::FunctionCall& call, script::program::Breakpoint& info)
  {
    triggerred = true;
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

  ASSERT_TRUE(!s.breakpoints(3).empty());

  auto debug_handler = std::make_shared<CustomDebugHandler>();

  engine.interpreter()->setDebugHandler(debug_handler);
 
  ASSERT_EQ(s.functions().size(), 1);

  s.functions().front().invoke({});

  ASSERT_TRUE(debug_handler->triggerred);
}
