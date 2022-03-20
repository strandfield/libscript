// Copyright (C) 2020 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/engine.h"
#include "script/functionbuilder.h"
#include "script/script.h"
#include "script/value.h"

#include "script/interpreter/executioncontext.h"

#include <iostream>
#include <regex>

static std::string parse_testname(std::string filename)
{
  size_t pos = filename.find("/test-");
  return std::string(filename.begin() + pos + 6, filename.end() - 7);
}

static std::string parse_output(const script::SourceFile& srcfile)
{
  const std::string& src = srcfile.content();

  std::string ret;

  auto pos = src.find("//>");

  while (pos != std::string::npos)
  {
    auto eol = src.find('\n', pos);
    ret.insert(ret.end(), src.begin() + pos + 3, src.begin() + eol + 1);
    pos = src.find("//>", eol);
  }

  return ret;
}

int main(int argc, char **argv)
{
  using namespace script;

  std::vector<std::string> list = {
#include "test-list.h"
  };

  std::string pattern = "";

  if (argc == 2)
  {
    pattern = argv[1];
    std::cout << "Filter regexp pattern: " << pattern << std::endl;
  }

  Engine engine;
  std::cout << "Engine setup..." << std::endl;
  engine.setup();

  Namespace ns = engine.rootNamespace();

  FunctionBuilder::Fun(ns, "print").params(Type::cref(Type::String)).create();

  int total_tests = 0;
  int total_failures = 0;

  std::cout << "Running tests..." << std::endl;


  for (const std::string& test_filename : list)
  {
    std::string test_name = parse_testname(test_filename);

    if (!pattern.empty())
    {
      std::regex pattern_regex{ pattern };

      if (!std::regex_match(test_name, pattern_regex))
        continue;
    }

    ++total_tests;

    std::cout << "Test " << test_name << "... ";

    Script s = engine.newScript(SourceFile{ test_filename });

    const bool result = s.compile();

    if (result)
    {
      ++total_failures;
      std::cout << " FAIL!" << std::endl;
      std::cout << "... script compiled successfully" << std::endl;
    }
    else
    {
      std::string output;

      const auto& messages = s.messages();
      for (const auto& m : messages)
      {
        if (m.severity() == diagnostic::Error)
          output += m.to_string() + '\n';
      }

      std::string expected = parse_output(s.source());

      if (output != expected)
      {
        ++total_failures;
        std::cout << " FAIL!" << std::endl;
        std::cout << "Expected:\n" << expected << std::endl;
        std::cout << "Got:\n" << output << std::endl;
      }
      else
      {
        std::cout << " PASS!" << std::endl;
      }
    }
  }

  std::cout << std::to_string(total_tests) << " test(s) run." << std::endl;
  std::cout << std::to_string(total_failures) << " test(s) failed." << std::endl;

  return total_failures;
}
