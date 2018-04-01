// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include <gtest/gtest.h>

#include "script/engine.h"
#include "script/functionbuilder.h"

#include "script/compiler/compiler.h"
#include "script/compiler/functioncompiler.h"
#include "script/compiler/scriptcompiler.h"

#include "script/userdata.h"
#include "script/namelookup.h"
#include "script/overloadresolution.h"
#include "script/interpreter/executioncontext.h"

struct MaxData : public script::UserData
{
  script::Function less;
  std::vector<script::Type> types;
  std::vector<script::ConversionSequence> conversions;

  MaxData(const script::Function & f, const std::vector<script::ConversionSequence> & convs) :
    less(f)
    , conversions(convs)
  {
    types = less.prototype().arguments();
  }
  ~MaxData() = default;
};

script::Value max_function(script::FunctionCall *c)
{
  script::Engine *e = c->engine();
  script::Function callee = c->callee();
  auto data = std::dynamic_pointer_cast<MaxData>(callee.data());

  script::Value max = c->arg(0);
  std::vector<script::Value> args;
  args.reserve(2);
  for (int i(1); i < c->args().size(); ++i)
  {
    args.push_back(max);
    args.push_back(c->arg(i));
    e->applyConversions(args, data->types, data->conversions);
    script::Value is_less = e->invoke(data->less, args);
    if (is_less.toBool())
      max = c->arg(i);
    e->destroy(is_less);
    args.clear();
  }

  return e->copy(max);
}

/// TODO : we need to have access to the Engine
/// also it could be nice to be able to set an error to explain why deduction failed
/// or this could be done in another function (e.g. NativeTemplateDeductionDiagnosticFunction)
bool max_function_template_deduce(std::vector<script::TemplateArgument> & result, const std::vector<script::Type> & args)
{
  if (result.size() > 1)
    return false;

  if (result.size() == 1)
    return true;

  if (args.empty())
    return false;

  for (size_t i(0); i < args.size(); ++i)
    result.push_back(script::TemplateArgument::make(args.front().baseType()));
  return true;
}

script::Function max_function_template_instantiation(script::FunctionTemplate t, const std::vector<script::TemplateArgument> & args)
{
  script::Engine *e = t.engine();

  auto function = script::FunctionBuilder::Function("max", script::Prototype{}, max_function)
    .setReturnType(args.front().type);

  for (const auto & a : args)
    function.addParam(script::Type::cref(a.type));

  script::NameLookup lookup = script::NameLookup::resolve(script::Operator::LessOperator, e->rootNamespace());
  script::OverloadResolution resol = script::OverloadResolution::New(e);
  const script::Type param_type = function.proto.argv(0);
  if (!resol.process(lookup.functions(), { param_type , param_type }))
    throw std::runtime_error{ "Cannot instantiate max function template (no operator< was found)" };

  script::Function less = resol.selectedOverload();

  function.setData(std::make_shared<MaxData>(less, resol.conversionSequence()));

  return e->newFunction(function);
}

TEST(TemplateTests, template1) {
  using namespace script;

  Engine engine;
  engine.setup();

  auto tmplt = engine.newFunctionTemplate("max", max_function_template_deduce, max_function_template_instantiation);
  
  std::vector<TemplateArgument> targs;
  std::vector<Type> types{ Type::Int, Type::Int };
  
  ASSERT_TRUE(tmplt.deduce(targs, types));

  Function f = tmplt.getInstance(targs);

  Value a = engine.newInt(1);
  Value b = engine.newInt(2);

  Value c = engine.invoke(f, { a, b });

  ASSERT_EQ(c.type(), Type::Int);
  ASSERT_EQ(c.toInt(), 2);

  engine.destroy(c);
  engine.destroy(b);
  engine.destroy(a);


  targs.clear();
  types[1] = Type::Float;
  ASSERT_TRUE(tmplt.deduce(targs, types));
  f = tmplt.getInstance(targs);
  ASSERT_EQ(f.prototype().argv(1), Type::cref(Type::Int));

  a = engine.newInt(1);
  b = engine.newFloat(3.14f);
  c = engine.call(f, { a, b });
  ASSERT_EQ(c.type(), Type::Int);
  ASSERT_EQ(c.toInt(), 3);
}


TEST(TemplateTests, compilation1) {
  using namespace script;

  const char *source =
    " max(1, 2, 3); ";

  Engine engine;
  engine.setup();

  engine.rootNamespace().addTemplate(engine.newFunctionTemplate("max", max_function_template_deduce, max_function_template_instantiation));

  Script s = engine.newScript(SourceFile::fromString(source));
  bool success = s.compile();
  const auto & errors = s.messages();
  ASSERT_TRUE(success);
}
