// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include <gtest/gtest.h>

#include "script/engine.h"

#include "script/array.h"
#include "script/context.h"
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


TEST(Scripts, basicoperations) {
  using namespace script;

  const char *source =
    " int a = (3 * 5 - 15 + 3) % 2;                 \n"
    " int b = ((25 / 5) << 2) >> 1;                 \n"
    " bool c = true || false;                       \n"
    " bool d = !(true && false);                    \n"
    " float e = (3.0f - 1.0f) * 2.0f + 2.0f;        \n"
    " float f = e / 6.0f;                           \n"
    " double g = (3.0 - 1.0) * 2.0 + 2.0;           \n"
    " double h = g / 6.0;                           \n"
    " char i = 'i';                                 \n"
    " char j = i + 1;                               \n";

  Engine engine;
  engine.setup();

  Script s = engine.newScript(SourceFile::fromString(source));
  bool success = s.compile();
  ASSERT_TRUE(success);
  ASSERT_EQ(s.globalNames().size(), 10);

  ASSERT_EQ(s.globals().size(), 0);

  s.run();

  ASSERT_EQ(s.globals().size(), 10);

  Value a = s.globals().at(0);
  ASSERT_EQ(a.toInt(), 1);
  Value b = s.globals().at(1);
  ASSERT_EQ(b.toInt(), 10);
  Value c = s.globals().at(2);
  ASSERT_EQ(c.toBool(), true);
  Value d = s.globals().at(3);
  ASSERT_EQ(d.toBool(), true);
  Value e = s.globals().at(4);
  ASSERT_EQ(e.toFloat(), 6.0f);
  Value f = s.globals().at(5);
  ASSERT_EQ(f.toFloat(), 1.0f);
  Value g = s.globals().at(6);
  ASSERT_EQ(g.toDouble(), 6.0);
  Value h = s.globals().at(7);
  ASSERT_EQ(h.toDouble(), 1.0);
  Value i = s.globals().at(8);
  ASSERT_EQ(i.toChar(), 'i');
  Value j = s.globals().at(9);
  ASSERT_EQ(j.toChar(), 'j');
}

TEST(Scripts, assignments) {
  using namespace script;

  const char *source =
    " int a = 0;                           \n"
    " a = 1;                               \n"
    " a += 2; // 3                         \n"
    " a *= 2; // 6                         \n"
    " a /= 2; // 3                         \n"
    " a %= 2; // 1                         \n"
    " a -= 1; // 0                         \n";

  Engine engine;
  engine.setup();

  Script s = engine.newScript(SourceFile::fromString(source));
  bool success = s.compile();
  ASSERT_TRUE(success);

  s.run();

  Value a = s.globals().at(0);
  ASSERT_EQ(a.toInt(), 0);
}

TEST(Scripts, comparisons) {
  using namespace script;

  const char *source =
    " bool a = 3 < 5;                                     \n"
    " bool b = !(6 >= 8);                                 \n"
    " bool c = 1 < 2 && 2 > 1 && 3 >= 3 && 4 <= 4;        \n"
    " bool d = 1.0 < 2.0 && 2.0 > 1.0;                    \n"
    " bool e = 2.0 <= 2.0 && 4.0 >= 3.0;                  \n"
    " bool f = 'i' < 'j' && 'g' >= 'f';                   \n"
    " bool g = 3.f < 4.f && 5.f >= 2.f && 4.f <= 5.f;     \n";

  Engine engine;
  engine.setup();

  Script s = engine.newScript(SourceFile::fromString(source));
  bool success = s.compile();
  ASSERT_TRUE(success);

  s.run();

  for (size_t i(0); i < s.globals().size(); ++i)
  {
    Value x = s.globals().at(i);
    ASSERT_TRUE(x.toBool());
  }
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
