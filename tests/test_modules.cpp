// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include <cmath>

#include <gtest/gtest.h>

#include "script/compiler/compilererrors.h"
#include "script/engine.h"
#include "script/functionbuilder.h"
#include "script/interpreter/executioncontext.h"
#include "script/module.h"

namespace callbacks
{

using namespace script;

Value max(FunctionCall *c)
{
  double a = c->arg(0).toDouble();
  double b = c->arg(1).toDouble();
  return c->engine()->newDouble(std::max(a, b));
}

Value cos(FunctionCall *c)
{
  return c->engine()->newDouble(std::cos(c->arg(0).toDouble()));
}

} // namespace callbacks

void load_math_module(script::Module math)
{
  using namespace script;

  Namespace ns = math.root();

  ns.Function("max", callbacks::max).returns(Type::Double).params(Type::cref(Type::Double), Type::cref(Type::Double)).create();
  ns.Function("cos", callbacks::cos).returns(Type::Double).params(Type::cref(Type::Double)).create();
}

void cleanup_module(script::Module)
{
}


TEST(ModuleTests, simple_module) {
  using namespace script;

  Engine engine;
  engine.setup();

  Module math = engine.newModule("math", load_math_module, cleanup_module);

  const char *source =
    "  import math;         "
    "  double y = cos(0);   ";

  Script s = engine.newScript(SourceFile::fromString(source));
  bool success = s.compile();
  const auto & errors = s.messages();

  ASSERT_TRUE(success);

  s.run();

  ASSERT_EQ(s.globals().size(), 1);

  Value y = s.globals().front();
  ASSERT_EQ(y.type(), Type::Double);
  ASSERT_EQ(y.toDouble(), 1.0);
}

TEST(ModuleTests, unknown_module) {
  using namespace script;

  Engine engine;
  engine.setup();

  //Module math = engine.newModule("math", load_math_module, cleanup_module);

  const char *source =
    "  import math;         "
    "  double y = cos(0);   ";

  Script s = engine.newScript(SourceFile::fromString(source));
  bool success = s.compile();
  const auto & errors = s.messages();

  ASSERT_FALSE(success);
  ASSERT_EQ(errors.size(), 1);
  //std::cout << errors.front().message() << std::endl;
  ASSERT_EQ(errors.front().code(), compiler::UnknownModuleName{"math"}.code());
}

void load_trig_module(script::Module trig)
{
  using namespace script;

  Namespace ns = trig.root();

  ns.Function("cos", callbacks::cos).returns(Type::Double).params(Type::cref(Type::Double)).create();
}

void load_misc_module(script::Module misc)
{
  using namespace script;

  Namespace ns = misc.root();

  ns.Function("max", callbacks::max).returns(Type::Double).params(Type::cref(Type::Double), Type::cref(Type::Double)).create();
}

TEST(ModuleTests, sub_module) {
  using namespace script;

  Engine engine;
  engine.setup();

  Module math = engine.newModule("math");
  Module trig = math.newSubModule("trig", load_trig_module, cleanup_module);

  const char *source =
    "  import math.trig;    "
    "  double y = cos(0);   ";

  Script s = engine.newScript(SourceFile::fromString(source));
  bool success = s.compile();
  const auto & errors = s.messages();

  ASSERT_TRUE(success);

  s.run();

  ASSERT_EQ(s.globals().size(), 1);

  Value y = s.globals().front();
  ASSERT_EQ(y.type(), Type::Double);
  ASSERT_EQ(y.toDouble(), 1.0);
}

TEST(ModuleTests, loading_wrong_submodule) {
  using namespace script;

  Engine engine;
  engine.setup();

  Module math = engine.newModule("math");
  Module trig = math.newSubModule("trig", load_trig_module, cleanup_module);
  Module misc = math.newSubModule("misc", load_misc_module, cleanup_module);
  misc.load();

  const char *source =
    "  import math.trig;    "
    "  int n = max(1, 2);   ";

  Script s = engine.newScript(SourceFile::fromString(source));
  bool success = s.compile();
  const auto & errors = s.messages();

  ASSERT_FALSE(success);
}


TEST(ModuleTests, sub_module_auto_loading) {
  using namespace script;

  Engine engine;
  engine.setup();

  Module math = engine.newModule("math");
  Module trig = math.newSubModule("trig", load_trig_module, cleanup_module);

  const char *source =
    "  import math;    "
    "  double y = cos(0);   ";

  Script s = engine.newScript(SourceFile::fromString(source));
  bool success = s.compile();
  const auto & errors = s.messages();

  ASSERT_TRUE(success);

  s.run();

  ASSERT_EQ(s.globals().size(), 1);

  Value y = s.globals().front();
  ASSERT_EQ(y.type(), Type::Double);
  ASSERT_EQ(y.toDouble(), 1.0);
}

TEST(ModuleTests, unknown_submodule) {
  using namespace script;

  Engine engine;
  engine.setup();

  Module math = engine.newModule("math", load_math_module, cleanup_module);

  const char *source =
    "  import math.trig;    "
    "  double y = cos(0);   ";

  Script s = engine.newScript(SourceFile::fromString(source));
  bool success = s.compile();
  const auto & errors = s.messages();

  ASSERT_FALSE(success);
  ASSERT_EQ(errors.size(), 1);
  //std::cout << errors.front().message() << std::endl;
  ASSERT_EQ(errors.front().code(), compiler::UnknownSubModuleName("trig", "math").code());
}


TEST(ModuleTests, script_module) {
  using namespace script;

  Engine engine;
  engine.setup();

  engine.setScriptExtension(".m");

  Script s = engine.newScript(SourceFile{"bar.m"});
  bool success = s.compile();
  const auto & errors = s.messages();

  ASSERT_TRUE(success);

  s.run();

  ASSERT_EQ(s.globals().size(), 1);

  Value a = s.globals().front();
  ASSERT_EQ(a.type(), Type::Int);
  ASSERT_EQ(a.toInt(), 4);
}

TEST(ModuleTests, script_module_import_inside_function_body) {
  using namespace script;

  Engine engine;
  engine.setup();

  engine.setScriptExtension(".m");

  Script s = engine.newScript(SourceFile{ "qux.m" });
  bool success = s.compile();
  const auto & errors = s.messages();

  ASSERT_TRUE(success);

  s.run();

  ASSERT_EQ(s.globals().size(), 1);

  Value a = s.globals().front();
  ASSERT_EQ(a.type(), Type::Int);
  ASSERT_EQ(a.toInt(), 6);
}