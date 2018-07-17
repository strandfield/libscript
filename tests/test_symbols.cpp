// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include <gtest/gtest.h>

#include "script/engine.h"
#include "script/symbol.h"


/****************************************************************
Testing function template creation
****************************************************************/

#include "script/template.h"
#include "script/templatebuilder.h"

TEST(Symbols, function_template_create) {
  using namespace script;

  Engine e;
  e.setup();

  Symbol s{ e.rootNamespace() };

  const auto nb_templates = e.rootNamespace().templates().size();

  // We cannot use get() here because FunctionTemplate has not been defined yet
  s.FunctionTemplate("foo").params(TemplateParameter{ TemplateParameter::TypeParameter{}, "T" })
    .setScope(e.rootNamespace()).create();

  ASSERT_EQ(e.rootNamespace().templates().size(), nb_templates + 1);
}

#include "script/functiontemplate.h"

TEST(Symbols, function_template_get) {
  using namespace script;

  Engine e;
  e.setup();

  Symbol s{ e.rootNamespace() };

  FunctionTemplate foo = s.FunctionTemplate("foo").params(
    TemplateParameter{ TemplateParameter::TypeParameter{}, "T" },
    TemplateParameter{ TemplateParameter::TypeParameter{}, "U" })
    .setScope(e.rootNamespace()).get();

  ASSERT_EQ(foo.name(), "foo");
  ASSERT_EQ(foo.enclosingSymbol().toNamespace(), e.rootNamespace());
  ASSERT_EQ(foo.parameters().size(), 2);
  ASSERT_EQ(foo.parameters().at(0).name(), "T");
  ASSERT_EQ(foo.parameters().at(1).name(), "U");
}


/****************************************************************
Testing class template creation
****************************************************************/

TEST(Symbols, class_template_create) {
  using namespace script;

  Engine e;
  e.setup();

  Symbol s{ e.rootNamespace() };

  const auto nb_templates = e.rootNamespace().templates().size();

  // We cannot use get() here because ClassTemplate has not been defined yet
  s.ClassTemplate("Bar").params(TemplateParameter{ TemplateParameter::TypeParameter{}, "T" })
    .setScope(e.rootNamespace()).create();

  ASSERT_EQ(e.rootNamespace().templates().size(), nb_templates + 1);
}

#include "script/classtemplate.h"

TEST(Symbols, class_template_get) {
  using namespace script;

  Engine e;
  e.setup();

  Symbol s{ e.rootNamespace() };

  ClassTemplate Bar = s.ClassTemplate("Bar").params(
    TemplateParameter{ TemplateParameter::TypeParameter{}, "T" },
    TemplateParameter{ TemplateParameter::TypeParameter{}, "U" })
    .setScope(e.rootNamespace()).get();

  ASSERT_EQ(Bar.name(), "Bar");
  ASSERT_EQ(Bar.enclosingSymbol().toNamespace(), e.rootNamespace());
  ASSERT_EQ(Bar.parameters().size(), 2);
  ASSERT_EQ(Bar.parameters().at(0).name(), "T");
  ASSERT_EQ(Bar.parameters().at(1).name(), "U");
}