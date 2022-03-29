// Copyright (C) 2018-2021 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include <gtest/gtest.h>

#include "script/array.h"
#include "script/class.h"
#include "script/conversions.h"
#include "script/engine.h"
#include "script/functionbuilder.h"
#include "script/functiontemplate.h"
#include "script/functiontype.h"
#include "script/initialization.h"
#include "script/private/template_p.h"
#include "script/script.h"
#include "script/templateargumentdeduction.h"
#include "script/templatebuilder.h"
#include "script/typesystem.h"

#include "script/userdata.h"
#include "script/namelookup.h"
#include "script/overloadresolution.h"
#include "script/interpreter/executioncontext.h"

struct MaxData : public script::UserData
{
  script::Function less;
  std::vector<script::Conversion> conversions;

  MaxData(const script::Function & f, const std::vector<script::Initialization> & inits) 
    : less(f)
  {
    for (const auto & i : inits)
      conversions.push_back(i.conversion());
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
    script::Conversion::apply(data->conversions, args);
    script::Value is_less = data->less.invoke(args);
    if (is_less.toBool())
      max = c->arg(i);
    e->destroy(is_less);
    args.clear();
  }

  return e->copy(max);
}

/// TODO : it could be nice to be able to set an error to explain why deduction failed

class MaxFunctionTemplate : public script::FunctionTemplateNativeBackend
{
  void deduce(script::TemplateArgumentDeduction& deduction, const std::vector<script::TemplateArgument>& targs, const std::vector<script::Type>& itypes) override
  {
    using namespace script;

    if (targs.size() > 2) // too many argument provided
      return deduction.fail();

    if (targs.size() > 0)
    {
      // checking first argument
      if (targs.front().kind != TemplateArgument::TypeArgument)
        return deduction.fail();

    }
    else
    {
      if (itypes.size() == 0)
        return deduction.fail();

      deduction.record_deduction(0, TemplateArgument{ itypes.front().baseType() });
    }

    if (targs.size() > 1)
    {
      // checking second argument
      if (targs.at(1).kind != TemplateArgument::IntegerArgument)
        return deduction.fail();
    }
    else
    {
      deduction.record_deduction(1, TemplateArgument{ int(itypes.size()) });
    }

    return deduction.set_success(true);
  }

  void substitute(script::FunctionBlueprint& blueprint, const std::vector<script::TemplateArgument>& targs) override
  {
    using namespace script;

    Type T = functionTemplate().get("T", targs).type;
    int N = functionTemplate().get("N", targs).integer;

    blueprint.prototype_.setReturnType(T);

    for (size_t i(0); i < size_t(N); ++i)
      blueprint.prototype_.push(Type::cref(T));
  }

  std::pair<script::NativeFunctionSignature, std::shared_ptr<script::UserData>> instantiate(script::Function& function) override
  {
    using namespace script;

    Engine* e = functionTemplate().engine();

    NameLookup lookup = NameLookup::resolve(LessOperator, e->rootNamespace());
    const Type param_type = function.parameter(0);
    OverloadResolution::Candidate resol = resolve_overloads(lookup.functions(), std::vector<Type>{ param_type, param_type });
    if (!resol)
      throw std::runtime_error{ "Cannot instantiate max function template (no operator< was found)" };

    Function less = resol.function;

    return std::make_pair(max_function, std::make_shared<MaxData>(less, resol.initializations));
  }
};

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

  FunctionTemplateBuilder(Symbol(engine.rootNamespace()), "max")
    .setParams(std::move(params))
    .setScope(Scope{})
    .withBackend<MaxFunctionTemplate>()
    .create();

  Script s = engine.newScript(SourceFile::fromString(source));
  bool success = s.compile();
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

  FunctionTemplateBuilder(Symbol(engine.rootNamespace()), "max")
    .setParams(std::move(params))
    .setScope(Scope{})
    .withBackend<MaxFunctionTemplate>()
    .create();

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

  FunctionTemplateBuilder(Symbol(engine.rootNamespace()), "max")
    .setParams(std::move(params))
    .setScope(Scope{})
    .withBackend<MaxFunctionTemplate>()
    .create();

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

  FunctionTemplateBuilder(Symbol(engine.rootNamespace()), "max")
    .setParams(std::move(params))
    .setScope(Scope{})
    .withBackend<MaxFunctionTemplate>()
    .create();

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

  FunctionTemplateBuilder(Symbol(engine.rootNamespace()), "max")
    .setParams(std::move(params))
    .setScope(Scope{})
    .withBackend<MaxFunctionTemplate>()
    .create();

  Script s = engine.newScript(SourceFile::fromString(source));
  bool success = s.compile();
  const auto & errors = s.messages();
  ASSERT_FALSE(success);
}

#include "script/parser/parser.h"
#include "script/templateargumentdeduction.h"

class DummyFunctionTemplateBackend : public script::FunctionTemplateNativeBackend
{
  void deduce(script::TemplateArgumentDeduction& deduction, const std::vector<script::TemplateArgument>& targs, const std::vector<script::Type>& itypes) override
  {
    throw std::runtime_error{ "dummy" };
  }

  void substitute(script::FunctionBlueprint& blueprint, const std::vector<script::TemplateArgument> & targs) override
  {
    throw std::runtime_error{ "dummy" };
  }

  std::pair<script::NativeFunctionSignature, std::shared_ptr<script::UserData>> instantiate(script::Function & function) override
  {
    throw std::runtime_error{ "dummy" };
  }
};

TEST(TemplateTests, argument_deduction_1) {
  using namespace script;

  Engine engine;
  engine.setup();

  const char *source =
    "  template<typename T>           "
    "  T abs(const T & a) { }    ";

  parser::Parser parser{ source };
  auto template_declaration = std::static_pointer_cast<ast::TemplateDeclaration>(parser.parseStatement());

  std::vector<TemplateParameter> params{
    TemplateParameter{ TemplateParameter::TypeParameter{}, "T" },
  };

  FunctionTemplate function_template = FunctionTemplateBuilder(Symbol(engine.rootNamespace()), "abs")
    .setParams(std::move(params))
    .setScope(Scope{})
    .withBackend<DummyFunctionTemplateBackend>()
    .get();

  std::vector<TemplateArgument> arguments;
  std::vector<Type> types{ Type::Int };
  Scope scp{ engine.rootNamespace() };

  TemplateArgumentDeduction deduction = TemplateArgumentDeduction::process(function_template, arguments, types, template_declaration);

  ASSERT_TRUE(deduction.success());
  ASSERT_EQ(deduction.get_deductions().size(), 1);
  ASSERT_EQ(deduction.deduced_value(0).type, Type::Int);

  //std::cout << "Deduced " << deduction.deduction_name(0) << " = " << engine.typeName(deduction.deduced_value(0).type) << std::endl;
  
  arguments = std::vector<TemplateArgument>{ TemplateArgument{ Type::Int } };
  types = std::vector<Type>{ Type::Float };
  deduction = TemplateArgumentDeduction::process(function_template, arguments, types, template_declaration);

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

  parser::Parser parser{ source };
  auto template_declaration = std::static_pointer_cast<ast::TemplateDeclaration>(parser.parseStatement());
  
  std::vector<TemplateParameter> params{
    TemplateParameter{ TemplateParameter::TypeParameter{}, "T" },
  };

  FunctionTemplate function_template = FunctionTemplateBuilder(Symbol(engine.rootNamespace()), "swap")
    .setParams(std::move(params))
    .setScope(Scope{})
    .withBackend<DummyFunctionTemplateBackend>()
    .get();

  std::vector<TemplateArgument> arguments;
  std::vector<Type> types{ Type::Int, Type::Int };
  Scope scp{ engine.rootNamespace() };

  TemplateArgumentDeduction deduction = TemplateArgumentDeduction::process(function_template, arguments, types, template_declaration);

  ASSERT_TRUE(deduction.success());
  ASSERT_EQ(deduction.get_deductions().size(), 1);
  ASSERT_EQ(deduction.deduced_value(0).type, Type::Int);

  //std::cout << "Deduced " << deduction.deduction_name(0) << " = " << engine.typeName(deduction.deduced_value(0).type) << std::endl;

  types = std::vector<Type>{ Type::Int, Type::Float };
  deduction = TemplateArgumentDeduction::process(function_template, arguments, types, template_declaration);

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

  parser::Parser parser{ source };
  auto template_declaration = std::static_pointer_cast<ast::TemplateDeclaration>(parser.parseStatement());

  std::vector<TemplateParameter> params{
    TemplateParameter{ TemplateParameter::TypeParameter{}, "T" },
  };

  FunctionTemplate function_template = FunctionTemplateBuilder(Symbol(engine.rootNamespace()), "max")
    .setParams(std::move(params))
    .setScope(Scope{engine.rootNamespace()})
    .withBackend<DummyFunctionTemplateBackend>()
    .get();

  std::vector<TemplateArgument> arguments;
  Type array_int = engine.newArray(Engine::ElementType{ Type::Int }).typeId();
  std::vector<Type> types{ array_int };
  Scope scp{ engine.rootNamespace() };

  TemplateArgumentDeduction deduction = TemplateArgumentDeduction::process(function_template, arguments, types, template_declaration);

  ASSERT_TRUE(deduction.success());
  ASSERT_EQ(deduction.get_deductions().size(), 1);
  ASSERT_EQ(deduction.deduced_value(0).type, Type::Int);

  // std::cout << "Deduced " << deduction.deduction_name(0) << " = " << engine.typeName(deduction.deduced_value(0).type) << std::endl;

  types = std::vector<Type>{ Type::Int };
  deduction = TemplateArgumentDeduction::process(function_template, arguments, types, template_declaration);

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

  parser::Parser parser{ source };
  auto template_declaration = std::static_pointer_cast<ast::TemplateDeclaration>(parser.parseStatement());

  std::vector<TemplateParameter> params{
    TemplateParameter{ TemplateParameter::TypeParameter{}, "R" },
    TemplateParameter{ TemplateParameter::TypeParameter{}, "A" },
  };

  FunctionTemplate function_template = FunctionTemplateBuilder(Symbol(engine.rootNamespace()), "apply")
    .setParams(std::move(params))
    .setScope(Scope{})
    .withBackend<DummyFunctionTemplateBackend>()
    .get();

  DynamicPrototype proto{ Type::Boolean, {Type::Int} };
  FunctionType function_type = engine.typeSystem()->getFunctionType(proto);

  std::vector<TemplateArgument> arguments;
  std::vector<Type> types{ function_type.type(), Type::Int };
  Scope scp{ engine.rootNamespace() };

  TemplateArgumentDeduction deduction = TemplateArgumentDeduction::process(function_template, arguments, types, template_declaration);

  ASSERT_TRUE(deduction.success());
  ASSERT_EQ(deduction.get_deductions().size(), 2);
  ASSERT_EQ(deduction.deduced_value(0).type, Type::Boolean);
  ASSERT_EQ(deduction.deduced_value(1).type, Type::Int);

  //std::cout << "Deduced " << deduction.deduction_name(0) << " = " << engine.typeName(deduction.deduced_value(0).type) << std::endl;
  //std::cout << "Deduced " << deduction.deduction_name(1) << " = " << engine.typeName(deduction.deduced_value(1).type) << std::endl;
}

TEST(TemplateTests, argument_deduction_5) {
  using namespace script;

  const char *source =
    "  template<typename T>                     "
    "  int foo(void(T) func) { }          "
    "                                           "
    "  template<typename T>                     "
    "  int bar(void(const T) func) { }    ";

  Engine engine;
  engine.setup();

  Script s = engine.newScript(SourceFile::fromString(source));
  bool success = s.compile();
  const auto & errors = s.messages();
  ASSERT_TRUE(success);

  ASSERT_EQ(s.rootNamespace().templates().size(), 2);

  FunctionTemplate foo = s.rootNamespace().templates().front().asFunctionTemplate();
  FunctionTemplate bar = s.rootNamespace().templates().back().asFunctionTemplate();

  auto create_func_type = [&engine](const Type & t) -> Type {
    DynamicPrototype proto{ Type::Void , {t} };
    return engine.typeSystem()->getFunctionType(proto).type();
  };

  auto get_definition = [](const FunctionTemplate & ft) -> compiler::TemplateDefinition&
  {
    return dynamic_cast<ScriptFunctionTemplateBackend*>(ft.backend())->definition;
  };

  std::vector<TemplateArgument> targs;
  std::vector<Type> inputs;
  TemplateArgumentDeduction deduction;

  inputs = std::vector<Type>{ create_func_type(Type::Int) };
  deduction = TemplateArgumentDeduction::process(foo, targs, inputs, get_definition(foo).decl_);
  ASSERT_TRUE(deduction.success());
  ASSERT_EQ(deduction.get_deductions().size(), 1);
  ASSERT_EQ(deduction.deduced_value(0).type, Type::Int);

  inputs = std::vector<Type>{ create_func_type(Type{Type::Int}.withConst()) };
  deduction = TemplateArgumentDeduction::process(foo, targs, inputs, get_definition(foo).decl_);
  ASSERT_TRUE(deduction.success()); 
  ASSERT_EQ(deduction.get_deductions().size(), 1);
  ASSERT_EQ(deduction.deduced_value(0).type, Type{ Type::Int }.withConst());

  inputs = std::vector<Type>{ create_func_type(Type{ Type::Int }.withConst()) };
  deduction = TemplateArgumentDeduction::process(bar, targs, inputs, get_definition(bar).decl_);
  ASSERT_TRUE(deduction.success()); 
  ASSERT_EQ(deduction.get_deductions().size(), 1);
  ASSERT_EQ(deduction.deduced_value(0).type, Type::Int);

  inputs = std::vector<Type>{ create_func_type(Type::Int) };
  deduction = TemplateArgumentDeduction::process(bar, targs, inputs, get_definition(bar).decl_);
  ASSERT_TRUE(deduction.success()); /// TODO : should it be a success in this case ?
  ASSERT_EQ(deduction.get_deductions().size(), 0);
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


/****************************************************************
Testing comparison of function template overloading
****************************************************************/

#include "script/compiler/templatespecialization.h"

TEST(TemplateTests, basic_overload_comp) {
  using namespace script;

  const char *source =
    "  template<typename T>         "
    "  T abs(T val) { }             "
    "                               "
    "  template<typename T>         "
    "  T abs(const T val) { }       "
    "                               "
    "  template<typename T>         "
    "  T abs(T & val) { }           "
    "                               "
    "  template<typename T>         "
    "  T abs(const T & val) { }     ";

  Engine engine;
  engine.setup();

  Script s = engine.newScript(SourceFile::fromString(source));
  bool success = s.compile();
  const auto & errors = s.messages();
  ASSERT_TRUE(success);

  ASSERT_EQ(s.rootNamespace().templates().size(), 4);

  FunctionTemplate abs_T = s.rootNamespace().templates().front().asFunctionTemplate();
  FunctionTemplate abs_cT = s.rootNamespace().templates().at(1).asFunctionTemplate();
  FunctionTemplate abs_Tref = s.rootNamespace().templates().at(2).asFunctionTemplate();
  FunctionTemplate abs_cTref = s.rootNamespace().templates().back().asFunctionTemplate();

  ASSERT_EQ(abs_T.name(), "abs");
  ASSERT_EQ(abs_cT.name(), "abs");
  ASSERT_EQ(abs_Tref.name(), "abs");
  ASSERT_EQ(abs_cTref.name(), "abs");

  using TemplatePartialOrdering = compiler::TemplatePartialOrdering;

  TemplatePartialOrdering c = compiler::TemplateSpecialization::compare(abs_T, abs_T);
  ASSERT_EQ(c, TemplatePartialOrdering::Indistinguishable);
  c = compiler::TemplateSpecialization::compare(abs_Tref, abs_Tref);
  ASSERT_EQ(c, TemplatePartialOrdering::Indistinguishable);

  c = compiler::TemplateSpecialization::compare(abs_T, abs_cT);
  ASSERT_EQ(c, TemplatePartialOrdering::SecondIsMoreSpecialized);
  c = compiler::TemplateSpecialization::compare(abs_cT, abs_T);
  ASSERT_EQ(c, TemplatePartialOrdering::FirstIsMoreSpecialized);

  c = compiler::TemplateSpecialization::compare(abs_Tref, abs_cTref);
  ASSERT_EQ(c, TemplatePartialOrdering::SecondIsMoreSpecialized);
  c = compiler::TemplateSpecialization::compare(abs_cTref, abs_Tref);
  ASSERT_EQ(c, TemplatePartialOrdering::FirstIsMoreSpecialized);

  c = compiler::TemplateSpecialization::compare(abs_Tref, abs_cT);
  ASSERT_EQ(c, TemplatePartialOrdering::NotComparable);
  c = compiler::TemplateSpecialization::compare(abs_cT, abs_Tref);
  ASSERT_EQ(c, TemplatePartialOrdering::NotComparable);
}


TEST(TemplateTests, overload_comp_array_overload) {
  using namespace script;

  const char *source =
    "  template<typename T>                      "
    "  int size(const T & a) = delete;           "
    "                                            "
    "  template<typename T>                      "
    "  int size(const Array<T> & a) { }          "
    "                                            "
    "  template<typename T>                      "
    "  int size(const Array<Array<T>> & a) { }   ";

  Engine engine;
  engine.setup();

  Script s = engine.newScript(SourceFile::fromString(source));
  bool success = s.compile();
  const auto & errors = s.messages();
  ASSERT_TRUE(success);

  ASSERT_EQ(s.rootNamespace().templates().size(), 3);

  FunctionTemplate size_T = s.rootNamespace().templates().front().asFunctionTemplate();
  FunctionTemplate size_Array_T = s.rootNamespace().templates().at(1).asFunctionTemplate();
  FunctionTemplate size_Array_Array_T = s.rootNamespace().templates().back().asFunctionTemplate();

  ASSERT_EQ(size_T.name(), "size");
  ASSERT_EQ(size_Array_T.name(), "size");
  ASSERT_EQ(size_Array_Array_T.name(), "size");

  using TemplatePartialOrdering = compiler::TemplatePartialOrdering;

  TemplatePartialOrdering c = compiler::TemplateSpecialization::compare(size_T, size_Array_T);
  ASSERT_EQ(c, TemplatePartialOrdering::SecondIsMoreSpecialized);
  c = compiler::TemplateSpecialization::compare(size_Array_T, size_T);
  ASSERT_EQ(c, TemplatePartialOrdering::FirstIsMoreSpecialized);

  c = compiler::TemplateSpecialization::compare(size_T, size_Array_Array_T);
  ASSERT_EQ(c, TemplatePartialOrdering::SecondIsMoreSpecialized);
  c = compiler::TemplateSpecialization::compare(size_Array_Array_T, size_T);
  ASSERT_EQ(c, TemplatePartialOrdering::FirstIsMoreSpecialized);

  c = compiler::TemplateSpecialization::compare(size_Array_T, size_Array_Array_T);
  ASSERT_EQ(c, TemplatePartialOrdering::SecondIsMoreSpecialized);
  c = compiler::TemplateSpecialization::compare(size_Array_Array_T, size_Array_T);
  ASSERT_EQ(c, TemplatePartialOrdering::FirstIsMoreSpecialized);
}

TEST(TemplateTests, overload_comp_function_type) {
  using namespace script;

  const char *source =
    "  template<typename T>                      "
    "  int foo(T func) = delete;                 "
    "                                            "
    "  template<typename T, typename U>          "
    "  int foo(T(U) func) { }                    ";

  Engine engine;
  engine.setup();

  Script s = engine.newScript(SourceFile::fromString(source));
  bool success = s.compile();
  const auto & errors = s.messages();
  ASSERT_TRUE(success);

  ASSERT_EQ(s.rootNamespace().templates().size(), 2);

  FunctionTemplate foo_T = s.rootNamespace().templates().front().asFunctionTemplate();
  FunctionTemplate foo_T_U = s.rootNamespace().templates().back().asFunctionTemplate();

  ASSERT_EQ(foo_T.name(), "foo");
  ASSERT_EQ(foo_T_U.name(), "foo");
  
  using TemplatePartialOrdering = compiler::TemplatePartialOrdering;

  TemplatePartialOrdering c = compiler::TemplateSpecialization::compare(foo_T, foo_T_U);
  ASSERT_EQ(c, TemplatePartialOrdering::SecondIsMoreSpecialized);
  c = compiler::TemplateSpecialization::compare(foo_T_U, foo_T);
  ASSERT_EQ(c, TemplatePartialOrdering::FirstIsMoreSpecialized);
}

TEST(TemplateTests, overload_comp_not_comparable) {
  using namespace script;

  const char *source =
    "  template<typename T, typename U>          "
    "  int foo(Array<T> a, U func) {}            "
    "                                            "
    "  template<typename T, typename U>          "
    "  int foo(T a, T(U) func) { }               ";

  Engine engine;
  engine.setup();

  Script s = engine.newScript(SourceFile::fromString(source));
  bool success = s.compile();
  const auto & errors = s.messages();
  ASSERT_TRUE(success);

  ASSERT_EQ(s.rootNamespace().templates().size(), 2);

  FunctionTemplate foo_array = s.rootNamespace().templates().front().asFunctionTemplate();
  FunctionTemplate foo_function = s.rootNamespace().templates().back().asFunctionTemplate();

  ASSERT_EQ(foo_array.name(), "foo");
  ASSERT_EQ(foo_function.name(), "foo");

  using TemplatePartialOrdering = compiler::TemplatePartialOrdering;

  TemplatePartialOrdering c = compiler::TemplateSpecialization::compare(foo_array, foo_function);
  ASSERT_EQ(c, TemplatePartialOrdering::NotComparable);
  c = compiler::TemplateSpecialization::compare(foo_function, foo_array);
  ASSERT_EQ(c, TemplatePartialOrdering::NotComparable);
}

TEST(TemplateTests, overload_less_parameters) {
  using namespace script;

  const char *source =
    "  template<typename T, typename U>          "
    "  int foo(T a, U b) {}                      "
    "                                            "
    "  template<typename T>                      "
    "  int foo(T a, T b) { }                     ";

  Engine engine;
  engine.setup();

  Script s = engine.newScript(SourceFile::fromString(source));
  bool success = s.compile();
  const auto & errors = s.messages();
  ASSERT_TRUE(success);

  ASSERT_EQ(s.rootNamespace().templates().size(), 2);

  FunctionTemplate foo_T_U = s.rootNamespace().templates().front().asFunctionTemplate();
  FunctionTemplate foo_T = s.rootNamespace().templates().back().asFunctionTemplate();

  ASSERT_EQ(foo_T_U.name(), "foo");
  ASSERT_EQ(foo_T.name(), "foo");

  using TemplatePartialOrdering = compiler::TemplatePartialOrdering;

  TemplatePartialOrdering c = compiler::TemplateSpecialization::compare(foo_T_U, foo_T);
  ASSERT_EQ(c, TemplatePartialOrdering::SecondIsMoreSpecialized);
  c = compiler::TemplateSpecialization::compare(foo_T, foo_T_U);
  ASSERT_EQ(c, TemplatePartialOrdering::FirstIsMoreSpecialized);
}


/****************************************************************
Testing comparison of partial template specializations
****************************************************************/

TEST(TemplateTests, partial_specializations_comp) {
  using namespace script;

  const char *source =
    "  template<typename T, typename U>                 "
    "  class foo {};                                    "
    "                                                   "
    "  template<typename T>                             "
    "  class foo<T, T> { };                             "
    "                                                   "
    "  template<typename T, typename U>                 "
    "  class foo<Array<T>, U> { };                      "
    "                                                   "
    "  template<typename T, typename U>                 "
    "  class foo<T, U(T)> { };                          "
    "                                                   "
    "  template<typename T, typename U, typename V>     "
    "  class foo<Array<T>, U(V)> { };                   ";

  Engine engine;
  engine.setup();

  Script s = engine.newScript(SourceFile::fromString(source));
  bool success = s.compile();
  const auto & errors = s.messages();
  ASSERT_TRUE(success);

  ASSERT_EQ(s.rootNamespace().templates().size(), 1);

  ClassTemplate foo = s.rootNamespace().templates().front().asClassTemplate();

  ASSERT_EQ(foo.partialSpecializations().size(), 4);

  PartialTemplateSpecialization foo_T_T = foo.partialSpecializations().at(0);
  PartialTemplateSpecialization foo_ArrayT_U = foo.partialSpecializations().at(1);
  PartialTemplateSpecialization foo_T_UT = foo.partialSpecializations().at(2);
  PartialTemplateSpecialization foo_ArrayT_UV = foo.partialSpecializations().at(3);

  using TemplatePartialOrdering = compiler::TemplatePartialOrdering;

  TemplatePartialOrdering c = compiler::TemplateSpecialization::compare(foo_T_T, foo_T_UT);
  ASSERT_EQ(c, TemplatePartialOrdering::SecondIsMoreSpecialized);

  c = compiler::TemplateSpecialization::compare(foo_ArrayT_U, foo_T_UT);
  ASSERT_EQ(c, TemplatePartialOrdering::NotComparable);

  c = compiler::TemplateSpecialization::compare(foo_ArrayT_UV, foo_ArrayT_U);
  ASSERT_EQ(c, TemplatePartialOrdering::FirstIsMoreSpecialized);

  c = compiler::TemplateSpecialization::compare(foo_T_T, foo_T_T);
  ASSERT_EQ(c, TemplatePartialOrdering::Indistinguishable);
}


/****************************************************************
Testing selection of partial template specializations
****************************************************************/

TEST(TemplateTests, partial_specializations_selec) {
  using namespace script;

  const char *source =
    "  template<typename T, typename U>                 "
    "  class foo {};                                    "
    "                                                   "
    "  template<typename T>                             "
    "  class foo<T, T> { };                             "
    "                                                   "
    "  template<typename T, typename U>                 "
    "  class foo<Array<T>, U> { };                      "
    "                                                   "
    "  template<typename T, typename U>                 "
    "  class foo<T, U(T)> { };                          ";

  Engine engine;
  engine.setup();

  Script s = engine.newScript(SourceFile::fromString(source));
  bool success = s.compile();
  const auto & errors = s.messages();
  ASSERT_TRUE(success);

  ASSERT_EQ(s.rootNamespace().templates().size(), 1);

  ClassTemplate foo = s.rootNamespace().templates().front().asClassTemplate();

  ASSERT_EQ(foo.partialSpecializations().size(), 3);

  PartialTemplateSpecialization foo_T_T = foo.partialSpecializations().at(0);
  PartialTemplateSpecialization foo_ArrayT_U = foo.partialSpecializations().at(1);
  PartialTemplateSpecialization foo_T_UT = foo.partialSpecializations().at(2);

  compiler::TemplateSpecializationSelector selector;
  std::vector<TemplateArgument> targs;
  std::pair<PartialTemplateSpecialization, std::vector<TemplateArgument>> result;

  targs = std::vector<TemplateArgument>{ TemplateArgument{Type::Int}, TemplateArgument{ Type::Boolean } };
  result = selector.select(foo, targs);
  ASSERT_TRUE(result.first.isNull());

  targs = std::vector<TemplateArgument>{ TemplateArgument{ Type::Int }, TemplateArgument{ Type::Int } };
  result = selector.select(foo, targs);
  ASSERT_EQ(result.first, foo_T_T);
  ASSERT_EQ(result.second.size(), 1);
  ASSERT_EQ(result.second.front().type, Type::Int);

  Type array_int = engine.newArray(Engine::ElementType{ Type::Int }).typeId();
  targs = std::vector<TemplateArgument>{ TemplateArgument{ array_int }, TemplateArgument{ Type::Boolean } };
  result = selector.select(foo, targs);
  ASSERT_EQ(result.first, foo_ArrayT_U);
  ASSERT_EQ(result.second.size(), 2);
  ASSERT_EQ(result.second.front().type, Type::Int);
  ASSERT_EQ(result.second.back().type, Type::Boolean);

  Type void_int = engine.typeSystem()->getFunctionType(DynamicPrototype{ Type::Void, {Type::Int} }).type();
  targs = std::vector<TemplateArgument>{ TemplateArgument{ Type::Int }, TemplateArgument{ void_int } };
  result = selector.select(foo, targs);
  ASSERT_EQ(result.first, foo_T_UT);
  ASSERT_EQ(result.second.size(), 2);
  ASSERT_EQ(result.second.front().type, Type::Int);
  ASSERT_EQ(result.second.back().type, Type::Void);

  Type int_int = engine.typeSystem()->getFunctionType(DynamicPrototype{ Type::Int, {Type::Int} }).type();
  targs = std::vector<TemplateArgument>{ TemplateArgument{ Type::Void }, TemplateArgument{ int_int } };
  result = selector.select(foo, targs);
  ASSERT_TRUE(result.first.isNull());
}


/****************************************************************
Testing selection of function template overload during full specialization
****************************************************************/

TEST(TemplateTests, full_spec_overload_selec) {
  using namespace script;

  const char *source =
    "  template<typename T>      "
    "  void foo(T a) { }         "
    "                            "
    "  template<typename T>      "
    "  void foo(const T a) { }   "
    "                            "
    "  template<typename T>      "
    "  void foo(Array<T> a) { }  ";

  Engine engine;
  engine.setup();

  Script s = engine.newScript(SourceFile::fromString(source));
  bool success = s.compile();
  const auto & errors = s.messages();
  ASSERT_TRUE(success);

  ASSERT_EQ(s.rootNamespace().templates().size(), 3);

  const auto & candidates = s.rootNamespace().templates();
  FunctionTemplate foo_T = s.rootNamespace().templates().at(0).asFunctionTemplate();
  FunctionTemplate foo_cT = s.rootNamespace().templates().at(1).asFunctionTemplate();
  FunctionTemplate foo_ArrayT = s.rootNamespace().templates().at(2).asFunctionTemplate();

  compiler::TemplateOverloadSelector selector;
  std::vector<TemplateArgument> targs;
  DynamicPrototype proto;
  std::pair<FunctionTemplate, std::vector<TemplateArgument>> result;

  proto = DynamicPrototype{ Type::Void, {Type{Type::Int}} };
  result = selector.select(candidates, targs, proto);
  ASSERT_EQ(result.first, foo_T);
  ASSERT_EQ(result.second.size(), 1);
  ASSERT_EQ(result.second.front().type, Type::Int);

  proto = DynamicPrototype{ Type::Void, {Type{ Type::Int, Type::ConstFlag }} };
  result = selector.select(candidates, targs, proto);
  ASSERT_EQ(result.first, foo_cT);
  ASSERT_EQ(result.second.size(), 1);
  ASSERT_EQ(result.second.front().type, Type::Int);

  proto = DynamicPrototype{ Type::Void, {Type{ Type::Int, Type::ReferenceFlag }} };
  result = selector.select(candidates, targs, proto);
  ASSERT_EQ(result.first, foo_T);
  ASSERT_EQ(result.second.size(), 1);
  ASSERT_EQ(result.second.front().type, Type(Type::Int, Type::ReferenceFlag));

  Type array_int = engine.newArray(Engine::ElementType{ Type::Int }).typeId();
  proto = DynamicPrototype{ Type::Void, {array_int} };
  result = selector.select(candidates, targs, proto);
  ASSERT_EQ(result.first, foo_ArrayT);
  ASSERT_EQ(result.second.size(), 1);
  ASSERT_EQ(result.second.front().type, Type::Int);
}
