// Copyright (C) 2019 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include <gtest/gtest.h>

#include "script/class.h"
#include "script/classbuilder.h"
#include "script/engine.h"
#include "script/namespace.h"
#include "script/typesystem.h"
#include "script/typesystemtransaction.h"

TEST(TypeSystemTests, transaction) {
  using namespace script;


  Engine engine;
  engine.setup();

  Namespace ns = engine.rootNamespace();

  Type A;

  {
    TypeSystemTransaction tr{ engine.typeSystem() };
    A = ns.newClass("A").get().id();
  }

  ASSERT_TRUE(engine.typeSystem()->exists(A));

  Type B;

  try {
    TypeSystemTransaction tr{ engine.typeSystem() };
    B = ns.newClass("B").get().id();
    throw std::runtime_error{ "Nope" };
  }
  catch (...) {

  }

  ASSERT_FALSE(engine.typeSystem()->exists(B));
}
