// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include <gtest/gtest.h>

#include "script/array.h"
#include "script/engine.h"
#include "script/functionbuilder.h"
#include "script/functiontemplate.h"
#include "script/functiontype.h"
#include "script/templateargumentdeduction.h"

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
void max_function_template_deduce(script::TemplateArgumentDeduction & result, const script::FunctionTemplate & max, const std::vector<script::TemplateArgument> & args, const std::vector<script::Type> & types)
{
  using namespace script;

  if (args.size() > 2) // too many argument provided
    return result.fail();

  if (args.size() > 0) 
  {
    // checking first argument
    if (args.front().kind != TemplateArgument::TypeArgument)
      return result.fail();

  }
  else 
  {
    if (types.size() == 0)
      return result.fail();

    result.record_deduction(0, TemplateArgument{ types.front().baseType() });
  }

  if (args.size() > 1)
  {
    // checking second argument
    if (args.at(1).kind != TemplateArgument::IntegerArgument)
      return result.fail();
  }
  else
  {
    result.record_deduction(1, TemplateArgument{ int(types.size()) });
  }

  return result.set_success(true);
}

void max_function_template_substitution(script::FunctionBuilder & result, script::FunctionTemplate t, const std::vector<script::TemplateArgument> & args)
{
  using namespace script;

  Engine *e = t.engine();

  Type T = t.get("T", args).type;
  int N = t.get("N", args).integer;

  result.setReturnType(T);

  for (size_t i(0); i < size_t(N); ++i)
    result.addParam(Type::cref(T));
}

std::pair<script::NativeFunctionSignature, std::shared_ptr<script::UserData>> max_function_template_instantiation(script::FunctionTemplate t, script::Function f)
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

  return std::make_pair(max_function, std::make_shared<MaxData>(less, resol.conversionSequence()));
}

TEST(TemplateTests, call_with_no_args) {
  using namespace script;

  const char *source =
    " max(1, 2, 3); ";

  Engine engine;
  engine.setup();

  std::vector<TemplateParameter> params{
    TemplateParameter{ TemplateParameter::TypeParameter{}, "T" },
    TemplateParameter{ Type::Int, "N" },
  };

  engine.rootNamespace().addTemplate(engine.newFunctionTemplate("max", std::move(params), Scope{}, max_function_template_deduce, max_function_template_substitution, max_function_template_instantiation));

  Script s = engine.newScript(SourceFile::fromString(source));
  bool success = s.compile();
  const auto & errors = s.messages();
  ASSERT_TRUE(success);
}


TEST(TemplateTests, call_to_template_with_no_args) {
  using namespace script;

  const char *source =
    " max<>(1, 2, 3); ";

  Engine engine;
  engine.setup();

  std::vector<TemplateParameter> params{
    TemplateParameter{ TemplateParameter::TypeParameter{}, "T" },
    TemplateParameter{ Type::Int, "N" },
  };

  engine.rootNamespace().addTemplate(engine.newFunctionTemplate("max", std::move(params), Scope{}, max_function_template_deduce, max_function_template_substitution, max_function_template_instantiation));

  Script s = engine.newScript(SourceFile::fromString(source));
  bool success = s.compile();
  const auto & errors = s.messages();
  ASSERT_TRUE(success);
}

TEST(TemplateTests, call_to_template_with_one_arg) {
  using namespace script;

  const char *source =
    " max<int>(1, 2, 3); ";

  Engine engine;
  engine.setup();

  std::vector<TemplateParameter> params{
    TemplateParameter{ TemplateParameter::TypeParameter{}, "T" },
    TemplateParameter{ Type::Int, "N" },
  };

  engine.rootNamespace().addTemplate(engine.newFunctionTemplate("max", std::move(params), Scope{}, max_function_template_deduce, max_function_template_substitution, max_function_template_instantiation));

  Script s = engine.newScript(SourceFile::fromString(source));
  bool success = s.compile();
  const auto & errors = s.messages();
  ASSERT_TRUE(success);
}

TEST(TemplateTests, call_to_template_with_all_args) {
  using namespace script;

  const char *source =
    " max<int, 3>(1, 2, 3); ";

  Engine engine;
  engine.setup();

  std::vector<TemplateParameter> params{
    TemplateParameter{ TemplateParameter::TypeParameter{}, "T" },
    TemplateParameter{ Type::Int, "N" },
  };

  engine.rootNamespace().addTemplate(engine.newFunctionTemplate("max", std::move(params), Scope{}, max_function_template_deduce, max_function_template_substitution, max_function_template_instantiation));

  Script s = engine.newScript(SourceFile::fromString(source));
  bool success = s.compile();
  const auto & errors = s.messages();
  ASSERT_TRUE(success);
}

TEST(TemplateTests, invalid_call_to_template_with_all_args) {
  using namespace script;

  const char *source =
    " max<int, 4>(1, 2, 3); ";

  Engine engine;
  engine.setup();

  std::vector<TemplateParameter> params{
    TemplateParameter{ TemplateParameter::TypeParameter{}, "T" },
    TemplateParameter{ Type::Int, "N" },
  };

  engine.rootNamespace().addTemplate(engine.newFunctionTemplate("max", std::move(params), Scope{}, max_function_template_deduce, max_function_template_substitution, max_function_template_instantiation));

  Script s = engine.newScript(SourceFile::fromString(source));
  bool success = s.compile();
  const auto & errors = s.messages();
  ASSERT_FALSE(success);
}

#include "script/parser/parser.h"
#include "script/templateargumentdeduction.h"

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

  FunctionTemplate function_template = engine.newFunctionTemplate("abs", std::move(params), Scope{}, nullptr, nullptr, nullptr);

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

  FunctionTemplate function_template = engine.newFunctionTemplate("swap", std::move(params), Scope{}, nullptr, nullptr, nullptr);

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

  FunctionTemplate function_template = engine.newFunctionTemplate("max", std::move(params), Scope{}, nullptr, nullptr, nullptr);

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

  FunctionTemplate function_template = engine.newFunctionTemplate("apply", std::move(params), Scope{}, nullptr, nullptr, nullptr);

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


/****************************************************************
  Testing user-defined function templates
****************************************************************/

TEST(TemplateTests, user_defined_function_template_1) {
  using namespace script;

  const char *source =
    "  template<typename T>     "
    "  T abs(const T & a)       "
    "  {                        "
    "    if(a < 0) return -a;   "
    "    return a;              "
    "  }                        ";

  Engine engine;
  engine.setup();

  Script s = engine.newScript(SourceFile::fromString(source));
  bool success = s.compile();
  const auto & errors = s.messages();
  ASSERT_TRUE(success);

  ASSERT_EQ(s.rootNamespace().templates().size(), 1);

  FunctionTemplate ft = s.rootNamespace().templates().front().asFunctionTemplate();
  ASSERT_EQ(ft.name(), "abs");
  Function f = ft.getInstance(std::vector<TemplateArgument>{TemplateArgument{ Type::Int }});

  ASSERT_TRUE(f.isTemplateInstance());
  ASSERT_EQ(f.instanceOf(), ft);
  ASSERT_EQ(f.arguments().size(), 1);
  ASSERT_EQ(f.arguments().front().type, Type::Int);
}

TEST(TemplateTests, user_defined_function_template_2) {
  using namespace script;

  const char *source =
    "  template<typename T>     "
    "  T abs(const T & a)       "
    "  {                        "
    "    if(a < 0) return -a;   "
    "    return a;              "
    "  }                        "
    "  int a = abs(-1);         ";

  Engine engine;
  engine.setup();

  Script s = engine.newScript(SourceFile::fromString(source));
  bool success = s.compile();
  const auto & errors = s.messages();
  ASSERT_TRUE(success);

  s.run();

  ASSERT_EQ(s.globals().size(), 1);
  Value a = s.globals().front();
  ASSERT_EQ(a.type(), Type::Int);
  ASSERT_EQ(a.toInt(), 1);
}

TEST(TemplateTests, user_defined_function_template_3) {
  using namespace script;

  const char *source =
    "  template<typename T>               "
    "  T max(const T & a, const T & b)    "
    "  {                                  "
    "    return a > b ? a : b;            "
    "  }                                  "
    "  int n = max(2, 3);                 ";

  Engine engine;
  engine.setup();

  Script s = engine.newScript(SourceFile::fromString(source));
  bool success = s.compile();
  const auto & errors = s.messages();
  ASSERT_TRUE(success);

  s.run();

  ASSERT_EQ(s.globals().size(), 1);
  Value n = s.globals().front();
  ASSERT_EQ(n.type(), Type::Int);
  ASSERT_EQ(n.toInt(), 3);
}

TEST(TemplateTests, user_defined_function_template_failure_1) {
  using namespace script;

  const char *source =
    "  template<typename T>               "
    "  T max(const T & a, const T & b)    "
    "  {                                  "
    "    return a > b ? a : b;            "
    "  }                                  "
    "  int n = max(2, 3.14);              ";

  Engine engine;
  engine.setup();

  Script s = engine.newScript(SourceFile::fromString(source));
  bool success = s.compile();
  const auto & errors = s.messages();
  ASSERT_FALSE(success);
}

TEST(TemplateTests, user_defined_function_template_called_with_args) {
  using namespace script;

  const char *source =
    "  template<typename T>               "
    "  T max(const T & a, const T & b)    "
    "  {                                  "
    "    return a > b ? a : b;            "
    "  }                                  "
    "  int n = max<int>(2, 3.14);         ";

  Engine engine;
  engine.setup();

  Script s = engine.newScript(SourceFile::fromString(source));
  bool success = s.compile();
  const auto & errors = s.messages();
  ASSERT_TRUE(success);

  s.run();

  ASSERT_EQ(s.globals().size(), 1);
  Value n = s.globals().front();
  ASSERT_EQ(n.type(), Type::Int);
  ASSERT_EQ(n.toInt(), 3);
}


/****************************************************************
Testing user-defined class templates
****************************************************************/

#include "script/classtemplate.h"

TEST(TemplateTests, user_defined_class_template_definition) {
  using namespace script;

  const char *source =
    "  template<typename First, typename Second>   "
    "  class Pair                                  "
    "  {                                           "
    "  public:                                     "
    "    First first;                              "
    "    Second second;                            "
    "                                              "
    "    Pair() = default;                         "
    "    ~Pair() = default;                        "
    "  };                                          ";

  Engine engine;
  engine.setup();

  Script s = engine.newScript(SourceFile::fromString(source));
  bool success = s.compile();
  const auto & errors = s.messages();
  ASSERT_TRUE(success);

  ASSERT_EQ(s.rootNamespace().templates().size(), 1);

  ClassTemplate pair = s.rootNamespace().templates().front().asClassTemplate();
  ASSERT_EQ(pair.name(), "Pair");
  ASSERT_FALSE(pair.is_native());

  const auto & params = pair.parameters();
  ASSERT_EQ(params.size(), 2);
  ASSERT_EQ(params.front().name(), "First");
  ASSERT_EQ(params.back().name(), "Second");
  ASSERT_EQ(params.front().kind(), TemplateParameter::TypeTemplateParameter);
  ASSERT_EQ(params.back().kind(), TemplateParameter::TypeTemplateParameter);

  ASSERT_TRUE(pair.instances().empty());
}

TEST(TemplateTests, user_defined_class_template_instantiation) {
  using namespace script;

  const char *source =
    "  template<typename First, typename Second>   "
    "  class Pair                                  "
    "  {                                           "
    "  public:                                     "
    "    First first;                              "
    "    Second second;                            "
    "                                              "
    "    Pair() = default;                         "
    "    ~Pair() = default;                        "
    "  };                                          "
    "  Pair<int, float> p;                         "
    "  p.first = 42;                               "
    "  int n = p.first;                            ";

  Engine engine;
  engine.setup();

  Script s = engine.newScript(SourceFile::fromString(source));
  bool success = s.compile();
  const auto & errors = s.messages();
  ASSERT_TRUE(success);

  s.run();

  ASSERT_EQ(s.globals().size(), 2);
  Value n = s.globals().back();
  ASSERT_EQ(n.type(), Type::Int);
  ASSERT_EQ(n.toInt(), 42);

  ClassTemplate pair = s.rootNamespace().templates().front().asClassTemplate();
  ASSERT_EQ(pair.instances().size(), 1);
  Class pair_int_float = pair.instances().begin()->second;
  ASSERT_EQ(pair_int_float.instanceOf(), pair);
  ASSERT_EQ(pair_int_float.arguments().size(), 2);
  ASSERT_EQ(pair_int_float.arguments().front().type, Type::Int);
  ASSERT_EQ(pair_int_float.arguments().back().type, Type::Float);
}

/****************************************************************
Testing class with member function template
****************************************************************/

TEST(TemplateTests, class_with_member_template) {
  using namespace script;

  const char *source =
    "  class Foo                           "
    "  {                                   "
    "  public:                             "
    "    int n;                            "
    "                                      "
    "    template<int N>                   "
    "    int bar() { return n + N; }       "
    "  };                                  ";

  Engine engine;
  engine.setup();

  Script s = engine.newScript(SourceFile::fromString(source));
  bool success = s.compile();
  const auto & errors = s.messages();
  ASSERT_TRUE(success);

  Class Foo = s.classes().front();
  ASSERT_EQ(Foo.name(), "Foo");

  ASSERT_EQ(Foo.templates().size(), 1);

  FunctionTemplate bar = Foo.templates().front().asFunctionTemplate();
  ASSERT_EQ(bar.name(), "bar");
  ASSERT_EQ(bar.parameters().size(), 1);
  ASSERT_EQ(bar.parameters().front().type(), Type::Int);
  ASSERT_EQ(bar.parameters().front().name(), "N");
}


TEST(TemplateTests, instantiating_class_member_template) {
  using namespace script;

  const char *source =
    "  class Foo                           "
    "  {                                   "
    "  public:                             "
    "    int n;                            "
    "                                      "
    "    Foo() = default;                  "
    "    ~Foo() = default;                 "
    "                                      "
    "    template<int N>                   "
    "    int bar() { return n + N; }       "
    "  };                                  "
    "  Foo f;  f.n = 0;                    "
    "  int n = f.bar<42>();                ";

  Engine engine;
  engine.setup();

  Script s = engine.newScript(SourceFile::fromString(source));
  bool success = s.compile();
  const auto & errors = s.messages();
  ASSERT_TRUE(success);

  Class Foo = s.classes().front();
  FunctionTemplate bar = Foo.templates().front().asFunctionTemplate();
  ASSERT_EQ(bar.instances().size(), 1);
  Function bar_42 = bar.instances().begin()->second;
  ASSERT_EQ(bar_42.instanceOf(), bar);
  ASSERT_EQ(bar_42.arguments().size(), 1);
  ASSERT_EQ(bar_42.arguments().front().integer, 42);
  ASSERT_TRUE(bar_42.isMemberFunction());
  ASSERT_EQ(bar_42.memberOf(), Foo);

  s.run();

  ASSERT_EQ(s.globals().size(), 2);
  Value n = s.globals().back();
  ASSERT_EQ(n.type(), Type::Int);
  ASSERT_EQ(n.toInt(), 42);
}
