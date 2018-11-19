// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "examples.h"

#include "script/engine.h"
#include "script/functionbuilder.h"
#include "script/script.h"
#include "script/value.h"

#include "script/interpreter/executioncontext.h"

#include <iostream>

DECLARE_EXAMPLE(io);
DECLARE_EXAMPLE(polymorphism);
DECLARE_EXAMPLE(units);

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

int main(int argc, char *argv)
{
  using namespace script;

  for (const auto & ex : Example::list())
  {
    Engine engine;
    engine.setup();

    Namespace ns = engine.rootNamespace();

    ns.newFunction("print", print_callback).params(Type::cref(Type::String)).create();
    ns.newFunction("scan", scan_callback).returns(Type::cref(Type::String)).create();

    ex->init(&engine);

    Script s = engine.newScript(SourceFile{ ex->name + ".script" });
    const bool result = s.compile();
    if (result)
    {
      s.run();
    }
    else
    {
      std::cout << "Could not compile script " << s.source().filepath() << std::endl;
      const auto & messages = s.messages();
      for (const auto & m : messages)
        std::cout << m.to_string() << std::endl;
    }

    std::cout << "-------------------------" << ex->name << std::endl;
  }
}