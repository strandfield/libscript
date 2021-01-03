// Copyright (C) 2018-2020 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include <gtest/gtest.h>

#include "script/engine.h"

#include "script/array.h"
#include "script/context.h"
#include "script/function.h"
#include "script/script.h"
#include "script/value.h"

TEST(Eval, test1) {
  using namespace script;

  Engine engine;
  engine.setup();

  Value a = engine.eval("a = 5");
  ASSERT_TRUE(a.type() == Type::Int);

  Context c = engine.currentContext();
  ASSERT_TRUE(c.exists("a"));
  ASSERT_TRUE(a == c.get("a"));

  a = engine.eval("a+3");
  ASSERT_TRUE(a.type() == Type::Int);
  ASSERT_EQ(a.toInt(), 8);

  a = engine.eval(" a <= 5 ");
  ASSERT_TRUE(a.type() == Type::Boolean);
  ASSERT_EQ(a.toBool(), true);
}


TEST(Eval, array1) {
  using namespace script;

  Engine engine;
  engine.setup();

  Value a = engine.eval(" a = [1, 2, 3] ");
  ASSERT_TRUE(a.isArray());
  
  Array aa = a.toArray();
  ASSERT_EQ(aa.size(), 3);
  ASSERT_EQ(aa.at(0).toInt(), 1);

  Value as = engine.eval("a.size()");
  ASSERT_TRUE(as.type() == Type::Int);
  ASSERT_EQ(as.toInt(), 3);
}

TEST(Eval, failure1) {
  using namespace script;

  Engine engine;
  engine.setup();

  ASSERT_ANY_THROW(engine.eval("this"));

  try
  {
    engine.eval("3 + \"Hello\"");
  }
  catch (EngineError & error)
  {
    ASSERT_EQ(error.errorCode(), EngineError::EvaluationError);
  }
}

TEST(Eval, conditional_expression) {
  using namespace script;

  Engine engine;
  engine.setup();

  Value x = engine.eval("3 + 3 != 6 ? 66 : -66");
  ASSERT_EQ(x.type(), Type::Int);
  ASSERT_EQ(x.toInt(), -66);

  x = engine.eval("true ? true : 2");
  ASSERT_EQ(x.type(), Type::Int);
  ASSERT_EQ(x.toInt(), 1);
}

TEST(Engine, references_1) {
  using namespace script;

  Engine engine;
  engine.setup();

  const char* source =
    " void incr(int& n) { n += 1; } ";

  Script s = engine.newScript(SourceFile::fromString(source));
  bool success = s.compile();
  ASSERT_TRUE(success);

  Function incr = s.functions().front();
  ASSERT_EQ(incr.name(), "incr");

  int n = 65;
  Value nn = engine.expose(n);
  ASSERT_EQ(nn.type(), Type::Int);
  ASSERT_TRUE(nn.isReference());
  incr.invoke({ nn });
  ASSERT_EQ(n, 66);
}

TEST(Scripts, conversions) {
  using namespace script;

  const char *source =
    " auto a = 3 * 5.f;                            \n"
    " auto b = true && 1;                          \n"
    " auto c = 3.f * 5.0;                          \n"
    " auto d = 3 + '0';                            \n";

  Engine engine;
  engine.setup();

  Script s = engine.newScript(SourceFile::fromString(source));
  bool success = s.compile();
  ASSERT_TRUE(success);
  ASSERT_EQ(s.globalNames().size(), 4);

  ASSERT_EQ(s.globals().size(), 0);

  s.run();

  ASSERT_EQ(s.globals().size(), 4);

  Value a = s.globals().at(0);
  ASSERT_EQ(a.type(), Type::Float);
  ASSERT_EQ(a.toFloat(), 15.f);
  Value b = s.globals().at(1);
  ASSERT_EQ(b.type(), Type::Boolean);
  ASSERT_EQ(b.toBool(), true);
  Value c = s.globals().at(2);
  ASSERT_EQ(c.type(), Type::Double);
  ASSERT_EQ(c.toDouble(), 15.0);
  Value d = s.globals().at(3);
  ASSERT_EQ(d.type(), Type::Int);
}
