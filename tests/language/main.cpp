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

constexpr int TERMINAL_WIDTH = 80;
constexpr int NAME_COL_WIDTH = 20;
constexpr int COMPILETIME_COL_WIDTH = 10;
constexpr int RUNTIME_COL_WIDTH = 10;
constexpr int OUTPUT_COL_WIDTH = 35;

std::string CURRENT_OUTPUT = "";

script::Value print_callback(script::FunctionCall *c)
{
  CURRENT_OUTPUT += c->arg(0).toString();
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

static void left_padding(std::string& str, size_t width, char c = ' ')
{
  if (str.size() < width)
    str = std::string(width - str.size(), c) + str;
}

static void right_padding(std::string& str, size_t width, char c = ' ')
{
  while (str.size() < width)
    str.push_back(c);
}

static void print_hline(char c)
{
  for (size_t i(0); i < TERMINAL_WIDTH; ++i)
    std::cout << c;
  std::cout << std::endl;
}

std::string lcol(std::string str, size_t width, char c = ' ')
{
  right_padding(str, width, c);
  return str;
}

std::string rcol(std::string str, size_t width, char c = ' ')
{
  left_padding(str, width, c);
  return str;
}

void print_header()
{
  std::cout << '|' << lcol("Test name", NAME_COL_WIDTH)
    << '|' << lcol("Compil.", COMPILETIME_COL_WIDTH)
    << '|' << lcol("Exec.", RUNTIME_COL_WIDTH)
    << '|' << lcol("Output", OUTPUT_COL_WIDTH) << '|' << std::endl;
}

void print_row(std::string name, std::string compiletime, std::string runtime, std::string output)
{
  std::cout << '|' << lcol(name, NAME_COL_WIDTH)
    << '|' << rcol(compiletime, COMPILETIME_COL_WIDTH)
    << '|' << rcol(runtime, RUNTIME_COL_WIDTH)
    << '|' << lcol(output, OUTPUT_COL_WIDTH) << '|' << std::endl;
}

int main(int argc, char **argv)
{
  using namespace script;

  std::vector<std::string> list = {
    "print",
    "builtin-types",
    "string",
    "while",
    "for",
    "simple-functions",
    "access-global",
    "static-function",
    "static-local-var",
    "default-arguments",
    "enum-assignment",
    "functor",
    "initializer-lists",
    "lambda",
    "list-initialization",
    "converting-ctor",
    "conversion-function",
    "polymorphism",
    "using-directive",
    "using-declaration",
    "namespace-alias",
    "typedef",
    "type-alias",
    "template-function-1",
    "template-function-2",
    "units",
    "math",
  };

  std::string pattern = "";

  print_hline('-');

  if (argc == 2)
  {
    pattern = argv[1];
    std::cout << '|' << lcol("Filter regexp pattern: " + pattern, TERMINAL_WIDTH - 2) << '|' << std::endl;

    print_hline('-');
  }

  print_header();

  print_hline('-');

  Engine engine;
  engine.setup();

  Namespace ns = engine.rootNamespace();

  ns.newFunction("print", print_callback).params(Type::cref(Type::String)).create();
  ns.newFunction("Assert", assert_callback).params(Type(Type::Boolean)).create();

  int nb_failed_compilations = 0;
  int nb_failed_assertions = 0;
  double total_compil_duration = 0.;
  double total_exec_duration = 0.;

  for (const std::string& test_filename : list)
  {
    if (!pattern.empty())
    {
      std::regex pattern_regex{ pattern };

      if (!std::regex_match(test_filename, pattern_regex))
        continue;
    }

    CURRENT_OUTPUT.clear();

    Script s = engine.newScript(SourceFile{ test_filename + ".script" });

    std::string compile_result;

    auto start = std::chrono::high_resolution_clock::now();

    const bool result = s.compile();

    auto end = std::chrono::high_resolution_clock::now();

    if (!result)
    {
      std::string output;

      const auto& messages = s.messages();
      for (const auto& m : messages)
      {
        if (m.severity() == diagnostic::Error)
          output = m.to_string();
      }

      ++nb_failed_compilations;

      print_row(test_filename, lcol("", COMPILETIME_COL_WIDTH, 'X'), "", output);

      continue;
    }

    auto compil_duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    compile_result = std::to_string(compil_duration);
    total_compil_duration += compil_duration;

    start = std::chrono::high_resolution_clock::now();

    try
    {
      s.run();
    }
    catch (script::RuntimeError & err)
    {
      std::cout << "Execution failed with error: " << err.message << "\n";

      ++nb_failed_assertions;

      print_row(test_filename, compile_result, lcol("", RUNTIME_COL_WIDTH, 'X'), err.message);

      continue;
    }
    catch (std::exception& ex)
    {
      std::cout << "Execution failed with error: " << ex.what() << "\n";

      ++nb_failed_assertions;

      print_row(test_filename, compile_result, lcol("", RUNTIME_COL_WIDTH, 'X'), ex.what());

      continue;
    }

    end = std::chrono::high_resolution_clock::now();
    auto execution_duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

    std::string exec_time = std::to_string(execution_duration);
    total_exec_duration += execution_duration;

    print_row(test_filename, compile_result, exec_time, CURRENT_OUTPUT);
  }

  int total_failures = nb_failed_compilations + nb_failed_assertions;

  print_hline('-');

  std::cout << '|' << lcol(std::to_string(total_failures) + " test(s) failed.", TERMINAL_WIDTH - 2)  << '|' << std::endl;

  print_hline('-');

  std::cout << '|' << lcol(std::string("Total: ") + std::to_string(total_compil_duration) + " compilation, " + std::to_string(total_exec_duration) + " exec.", TERMINAL_WIDTH - 2) << '|' << std::endl;

  print_hline('-');

  return nb_failed_compilations + nb_failed_assertions;
}
