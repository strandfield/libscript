// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include <gtest/gtest.h>

#include "script/engine.h"
#include "script/namespace.h"
#include "script/script.h"
#include "script/value.h"

TEST(StringTests, construction) {
  using namespace script;

  const char *source =
    "  String a = \"Hello World !\"; ";

  Engine engine;
  engine.setup();

  Script s = engine.newScript(SourceFile::fromString(source));
  bool success = s.compile();
  ASSERT_TRUE(success);

  s.run();

  ASSERT_EQ(s.globals().size(), 1);
  Value a = s.globals().back();
  ASSERT_EQ(a.type(), Type::String);
  ASSERT_EQ(a.toString(), "Hello World !");
}

TEST(StringTests, assignment) {
  using namespace script;

  const char *source =
    "  String a = \"Hello World !\"; "
    "  a = \"Good bye !\";           ";

  Engine engine;
  engine.setup();

  Script s = engine.newScript(SourceFile::fromString(source));
  bool success = s.compile();
  const auto & errors = s.messages();
  ASSERT_TRUE(success);

  s.run();

  ASSERT_EQ(s.globals().size(), 1);
  Value a = s.globals().back();
  ASSERT_EQ(a.type(), Type::String);
  ASSERT_EQ(a.toString(), "Good bye !");
}

TEST(StringTests, methods_1) {
  using namespace script;

  const char *source =
    "  String a = \"Hello World !\";       "
    "  bool empty = a.empty();             "
    "  int size = a.size();                "
    "  char c = a.at(0);                   "
    "  a.replace(0, 5, \"Goodbye\");       "
    "  bool eq = a != \"Hello World !\";   ";

  Engine engine;
  engine.setup();

  Script s = engine.newScript(SourceFile::fromString(source));
  bool success = s.compile();
  const auto & errors = s.messages();
  ASSERT_TRUE(success);

  s.run();

  ASSERT_EQ(s.globals().size(), 5);

  Value empty = s.globals().at(1);
  ASSERT_EQ(empty.type(), Type::Boolean);
  ASSERT_EQ(empty.toBool(), false);

  Value size = s.globals().at(2);
  ASSERT_EQ(size.type(), Type::Int);
  ASSERT_EQ(size.toInt(), 13);

  Value c = s.globals().at(3);
  ASSERT_EQ(c.type(), Type::Char);
  ASSERT_EQ(c.toChar(), 'H');

  Value eq = s.globals().at(4);
  ASSERT_EQ(eq.type(), Type::Boolean);
  ASSERT_EQ(eq.toBool(), true);

  Value a = s.globals().at(0);
  ASSERT_EQ(a.type(), Type::String);
  ASSERT_EQ(a.toString(), "Goodbye World !");
}

TEST(StringTests, methods_2) {
  using namespace script;

  const char *source =
    "  String a = \"Hello World !\";       "
    "  a.erase(6, 6);                      "
    "  bool b = a == \"Hello !\";          "
    "  a.insert(6, \"Bob\");               "
    "  bool c = a == \"Hello Bob!\";       "
    "  a.clear();                          ";

  Engine engine;
  engine.setup();

  Script s = engine.newScript(SourceFile::fromString(source));
  bool success = s.compile();
  const auto & errors = s.messages();
  ASSERT_TRUE(success);

  s.run();

  ASSERT_EQ(s.globals().size(), 3);

  Value a = s.globals().at(0);
  ASSERT_EQ(a.type(), Type::String);
  ASSERT_EQ(a.toString(), "");

  Value b = s.globals().at(1);
  ASSERT_EQ(b.type(), Type::Boolean);
  ASSERT_EQ(b.toBool(), true);

  Value c = s.globals().at(2);
  ASSERT_EQ(c.type(), Type::Boolean);
  ASSERT_EQ(c.toBool(), true);
}

TEST(StringTests, subscript) {
  using namespace script;

  const char *source =
    "  String str = \"abc\";       "
    "  char c = str[0];            "
    "  str[2] = 'a';               ";

  Engine engine;
  engine.setup();

  Script s = engine.newScript(SourceFile::fromString(source));
  bool success = s.compile();
  const auto & errors = s.messages();
  ASSERT_TRUE(success);

  s.run();

  ASSERT_EQ(s.globals().size(), 2);

  Value str = s.globals().at(0);
  ASSERT_EQ(str.type(), Type::String);
  ASSERT_EQ(str.toString(), "aba");

  Value c = s.globals().at(1);
  ASSERT_EQ(c.type(), Type::Char);
  ASSERT_EQ(c.toChar(), 'a');
}

TEST(StringTests, operations) {
  using namespace script;

  const char *source =
    "  String str = \"abc\";           "
    "  str = str + \"def\";            "
    "  bool leq = str <= \"abcdef\";   ";

  Engine engine;
  engine.setup();

  Script s = engine.newScript(SourceFile::fromString(source));
  bool success = s.compile();
  const auto & errors = s.messages();
  ASSERT_TRUE(success);

  s.run();

  ASSERT_EQ(s.globals().size(), 2);

  Value str = s.globals().at(0);
  ASSERT_EQ(str.type(), Type::String);
  ASSERT_EQ(str.toString(), "abcdef");

  Value leq = s.globals().at(1);
  ASSERT_EQ(leq.type(), Type::Boolean);
  ASSERT_EQ(leq.toBool(), true);
}
