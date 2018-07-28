// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include <gtest/gtest.h>

#include "script/class.h"
#include "script/classtemplate.h"
#include "script/engine.h"
#include "script/function.h"
#include "script/initializerlist.h"
#include "script/namelookup.h"

TEST(InitializerLists, class_template) {
  using namespace script;

  Engine engine;
  engine.setup();
  
  ClassTemplate ilist_template = engine.getTemplate(Engine::InitializerListTemplate);

  Class ilist_int = ilist_template.getInstance({
    TemplateArgument{ Type{Type::Int} }
    });

  ASSERT_EQ(ilist_int.classes().size(), 1);

  Class iter = ilist_int.classes().front();

  NameLookup lookup = NameLookup::member("begin", ilist_int);
  ASSERT_EQ(lookup.functions().size(), 1);

  Function begin = lookup.functions().front();
  ASSERT_EQ(begin.returnType(), iter.id());

  lookup = NameLookup::member("end", ilist_int);
  ASSERT_EQ(lookup.functions().size(), 1);

  Function end = lookup.functions().front();
  ASSERT_EQ(end.returnType(), iter.id());

  lookup = NameLookup::member("get", iter);
  ASSERT_EQ(lookup.functions().size(), 1);

  Function get = lookup.functions().front();
  ASSERT_EQ(get.returnType().baseType(), Type::Int);
}

