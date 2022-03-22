// Copyright (C) 2018-2022 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include <gtest/gtest.h>

#include "script/cast.h"
#include "script/class.h"
#include "script/classbuilder.h"
#include "script/conversions.h"
#include "script/diagnosticmessage.h"
#include "script/engine.h"
#include "script/initialization.h"
#include "script/namespace.h"
#include "script/operator.h"
#include "script/overloadresolution.h"

#include "script/functionbuilder.h"

TEST(OverloadResolution, test1) {
  using namespace script;

  Engine e;
  e.setup();

  std::vector<Function> overloads;

  overloads.push_back(FunctionBuilder::Fun(e.rootNamespace(), "foo").get());

  OverloadResolution::Candidate resol = resolve_overloads(overloads, std::vector<Type>{ Type::Int });
  ASSERT_FALSE(bool(resol));

  overloads.push_back(FunctionBuilder::Fun(e.rootNamespace(), "foo").params(Type::Int).get());

  resol = resolve_overloads(overloads, std::vector<Type>{ Type::Int });
  ASSERT_TRUE(bool(resol));
  ASSERT_EQ(resol.function, overloads.at(1));
  auto inits = resol.initializations;
  ASSERT_TRUE(inits.at(0).conversion().firstStandardConversion().isCopy());

  overloads.push_back(FunctionBuilder::Fun(e.rootNamespace(), "foo").params(Type::Char).get());
  resol = resolve_overloads(overloads, std::vector<Type>{ Type::Float });
  ASSERT_FALSE(bool(resol));
}


TEST(OverloadResolution, builtin_operators) {
  using namespace script;

  Engine e;
  e.setup();

  const std::vector<Operator> & operators = e.rootNamespace().operators();
  std::vector<Function> overloads;
  for (const auto & op : operators)
  {
    if (op.operatorId() == AdditionOperator)
      overloads.push_back(op);
  }
  OverloadResolution::Candidate resol = resolve_overloads(overloads, std::vector<Type>{Type::Int, Type::Float});
  ASSERT_TRUE(bool(resol));
  auto selected = resol.function;
  ASSERT_TRUE(selected.prototype().at(0).baseType() == Type::Float);
  ASSERT_TRUE(selected.prototype().at(1).baseType() == Type::Float);
}

TEST(OverloadResolution, failure_indistinguishable) {
  using namespace script;

  Engine e;
  e.setup();

  std::vector<Function> overloads;

  overloads.push_back(FunctionBuilder::Fun(e.rootNamespace(), "foo").get());

  overloads.push_back(FunctionBuilder::Fun(e.rootNamespace(), "foo").returns(Type::Int).get());

  std::vector<Type> types{ };
  OverloadResolution::Candidate resol = resolve_overloads(overloads, types);
  ASSERT_FALSE(bool(resol));

  // @TODO: test that OR fails because the overloads are indistinguishable
}

TEST(OverloadResolution, failure_no_viable_candidates) {
  using namespace script;

  Engine e;
  e.setup();

  std::vector<Function> overloads;

  overloads.push_back(FunctionBuilder::Fun(e.rootNamespace(), "foo").params(Type::Int).get());

  overloads.push_back(FunctionBuilder::Fun(e.rootNamespace(), "foo").returns(Type::Int).params(Type::Float).get());

  Class A = Symbol{ e.rootNamespace() }.newClass("A").get();
  overloads.push_back(FunctionBuilder::Fun(e.rootNamespace(), "foo").returns(Type::Int).params(Type::Boolean, A.id()).get());

  std::vector<Type> types{ Type::Int, Type::Float };
  OverloadResolution::Candidate resol = resolve_overloads(overloads, types);
  ASSERT_FALSE(bool(resol));

  // @TODO: test that OR fails because argument are missing or not convertible
}
