// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include <gtest/gtest.h>

#include "script/array.h"
#include "script/engine.h"
#include "script/functionbuilder.h"
#include "script/functiontemplate.h"
#include "script/functiontype.h"

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

/// TODO : it could be nice to be able to set an error to explain why deduction failed
/// or this could be done in another function (e.g. NativeTemplateDeductionDiagnosticFunction)
bool max_function_template_deduce(const script::Template & max, std::vector<script::TemplateArgument> & result, const std::vector<script::Type> & args)
{
  using namespace script;

  if (result.size() > 2) // too many argument provided
    return false;

  if (result.size() > 0) 
  {
    // checking first argument
    if (result.front().kind != TemplateArgument::TypeArgument)
      return false;
  }
  else 
  {
    if (args.size() == 0)
      return false;

    result.push_back(script::TemplateArgument{ args.front().baseType() });
  }

  if (result.size() > 1)
  {
    // checking second argument
    if (result.at(1).kind != TemplateArgument::IntegerArgument)
      return false;
  }
  else
  {
    result.push_back(script::TemplateArgument{ int(args.size()) });
  }

  return true;
}

script::Function max_function_template_substitution(script::FunctionTemplate t, const std::vector<script::TemplateArgument> & args)
{
  using namespace script;

  Engine *e = t.engine();

  Type T = t.get("T", args).type;
  int N = t.get("N", args).integer;

  auto function = FunctionBuilder::Function("max", Prototype{}, max_function)
    .setReturnType(T);

  for (size_t i(0); i < size_t(N); ++i)
    function.addParam(Type::cref(T));

  return t.build(function, args);
}

script::Function max_function_template_instantiation(script::FunctionTemplate t, script::Function f)
{
  using namespace script;

  Engine *e = t.engine();

  const std::vector<TemplateArgument> & targs = f.arguments();

  NameLookup lookup = NameLookup::resolve(Operator::LessOperator, e->rootNamespace());
  OverloadResolution resol = OverloadResolution::New(e);
  const Type param_type = f.parameter(0);
  if (!resol.process(lookup.functions(), { param_type , param_type }))
    throw std::runtime_error{ "Cannot instantiate max function template (no operator< was found)" };

  Function less = resol.selectedOverload();

  FunctionTemplate::setInstanceData(f, std::make_shared<MaxData>(less, resol.conversionSequence()));

  return f;
}

TEST(TemplateTests, template1) {
  using namespace script;

  Engine engine;
  engine.setup();

  std::vector<TemplateParameter> params{
    TemplateParameter{ TemplateParameter::TypeParameter{}, "T" },
    TemplateParameter{ Type::Int, "N" },
  };

  auto tmplt = engine.newFunctionTemplate("max", std::move(params), max_function_template_deduce, max_function_template_substitution, max_function_template_instantiation);
  
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

  std::vector<TemplateParameter> params{
    TemplateParameter{ TemplateParameter::TypeParameter{}, "T" },
    TemplateParameter{ Type::Int, "N" },
  };

  engine.rootNamespace().addTemplate(engine.newFunctionTemplate("max", std::move(params), max_function_template_deduce, max_function_template_substitution, max_function_template_instantiation));

  Script s = engine.newScript(SourceFile::fromString(source));
  bool success = s.compile();
  const auto & errors = s.messages();
  ASSERT_TRUE(success);
}


#include "script/parser/parser.h"
#include "script/private/templateargumentdeduction.h"

TEST(TemplateTests, argument_deduction_1) {
  using namespace script;

  Engine engine;
  engine.setup();

  const char *source =
    "  template<typename T>           "
    "  T abs(const T & a) { }    ";

  parser::Parser parser{ script::SourceFile::fromString(source) };
  auto template_declaration = std::static_pointer_cast<ast::TemplateDeclaration>(parser.parseStatement());

  std::vector<TemplateParameter> params{
    TemplateParameter{ TemplateParameter::TypeParameter{}, "T" },
  };

  FunctionTemplate function_template = engine.newFunctionTemplate("abs", std::move(params), nullptr, nullptr, nullptr);

  std::vector<TemplateArgument> arguments;
  std::vector<Type> types{ Type::Int };
  Scope scp{ engine.rootNamespace() };

  TemplateArgumentDeduction deduction = TemplateArgumentDeduction::process(function_template, arguments, types, scp, template_declaration);

  ASSERT_TRUE(deduction.success());
  ASSERT_EQ(deduction.get_deductions().size(), 1);
  ASSERT_EQ(deduction.deduced_value(0).type, Type::Int);

  //std::cout << "Deduced " << deduction.deduction_name(0) << " = " << engine.typeName(deduction.deduced_value(0).type) << std::endl;
  
  arguments = std::vector<TemplateArgument>{ TemplateArgument{ Type::Int } };
  types = std::vector<Type>{ Type::Float };
  deduction = TemplateArgumentDeduction::process(function_template, arguments, types, scp, template_declaration);

  ASSERT_TRUE(deduction.success());
  ASSERT_EQ(deduction.get_deductions().size(), 0);
}

TEST(TemplateTests, argument_deduction_2) {
  using namespace script;

  Engine engine;
  engine.setup();

  const char *source =
    "  template<typename T>           "
    "  void swap(T & a, T & b) { }    ";

  parser::Parser parser{ script::SourceFile::fromString(source) };
  auto template_declaration = std::static_pointer_cast<ast::TemplateDeclaration>(parser.parseStatement());
  
  std::vector<TemplateParameter> params{
    TemplateParameter{ TemplateParameter::TypeParameter{}, "T" },
  };

  FunctionTemplate function_template = engine.newFunctionTemplate("swap", std::move(params), nullptr, nullptr, nullptr);

  std::vector<TemplateArgument> arguments;
  std::vector<Type> types{ Type::Int, Type::Int };
  Scope scp{ engine.rootNamespace() };

  TemplateArgumentDeduction deduction = TemplateArgumentDeduction::process(function_template, arguments, types, scp, template_declaration);

  ASSERT_TRUE(deduction.success());
  ASSERT_EQ(deduction.get_deductions().size(), 1);
  ASSERT_EQ(deduction.deduced_value(0).type, Type::Int);

  //std::cout << "Deduced " << deduction.deduction_name(0) << " = " << engine.typeName(deduction.deduced_value(0).type) << std::endl;

  types = std::vector<Type>{ Type::Int, Type::Float };
  deduction = TemplateArgumentDeduction::process(function_template, arguments, types, scp, template_declaration);

  ASSERT_TRUE(deduction.failure());
  ASSERT_EQ(deduction.get_deductions().size(), 2);
}

TEST(TemplateTests, argument_deduction_3) {
  using namespace script;

  Engine engine;
  engine.setup();

  const char *source =
    "  template<typename T>           "
    "  T max(const Array<T> & a) { }    ";

  parser::Parser parser{ script::SourceFile::fromString(source) };
  auto template_declaration = std::static_pointer_cast<ast::TemplateDeclaration>(parser.parseStatement());

  std::vector<TemplateParameter> params{
    TemplateParameter{ TemplateParameter::TypeParameter{}, "T" },
  };

  FunctionTemplate function_template = engine.newFunctionTemplate("max", std::move(params), nullptr, nullptr, nullptr);

  std::vector<TemplateArgument> arguments;
  Type array_int = engine.newArray(Engine::ElementType{ Type::Int }).typeId();
  std::vector<Type> types{ array_int };
  Scope scp{ engine.rootNamespace() };

  TemplateArgumentDeduction deduction = TemplateArgumentDeduction::process(function_template, arguments, types, scp, template_declaration);

  ASSERT_TRUE(deduction.success());
  ASSERT_EQ(deduction.get_deductions().size(), 1);
  ASSERT_EQ(deduction.deduced_value(0).type, Type::Int);

  // std::cout << "Deduced " << deduction.deduction_name(0) << " = " << engine.typeName(deduction.deduced_value(0).type) << std::endl;

  types = std::vector<Type>{ Type::Int };
  deduction = TemplateArgumentDeduction::process(function_template, arguments, types, scp, template_declaration);

  ASSERT_TRUE(deduction.success());
  ASSERT_EQ(deduction.get_deductions().size(), 0);
}

TEST(TemplateTests, argument_deduction_4) {
  using namespace script;

  Engine engine;
  engine.setup();

  const char *source =
    "  template<typename R, typename A>           "
    "  R apply(R(A) func, const A & arg) { }    ";

  parser::Parser parser{ script::SourceFile::fromString(source) };
  auto template_declaration = std::static_pointer_cast<ast::TemplateDeclaration>(parser.parseStatement());

  std::vector<TemplateParameter> params{
    TemplateParameter{ TemplateParameter::TypeParameter{}, "R" },
    TemplateParameter{ TemplateParameter::TypeParameter{}, "A" },
  };

  FunctionTemplate function_template = engine.newFunctionTemplate("apply", std::move(params), nullptr, nullptr, nullptr);

  Prototype proto{ Type::Boolean, Type::Int };
  FunctionType function_type = engine.getFunctionType(proto);

  std::vector<TemplateArgument> arguments;
  std::vector<Type> types{ function_type.type(), Type::Int };
  Scope scp{ engine.rootNamespace() };

  TemplateArgumentDeduction deduction = TemplateArgumentDeduction::process(function_template, arguments, types, scp, template_declaration);

  ASSERT_TRUE(deduction.success());
  ASSERT_EQ(deduction.get_deductions().size(), 2);
  ASSERT_EQ(deduction.deduced_value(0).type, Type::Boolean);
  ASSERT_EQ(deduction.deduced_value(1).type, Type::Int);

  //std::cout << "Deduced " << deduction.deduction_name(0) << " = " << engine.typeName(deduction.deduced_value(0).type) << std::endl;
  //std::cout << "Deduced " << deduction.deduction_name(1) << " = " << engine.typeName(deduction.deduced_value(1).type) << std::endl;
}