// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include <gtest/gtest.h>

#include "script/cast.h"
#include "script/class.h"
#include "script/classbuilder.h"
#include "script/conversions.h"
#include "script/diagnosticmessage.h"
#include "script/engine.h"
#include "script/namespace.h"
#include "script/operator.h"
#include "script/overloadresolution.h"

#include "script/functionbuilder.h"

TEST(OverloadResolution, test1) {
  using namespace script;

  Engine e;
  e.setup();

  std::vector<Function> overloads;


  overloads.push_back(Symbol{ e.rootNamespace() }.Function("foo").create());

  OverloadResolution resol = OverloadResolution::New(&e);
  ASSERT_FALSE(resol.process(overloads, std::vector<Type>{ Type::Int }));

  overloads.push_back(Symbol{ e.rootNamespace() }.Function("foo").params(Type::Int).create());

  resol = OverloadResolution::New(&e);
  ASSERT_TRUE(resol.process(overloads, std::vector<Type>{ Type::Int }));
  ASSERT_EQ(resol.selectedOverload(), overloads.at(1));
  auto conversions = resol.conversionSequence();
  ASSERT_TRUE(conversions.at(0).conv1.isCopyInitialization());

  overloads.push_back(Symbol{ e.rootNamespace() }.Function("foo").params(Type::Char).create());
  resol = OverloadResolution::New(&e);
  bool result = resol.process(overloads, std::vector<Type>{ Type::Float });
  ASSERT_FALSE(result);
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
  OverloadResolution resol = OverloadResolution::New(&e);
  ASSERT_TRUE(resol.process(overloads, std::vector<Type>{Type::Int, Type::Float}));
  auto selected = resol.selectedOverload();
  ASSERT_TRUE(selected.prototype().at(0).baseType() == Type::Float);
  ASSERT_TRUE(selected.prototype().at(1).baseType() == Type::Float);
}

TEST(OverloadResolution, failure_indistinguishable) {
  using namespace script;

  Engine e;
  e.setup();

  std::vector<Function> overloads;

  overloads.push_back(Symbol{ e.rootNamespace() }.Function("foo").create());

  overloads.push_back(Symbol{ e.rootNamespace() }.Function("foo").returns(Type::Int).create());

  auto resol = OverloadResolution::New(&e);
  std::vector<Type> types{ };
  resol.process(overloads, types);
  ASSERT_FALSE(resol.success());

  diagnostic::Message mssg = resol.emitDiagnostic();
  //std::cout << mssg.to_string() << std::endl;
  ASSERT_TRUE(mssg.message().find("indistinguishable") != std::string::npos);
}

TEST(OverloadResolution, failure_no_viable_candidates) {
  using namespace script;

  Engine e;
  e.setup();

  std::vector<Function> overloads;

  overloads.push_back(Symbol{ e.rootNamespace() }.Function("foo").params(Type::Int).create());

  overloads.push_back(Symbol{ e.rootNamespace() }.Function("foo").returns(Type::Int).params(Type::Float).create());

  Class A = Symbol{ e.rootNamespace() }.Class("A").get();
  overloads.push_back(Symbol{ e.rootNamespace() }.Function("foo").returns(Type::Int).params(Type::Boolean, A.id()).create());

  auto resol = OverloadResolution::New(&e);
  std::vector<Type> types{Type::Int, Type::Float};
  resol.process(overloads, types);
  ASSERT_FALSE(resol.success());

  ASSERT_EQ(resol.arguments().kind(), OverloadResolution::Arguments::TypeArguments);
  ASSERT_EQ(resol.arguments().size(), 2);
  ASSERT_EQ(resol.arguments().types().back(), Type::Float);
 
  diagnostic::Message mssg = resol.emitDiagnostic();
  //std::cout << mssg.to_string() << std::endl;
  ASSERT_TRUE(mssg.message().find("expects 1 but 2 were provided") != std::string::npos);
  ASSERT_TRUE(mssg.message().find("Could not convert argument 2") != std::string::npos);

}
