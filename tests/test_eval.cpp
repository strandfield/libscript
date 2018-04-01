// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include <gtest/gtest.h>

#include "script/engine.h"

#include "script/array.h"
#include "script/context.h"

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
