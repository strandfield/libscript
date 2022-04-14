// Copyright (C) 2022 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/engine.h"
#include "script/function.h"
#include "script/functionbuilder.h"
#include "script/namespace.h"
#include "script/script.h"
#include "script/interpreter/executioncontext.h"

#include <iostream>

script::Value print_callback(script::FunctionCall* c)
{
  std::cout << c->arg(0).toString() << std::endl;
  return script::Value::Void;
}

int main()
{
  using namespace script;

  Engine e;
  e.setup();

  FunctionBuilder::Fun(e.rootNamespace(), "print")
    .setCallback(print_callback)
    .params(Type::cref(Type::String))
    .create();

  const char* source = R"(
     print("Hello World!");
  )";

  Script s = e.newScript(SourceFile::fromString(source));
  
  if (s.compile())
    s.run();
}
