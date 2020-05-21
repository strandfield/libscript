// Copyright (C) 2020 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/engine.h"
#include "script/functionbuilder.h"
#include "script/script.h"
#include "script/value.h"

#include "script/interpreter/executioncontext.h"

#include <chrono>
#include <iostream>
#include <regex>

script::Value print_callback(script::FunctionCall *c)
{
  std::cout << c->arg(0).toString() << std::endl;
  return script::Value::Void;
}

script::Value assert_callback(script::FunctionCall* c)
{
  if (!c->arg(0).toBool())
  {
    throw script::RuntimeError{"Assertion failure"};
  }

  return script::Value::Void;
}

int main(int argc, char **argv)
{
  using namespace script;

  std::vector<std::string> list = {
    "print",
    "polymorphism",
    "units",
    "math",
  };

  std::string pattern = "";

  if (argc == 2)
  {
    pattern = argv[1];
    std::cout << "Filter regexp pattern: " << pattern << std::endl;
  }

  Engine engine;
  engine.setup();

  Namespace ns = engine.rootNamespace();

  ns.newFunction("print", print_callback).params(Type::cref(Type::String)).create();
  ns.newFunction("Assert", assert_callback).params(Type(Type::Boolean)).create();

  int nb_failed_compilations = 0;
  int nb_failed_assertions = 0;

  for (const std::string& test_filename : list)
  {
    if (!pattern.empty())
    {
      std::regex pattern_regex{ pattern };

      if (!std::regex_match(test_filename, pattern_regex))
        continue;
    }

    Script s = engine.newScript(SourceFile{ test_filename + ".script" });

    auto start = std::chrono::high_resolution_clock::now();

    const bool result = s.compile();

    if (!result)
    {
      std::cout << "Could not compile script " << s.source().filepath() << std::endl;
      const auto& messages = s.messages();
      for (const auto& m : messages)
        std::cout << m.to_string() << std::endl;

      std::cout << "\n" << std::endl;

      ++nb_failed_compilations;

      continue;
    }

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> compilation_duration = end - start;

    start = std::chrono::high_resolution_clock::now();

    try
    {
      s.run();
    }
    catch (script::RuntimeError & err)
    {
      std::cout << "Execution failed with error: " << err.message << "\n";

      ++nb_failed_assertions;

      continue;
    }
    catch (std::exception& ex)
    {
      std::cout << "Execution failed with error: " << ex.what() << "\n";

      ++nb_failed_assertions;

      continue;
    }

    end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> execution_duration = end - start;

    std::cout << test_filename << " " << (compilation_duration.count() * 1000.) << " " << (execution_duration.count() * 1000.) << std::endl;
  }

  return nb_failed_compilations + nb_failed_assertions;
}
