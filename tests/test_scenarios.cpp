// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include <gtest/gtest.h>

#include "script/class.h"
#include "script/engine.h"
#include "script/function.h"
#include "script/object.h"
#include "script/scope.h"
#include "script/script.h"
#include "script/value.h"

TEST(Scenarios, manual_construction_and_delegate_ctor) {
  using namespace script;

  const char *source =
    "  class Foo                  "
    "  {                          "
    "  public:                    "
    "    int n;                   "
    "  public:                    "
    "    Foo(int a) : n(a) { }    "
    "    Foo() : Foo(10) { }      "
    "    ~Foo() { }               "
    "  };                         ";

  Engine engine;
  engine.setup();

  Script s = engine.newScript(SourceFile::fromString(source));
  bool success = s.compile();
  ASSERT_TRUE(success);

  ASSERT_ANY_THROW(engine.typeId("Foo"));

  Type foo_type = engine.typeId("Foo", Scope{ s });
  Class foo = engine.getClass(foo_type);

  // We create a value of type Foo without initializing it
  Value val = engine.allocate(foo_type);

  Function default_ctor = foo.defaultConstructor();
  ASSERT_FALSE(default_ctor.isNull());

  // We manually call Foo's constructor
  engine.invoke(default_ctor, { val });

  Object obj = val.toObject();
  ASSERT_EQ(obj.size(), 1);

  Value n = obj.at(0);
  ASSERT_EQ(n.toInt(), 10);

  Function dtor = foo.destructor();
  ASSERT_FALSE(dtor.isNull());

  // We call the destructor manually
  engine.invoke(dtor, { val });

  // We free the memory
  engine.free(val);
}

