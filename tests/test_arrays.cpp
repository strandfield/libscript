// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include <gtest/gtest.h>

#include "script/array.h"
#include "script/engine.h"


TEST(Arrays, impl) {
  using namespace script;

  Engine engine;
  engine.setup();
  
  Array a = engine.newArray(Engine::ElementType{ Type::Int });
  ASSERT_FALSE(a.isNull());
  ASSERT_EQ(a.elementTypeId(), Type::Int);
  ASSERT_EQ(a.size(), 0);

  a.resize(10);
  ASSERT_EQ(a.size(), 10);
  ASSERT_EQ(a[0].type(), Type::Int);

  Value n = a[0];
  a[0] = engine.newInt(66);
  engine.destroy(n);

  ASSERT_EQ(a.at(0).toInt(), 66);

  Array b = a;
  ASSERT_EQ(b.impl(), a.impl());

  b.detach();
  ASSERT_EQ(b.size(), a.size());
  ASSERT_EQ(b.at(0).toInt(), 66);

  n = b[0];
  b[0] = engine.newInt(47);
  engine.destroy(n);

  ASSERT_EQ(b.at(0).toInt(), 47);
  ASSERT_EQ(a.at(0).toInt(), 66);

  // no-op
  auto impl = a.impl();
  a.assign(a);
  ASSERT_EQ(a.impl(), impl);


  a.resize(0);
  ASSERT_EQ(a.size(), 0);

  // no-op
  a.resize(-1);
  ASSERT_EQ(a.size(), 0);
}


#include "script/script.h"

TEST(Arrays, binding) {
  using namespace script;

  Engine engine;
  engine.setup();

  const char *src =
    "  Array<int> a = [1, 2, 3, 4, 5];   \n"
    "  Array<int> b = a;                 \n"
    "  Array<int> c(10);                 \n"
    "  b[0] = 5;                         \n"
    "  int d = a[0];                     \n"
    "  int e = b[0];                     \n"
    "  a.resize(10);                     \n"
    "  Array<int> f;                     \n"
    "  Array<int> g;                     \n"
    "  g = b;                            \n";

  Script s = engine.newScript(SourceFile::fromString(src));
  bool success = s.compile();
  ASSERT_TRUE(success);

  s.run();

  ASSERT_EQ(s.globals().size(), 7);

  ASSERT_TRUE(s.globals().at(0).isArray());
  ASSERT_TRUE(s.globals().at(1).isArray());
  ASSERT_TRUE(s.globals().at(2).isArray());
  ASSERT_TRUE(s.globals().at(5).isArray());
  ASSERT_TRUE(s.globals().at(6).isArray());
  ASSERT_EQ(s.globals().at(3).type(), Type::Int);
  ASSERT_EQ(s.globals().at(4).type(), Type::Int);

  Array a = s.globals().at(0).toArray();
  Array b = s.globals().at(1).toArray();
  Array c = s.globals().at(2).toArray();
  Array f = s.globals().at(5).toArray();
  int d = s.globals().at(3).toInt();
  int e = s.globals().at(4).toInt();
  Array g = s.globals().at(6).toArray();

  ASSERT_EQ(d, 1);
  ASSERT_EQ(e, 5);
  ASSERT_EQ(a.size(), 10);
  ASSERT_EQ(c.size(), 10);
  ASSERT_EQ(f.size(), 0);

  ASSERT_EQ(g.size(), 5);
  ASSERT_EQ(g.at(0).toInt(), 5);
  ASSERT_EQ(g.at(1).toInt(), 2);
}
