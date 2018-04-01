// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include <iostream>

#include "script/engine.h"
#include "script/functionbuilder.h"
#include "script/interpreter/executioncontext.h"

script::Value print_callback(script::FunctionCall *c)
{
  std::cout << c->arg(0).toString() << std::endl;
  return script::Value::Void;
}

script::Value scan_callback(script::FunctionCall *c)
{
  std::string ret;
  std::cin >> ret;
  return c->engine()->newString(ret);
}

int main()
{
  using namespace script;

  Engine e;
  e.setup();

  auto b = FunctionBuilder::Function("print", Prototype{}, print_callback)
    .addParam(Type::cref(Type::String));
  e.rootNamespace().newFunction(b);

  b = FunctionBuilder::Function("scan", Prototype{ Type::String }, scan_callback);
  e.rootNamespace().newFunction(b);
  
  Script s = e.newScript(SourceFile{ "io.script" });
  const bool result = s.compile();
  if (result)
    s.run();
  else
  {
    std::cout << "Could not compile script " << s.source().filepath() << std::endl;
    const auto & messages = s.messages();
    for (const auto & m : messages)
      std::cout << m.to_string() << std::endl;
  }
}