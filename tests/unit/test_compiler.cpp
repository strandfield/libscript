// Copyright (C) 2018-2022 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include <gtest/gtest.h>

#include "script/attributes.h"
#include "script/cast.h"
#include "script/class.h"
#include "script/datamember.h"
#include "script/defaultarguments.h"
#include "script/engine.h"
#include "script/enumerator.h"
#include "script/function-impl.h"
#include "script/functionbuilder.h"
#include "script/functioncreator.h"
#include "script/functiontype.h"
#include "script/functiontemplate.h"
#include "script/lambda.h"
#include "script/locals.h"
#include "script/namelookup.h"
#include "script/namespace.h"
#include "script/script.h"
#include "script/staticdatamember.h"
#include "script/typedefs.h"
#include "script/typesystem.h"

#include "script/interpreter/executioncontext.h"

#include "script/compiler/compiler.h"
#include "script/compiler/errors.h"

#include "script/program/expression.h"
#include "script/program/statements.h"

#include "script/parser/parser.h"

#include <array>

// @TODO: avoid calling run() in these tests, do that in the "language_test" target

TEST(CompilerTests, bind_expression) {
  using namespace script;

  const char *source =
    " a= 5 ";

  Engine engine;
  engine.setup();

  compiler::Compiler cmd{ &engine };
  auto expr = cmd.compile(source, engine.currentContext());

  ASSERT_TRUE(expr->is<program::BindExpression>());
  const program::BindExpression & bind = dynamic_cast<const program::BindExpression &>(*expr);
  ASSERT_EQ(bind.name, "a");
}

TEST(CompilerTests, function1) {
  using namespace script;

  const char *source =
    " // This single line comment is going to be ignored \n "
    " /* This multiline comment is going to              \n "
    "    be ignored too! */                              \n "
    "int f(int a, int b) { return 0; }                      ";

  Engine engine;
  engine.setup();

  Script s = engine.newScript(SourceFile::fromString(source));
  bool success = s.compile();
  ASSERT_TRUE(success);

  ASSERT_EQ(s.rootNamespace().functions().size(), 1);
  Function f = s.rootNamespace().functions().front();

  auto code = f.program();

  ASSERT_TRUE(code->is<program::Statement>());
  const program::CompoundStatement & cs = dynamic_cast<const program::CompoundStatement&>(*code);
  {
    ASSERT_EQ(cs.statements.size(), 1);

    {
      auto statement = cs.statements.front();
      ASSERT_TRUE(statement->is<program::ReturnStatement>());
      const program::ReturnStatement & rs = dynamic_cast<const program::ReturnStatement&>(*statement);
      ASSERT_TRUE(rs.returnValue->is<program::Copy>());
      const program::Copy & cop = dynamic_cast<const program::Copy&>(*rs.returnValue);
      ASSERT_TRUE(cop.argument->is<program::Literal>());
      const program::Literal & li = dynamic_cast<const program::Literal&>(*cop.argument);
      ASSERT_EQ(li.value.type(), Type::Int);
    }
    
  }

  Value input = engine.newInt(3);
  Value val = f.invoke({ input , input });
  ASSERT_TRUE(val.type() == Type::Int);
  ASSERT_EQ(val.toInt(), 0);
}

TEST(CompilerTests, deleted_function) {
  using namespace script;

  const char *source =
    "int f(int) = delete; ";

  Engine engine;
  engine.setup();

  Script s = engine.newScript(SourceFile::fromString(source));
  bool success = s.compile();
  const auto & errors = s.messages();
  ASSERT_TRUE(success);
  ASSERT_EQ(s.rootNamespace().functions().size(), 1);
  
  Function f = s.rootNamespace().functions().front();
  ASSERT_EQ(f.returnType(), Type::Int);
  ASSERT_EQ(f.prototype().count(), 1);
  ASSERT_EQ(f.prototype().at(0), Type::Int);
  ASSERT_TRUE(f.isDeleted());
}

TEST(CompilerTests, enum1) {
  using namespace script;

  const char *source =
    " enum A{AA, AB, AC}; ";

  Engine engine;
  engine.setup();

  Script s = engine.newScript(SourceFile::fromString(source));
  bool success = s.compile();
  const auto & errors = s.messages();
  ASSERT_TRUE(success);
  ASSERT_EQ(s.rootNamespace().enums().size(), 1);

  Enum A = s.rootNamespace().enums().front();
  ASSERT_EQ(A.values().size(), 3);
  ASSERT_TRUE(A.hasKey("AA"));
  ASSERT_TRUE(A.hasKey("AB"));
  ASSERT_TRUE(A.hasKey("AC"));
}

TEST(CompilerTests, class1) {
  using namespace script;

  const char *source =
    " class A{ A() {} }; ";

  Engine engine;
  engine.setup();

  Script s = engine.newScript(SourceFile::fromString(source));
  bool success = s.compile();
  const auto & errors = s.messages();
  ASSERT_TRUE(success);
  ASSERT_EQ(s.rootNamespace().classes().size(), 1);

  Class A = s.rootNamespace().classes().front();
  ASSERT_FALSE(A.defaultConstructor().isNull());
}

TEST(CompilerTests, var_decl_auto) {
  using namespace script;

  // @TODO: this could be tested in a script if we had "decltype"

  const char *source =
    " auto a = 5; ";

  Engine engine;
  engine.setup();

  Script s = engine.newScript(SourceFile::fromString(source));
  bool success = s.compile();
  const auto & errors = s.messages();
  ASSERT_TRUE(success);
  ASSERT_EQ(s.globalNames().size(), 1);

  ASSERT_EQ(s.globals().size(), 0);

  s.run();

  ASSERT_EQ(s.globals().size(), 1);

  Value a = s.globals().front();
  ASSERT_EQ(a.type(), Type::Int);
  ASSERT_EQ(a.toInt(), 5);
}

TEST(CompilerTests, lambda) {
  using namespace script;

  const char *source =
    " auto f = [](){ return 42; }; "
    " int a = f();                 ";

  Engine engine;
  engine.setup();

  Script s = engine.newScript(SourceFile::fromString(source));
  bool success = s.compile();
  const auto & errors = s.messages();
  ASSERT_TRUE(success);
  ASSERT_EQ(s.globalNames().size(), 2);

  ASSERT_EQ(s.globals().size(), 0);

  s.run();

  ASSERT_EQ(s.globals().size(), 2);

  Value f = s.globals().front();
  ASSERT_TRUE(f.type().isClosureType());
  Lambda lambda = f.toLambda();
  ASSERT_EQ(lambda.closureType().function().returnType(), Type::Int);

  Function call = lambda.closureType().function();
  ASSERT_TRUE(call.isNonStaticMemberFunction());
  ASSERT_TRUE(call.memberOf().isClosure());
  ASSERT_EQ(call.memberOf().toClosure(), lambda.closureType());

  Value a = s.globals().back();
  ASSERT_EQ(a.type(), Type::Int);
  ASSERT_EQ(a.toInt(), 42);
}

TEST(CompilerTests, operator_overload) {
  using namespace script;

  const char *source =
    " class A {};                                         "
    " int operator+(const A & a, int n) { return n; }     ";

  Engine engine;
  engine.setup();

  Script s = engine.newScript(SourceFile::fromString(source));
  bool success = s.compile();
  const auto & errors = s.messages();
  ASSERT_TRUE(success);
  
  ASSERT_EQ(s.classes().size(), 1);
  Class A = s.classes().front();
  ASSERT_EQ(A.name(), "A");
  ASSERT_FALSE(A.isDefaultConstructible());
  ASSERT_FALSE(A.isCopyConstructible());
  ASSERT_FALSE(A.isMoveConstructible());

  ASSERT_EQ(s.operators().size(), 1);

  Operator op = s.operators().front();
  ASSERT_EQ(op.operatorId(), AdditionOperator);
  ASSERT_EQ(op.returnType(), Type::Int);
  ASSERT_EQ(op.firstOperand(), Type::cref(A.id()));
  ASSERT_EQ(op.secondOperand(), Type::Int);
}

TEST(CompilerTests, operator_overload_2) {
  using namespace script;

  const char *source =
    " class A {                                           "
    " int operator()(int a, int b, int c) { return 0; }     "
    " };     ";

  Engine engine;
  engine.setup();

  Script s = engine.newScript(SourceFile::fromString(source));
  bool success = s.compile();
  const auto & errors = s.messages();
  ASSERT_TRUE(success);

  ASSERT_EQ(s.classes().size(), 1);
  Class A = s.classes().front();
  ASSERT_EQ(A.name(), "A");
  ASSERT_FALSE(A.isDefaultConstructible());
  ASSERT_FALSE(A.isCopyConstructible());
  ASSERT_FALSE(A.isMoveConstructible());

  ASSERT_EQ(A.operators().size(), 1);

  Operator op = A.operators().front();
  ASSERT_EQ(op.operatorId(), FunctionCallOperator);
  ASSERT_EQ(op.returnType(), Type::Int);
  ASSERT_EQ(op.prototype().count(), 4);
  ASSERT_EQ(op.prototype().at(0), Type::ref(A.id()));
  ASSERT_EQ(op.prototype().at(1), Type::Int);
  ASSERT_EQ(op.prototype().at(2), Type::Int);
  ASSERT_EQ(op.prototype().at(3), Type::Int);
}

TEST(CompilerTests, class_with_destructor) {
  using namespace script;

  const char *source =
    "  class A                               "
    "  {                                     "
    "    ~A() { }                            "
    "  };                                    ";

  Engine engine;
  engine.setup();

  Script s = engine.newScript(SourceFile::fromString(source));
  bool success = s.compile();
  const auto & errors = s.messages();
  ASSERT_TRUE(success);
  ASSERT_EQ(s.rootNamespace().classes().size(), 1);

  Class A = s.rootNamespace().classes().front();
  ASSERT_FALSE(A.destructor().isNull());
}

TEST(CompilerTests, class_with_member) {
  using namespace script;

  const char *source =
    "  class A                               "
    "  {                                     "
    "    int a;                              "
    "  };                                    ";

  Engine engine;
  engine.setup();

  Script s = engine.newScript(SourceFile::fromString(source));
  bool success = s.compile();
  const auto & errors = s.messages();
  ASSERT_TRUE(success);
  ASSERT_EQ(s.rootNamespace().classes().size(), 1);

  Class A = s.rootNamespace().classes().front();

  ASSERT_EQ(A.dataMembers().size(), 1);
  auto dm = A.dataMembers().front();
  ASSERT_EQ(dm.type, Type::Int);
  ASSERT_EQ(dm.name, "a");
}


TEST(CompilerTests, class_with_cast) {
  using namespace script;

  const char *source =
    "  class A                               "
    "  {                                     "
    "    int a;                              "
    "    operator int() const { return a; }  "
    "  };                                    ";

  Engine engine;
  engine.setup();

  Script s = engine.newScript(SourceFile::fromString(source));
  bool success = s.compile();
  const auto & errors = s.messages();
  ASSERT_TRUE(success);
  ASSERT_EQ(s.rootNamespace().classes().size(), 1);

  Class A = s.rootNamespace().classes().front();

  ASSERT_EQ(A.dataMembers().size(), 1);
  auto dm = A.dataMembers().front();
  ASSERT_EQ(dm.type, Type::Int);
  ASSERT_EQ(dm.name, "a");

  ASSERT_EQ(A.casts().size(), 1);
  Cast to_int = A.casts().front();
  ASSERT_EQ(to_int.returnType(), Type::Int);
}



TEST(CompilerTests, class2) {
  using namespace script;

  const char *source =
    "  class A                               "
    "  {                                     "
    "    int a;                              "
    "    A() : a(0) { }                      "
    "    ~A() { }                            "
    "    void incr() { ++a; }                "
    "    operator int() const { return a; }  "
    "  };                                    ";

  Engine engine;
  engine.setup();

  Script s = engine.newScript(SourceFile::fromString(source));
  bool success = s.compile();
  const auto & errors = s.messages();
  ASSERT_TRUE(success);
  ASSERT_EQ(s.rootNamespace().classes().size(), 1);

  Class A = s.rootNamespace().classes().front();
  ASSERT_FALSE(A.defaultConstructor().isNull());
  ASSERT_FALSE(A.destructor().isNull());

  ASSERT_EQ(A.memberFunctions().size(), 1);
  Function incr = A.memberFunctions().front();
  ASSERT_EQ(incr.name(), "incr");
  ASSERT_EQ(incr.returnType(), Type::Void);

  ASSERT_EQ(A.casts().size(), 1);
  Cast to_int = A.casts().front();
  ASSERT_EQ(to_int.returnType(), Type::Int);
}

TEST(CompilerTests, generated_default_ctor) {
  using namespace script;

  const char *source =
    "  class A             "
    "  {                   "
    "  public:             "
    "    float x;          "
    "    A() = default;    "
    "    ~A() { }          "
    "  };                  "
    "  A a;                "
    "  float x = a.x;      ";

  Engine engine;
  engine.setup();

  Script s = engine.newScript(SourceFile::fromString(source));
  bool success = s.compile();
  const auto & errors = s.messages();
  ASSERT_TRUE(success);

  Class A = s.classes().front();

  ASSERT_EQ(s.globalNames().size(), 2);

  s.run();

  ASSERT_EQ(s.globals().size(), 2);
  Value a = s.globals().front();
  ASSERT_EQ(a.type(), A.id());

  Value x = s.globals().back();
  ASSERT_EQ(x.type(), Type::Float);
  ASSERT_EQ(x.toFloat(), 0.f);
}


TEST(CompilerTests, generated_dtor) {
  using namespace script;

  const char *source =
    "  class A             "
    "  {                   "
    "    A() = default;    "
    "    ~A() = default;   "
    "  };                  "
    "                      "
    "  { A a; }            ";

  Engine engine;
  engine.setup();

  Script s = engine.newScript(SourceFile::fromString(source));
  bool success = s.compile();
  const auto & errors = s.messages();
  ASSERT_TRUE(success);

  Class A = s.classes().front();
  Function dtor = A.destructor();
  ASSERT_TRUE(!dtor.isNull() && dtor.isDefaulted());
}

TEST(CompilerTests, generated_assignment) {
  using namespace script;

  const char *source =
    "  class A {                                      "
    "  public:                                        "
    "    int val;                                     "
    "    A(int n) : val(n) { }                        "
    "    ~A() { }                                     "
    "                                                 "
    "    A & operator=(const A & other) = default;    "
    "  };                                             "
    "                                                 "
    "  A a(1);                                        "
    "  A b(2);                                        "
    "  a = b;                                         "
    "  int n = a.val;                                 ";

  Engine engine;
  engine.setup();

  Script s = engine.newScript(SourceFile::fromString(source));
  bool success = s.compile();
  const auto & errors = s.messages();
  ASSERT_TRUE(success);

  Class A = s.classes().front();
  ASSERT_EQ(A.operators().size(), 1);
  Operator op = A.operators().front();
  ASSERT_EQ(op.operatorId(), AssignmentOperator);
  ASSERT_TRUE(!op.isNull() && op.isDefaulted());

  s.run();

  ASSERT_EQ(s.globals().size(), 3);
  Value n = s.globals().back();
  ASSERT_EQ(n.type(), Type::Int);
  ASSERT_EQ(n.toInt(), 2);
}

TEST(CompilerTests, default_argument) {
  using namespace script;

  const char *source =
    " int true_random(int result = 42) { return result; } "
    " int a = true_random(66);                            "
    " int b = true_random();                              ";

  Engine engine;
  engine.setup();

  Script s = engine.newScript(SourceFile::fromString(source));
  bool success = s.compile();
  const auto & errors = s.messages();
  ASSERT_TRUE(success);
  ASSERT_EQ(s.rootNamespace().functions().size(), 1);

  Function true_random = s.rootNamespace().functions().front();
  ASSERT_EQ(true_random.name(), "true_random");
  ASSERT_EQ(true_random.returnType(), Type::Int);
  ASSERT_EQ(true_random.prototype().count(), 1);
  ASSERT_EQ(true_random.prototype().at(0), Type::Int);
  ASSERT_EQ(true_random.defaultArguments().size(), 1);

  ASSERT_EQ(s.globalNames().size(), 2);

  ASSERT_EQ(s.globals().size(), 0);

  s.run();

  ASSERT_EQ(s.globals().size(), 2);

  Value a = s.globals().front();
  ASSERT_EQ(a.type(), Type::Int);
  ASSERT_EQ(a.toInt(), 66);

  Value b = s.globals().back();
  ASSERT_EQ(b.type(), Type::Int);
  ASSERT_EQ(b.toInt(), 42);
}



TEST(CompilerTests, inheritance) {
  using namespace script;

  const char *source =
    "  class A {                                   "
    "  public:                                     "
    "    A() { }                                   "
    "    virtual ~A() { }                          "
    "  };                                          "
    "                                              "
    "  class B : A {                               "
    "  public:                                     "
    "    B() { }                                   "
    "    ~B() { }                                  "
    "  };                                          ";

  Engine engine;
  engine.setup();

  Script s = engine.newScript(SourceFile::fromString(source));
  bool success = s.compile();
  const auto & errors = s.messages();
  ASSERT_TRUE(success);
  ASSERT_EQ(s.classes().size(), 2);

  Class A = s.classes().front();
  ASSERT_EQ(A.name(), "A");
  Function dtor = A.destructor();
  ASSERT_TRUE(!dtor.isNull() && dtor.isVirtual());

  Class B = s.classes().back();
  ASSERT_EQ(B.name(), "B");
  ASSERT_EQ(B.parent(), A);
  dtor = B.destructor();
  ASSERT_TRUE(!dtor.isNull() && dtor.isVirtual());
}

TEST(CompilerTests, virtual_call) {
  using namespace script;

  const char *source =
    "  class A {                                   "
    "  public:                                     "
    "    A() { }                                   "
    "    virtual ~A() { }                          "
    "    virtual int foo() const { return 0; }     "
    "  };                                          "
    "                                              "
    "  class B : A {                               "
    "  public:                                     "
    "    B() { }                                   "
    "    ~B() { }                                  "
    "                                              "
    "    int foo() const { return 1; }             "
    "  };                                          "
    "                                              "
    "  int bar(const A & a)                        "
    "  {                                           "
    "    return a.foo();                           "
    "  }                                           "
    "                                              "
    "  B b;                                        "
    "  int n = bar(b);                             ";

  Engine engine;
  engine.setup();

  Script s = engine.newScript(SourceFile::fromString(source));
  bool success = s.compile();
  const auto & errors = s.messages();
  ASSERT_TRUE(success);
  ASSERT_EQ(s.classes().size(), 2);

  Class B = s.classes().back();
  Function foo_B = B.memberFunctions().front();
  ASSERT_TRUE(foo_B.isVirtual());

  Function bar = s.rootNamespace().functions().front();
  auto prog = bar.program();
  const auto & statements = dynamic_cast<const program::CompoundStatement &>(*prog);
  {
    auto ret = std::dynamic_pointer_cast<program::ReturnStatement>(statements.statements.front());
    ASSERT_TRUE(ret->returnValue->is<program::Copy>());
    auto copy = std::dynamic_pointer_cast<program::Copy>(ret->returnValue);
    ASSERT_TRUE(copy->argument->is<program::VirtualCall>());
  }

  s.run();

  ASSERT_EQ(s.globals().size(), 2);

  Value n = s.globals().back();
  ASSERT_EQ(n.type(), Type::Int);
  ASSERT_EQ(n.toInt(), 1);
}


TEST(CompilerTests, uninitialized_function_variable) {
  using namespace script;

  const char *source =
    "  int(int) func;             ";

  Engine engine;
  engine.setup();

  Script s = engine.newScript(SourceFile::fromString(source));
  bool success = s.compile();
  const auto & errors = s.messages();
  ASSERT_FALSE(success);
}

TEST(CompilerTests, function_variable) {
  using namespace script;

  const char *source =
    "  int foo(int a) { return 2*a; }   "
    "  int(int) func = foo;             ";

  Engine engine;
  engine.setup();

  Script s = engine.newScript(SourceFile::fromString(source));
  bool success = s.compile();
  const auto & errors = s.messages();
  ASSERT_TRUE(success);

  s.run();

  ASSERT_EQ(s.globals().size(), 1);

  Value func = s.globals().back();

  DynamicPrototype proto{ Type::Int, {Type::Int} };
  ASSERT_EQ(func.type(), engine.typeSystem()->getFunctionType(proto).type());
}


TEST(CompilerTests, call_to_function_variable) {
  using namespace script;

  const char *source =
    "  int foo(int a) { return 2*a; }   "
    "  int(int) func = foo;             "
    "  int n = func(2);                 ";

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
  ASSERT_EQ(n.toInt(), 4);
}

TEST(CompilerTests, function_variable_assignment) {
  using namespace script;

  const char *source =
    "  int foo(int a) { return 2*a; }   "
    "  int bar(int a) { return 3*a; }   "
    "  int(int) func = foo;             "
    "  func = bar;                      ";

  Engine engine;
  engine.setup();

  Script s = engine.newScript(SourceFile::fromString(source));
  bool success = s.compile();
  const auto & errors = s.messages();
  ASSERT_TRUE(success);

  s.run();

  Function bar = s.rootNamespace().functions().back();
  ASSERT_EQ(bar.name(), "bar");

  ASSERT_EQ(s.globals().size(), 1);
  Value func = s.globals().back();

  DynamicPrototype proto{ Type::Int, {Type::Int} };
  ASSERT_EQ(func.type(), engine.typeSystem()->getFunctionType(proto).type());

  ASSERT_EQ(func.toFunction(), bar);
}

TEST(CompilerTests, typedef_script_scope) {
  using namespace script;

  const char *source =
    "  typedef double Distance;   ";

  Engine engine;
  engine.setup();

  Script s = engine.newScript(SourceFile::fromString(source));
  bool success = s.compile();
  const auto & errors = s.messages();
  ASSERT_TRUE(success);

  ASSERT_EQ(s.rootNamespace().typedefs().size(), 1);
  ASSERT_EQ(s.rootNamespace().typedefs().front(), Typedef("Distance", Type::Double));
}

TEST(CompilerTests, static_data_member) {
  using namespace script;

  const char *source =
    "  class A                "
    "  {                      "
    "  public:                "
    "    static int n = 3;    "
    "    static int p = n+1;  "
    "  };                     ";

  Engine engine;
  engine.setup();

  Script s = engine.newScript(SourceFile::fromString(source));
  bool success = s.compile();
  const auto & errors = s.messages();
  ASSERT_TRUE(success);

  ASSERT_EQ(s.rootNamespace().classes().size(), 1);

  Class A = s.rootNamespace().classes().front();
  ASSERT_EQ(A.staticDataMembers().size(), 2);

  const auto & sdm = A.staticDataMembers();
  auto it = sdm.find("n");
  ASSERT_TRUE(it != sdm.end());
  ASSERT_EQ(it->second.value.type(), Type::Int);
  ASSERT_EQ(it->second.value.toInt(), 3);

  it = sdm.find("p");
  ASSERT_TRUE(it != sdm.end());
  ASSERT_EQ(it->second.value.type(), Type::Int);
  ASSERT_EQ(it->second.value.toInt(), 4);
}

TEST(CompilerTests, static_member_function) {
  using namespace script;

  const char *source =
    "  class A                             "
    "  {                                   "
    "  public:                             "
    "    static int foo() { return 66; }   "
    "  };                                  "
    "                                      ";

  Engine engine;
  engine.setup();

  Script s = engine.newScript(SourceFile::fromString(source));
  bool success = s.compile();
  ASSERT_TRUE(success);

  ASSERT_EQ(s.rootNamespace().classes().size(), 1);

  Class A = s.rootNamespace().classes().front();
  ASSERT_EQ(A.memberFunctions().size(), 1);

  Function foo = A.memberFunctions().front();
  ASSERT_TRUE(foo.isMemberFunction());
  ASSERT_EQ(foo.memberOf(), A);
  ASSERT_TRUE(foo.isStatic());
}

TEST(CompilerTests, namespace_decl_with_function) {
  using namespace script;

  const char *source =
    "  namespace ns {            "
    "    int foo() { return 4; } "
    "    namespace bar { }       "
    "  }                         ";

  Engine engine;
  engine.setup();

  Script s = engine.newScript(SourceFile::fromString(source));
  bool success = s.compile();
  const auto & errors = s.messages();
  ASSERT_TRUE(success);

  ASSERT_EQ(s.rootNamespace().namespaces().size(), 1);
  
  Namespace ns = s.rootNamespace().namespaces().front();
  ASSERT_EQ(ns.name(), "ns");

  ASSERT_EQ(ns.functions().size(), 1);
  ASSERT_EQ(ns.functions().front().name(), "foo");

  ASSERT_EQ(ns.namespaces().size(), 1);
  
  Namespace bar = ns.namespaces().front();
  ASSERT_EQ(bar.name(), "bar");
}

TEST(CompilerTests, namespace_decl_with_variable) {
  using namespace script;

  const char *source =
    "  namespace ns {   "
    "    int n = 4;     "
    "  }                ";

  Engine engine;
  engine.setup();

  Script s = engine.newScript(SourceFile::fromString(source));
  bool success = s.compile();
  const auto & errors = s.messages();
  ASSERT_TRUE(success);

  ASSERT_EQ(s.rootNamespace().namespaces().size(), 1);

  Namespace ns = s.rootNamespace().namespaces().front();
  ASSERT_EQ(ns.name(), "ns");

  ASSERT_EQ(ns.vars().size(), 1);

  auto it = ns.vars().find("n");
  ASSERT_TRUE(it != ns.vars().end());
  ASSERT_EQ(it->second.type(), Type::Int);
  ASSERT_EQ(it->second.toInt(), 4);
}

TEST(CompilerTests, access_specifier_data_member_1) {
  using namespace script;

  const char *source =
    "  class A                        "
    "  {                              "
    "  public:                        "
    "    A() = default;               "
    "    ~A() = default;              "
    "                                 "
    "  private:                       "
    "    double x;                    "
    "    static int a = 0;            "
    "  protected:                     "
    "    double y;                    "
    "    static int b = 0;            "
    "  public:                        "
    "    double z;                    "
    "    static int c = 0;            "
    "  };                             ";

  Engine engine;
  engine.setup();

  Script s = engine.newScript(SourceFile::fromString(source));
  bool success = s.compile();
  const auto & errors = s.messages();
  ASSERT_TRUE(success);

  Class A = s.classes().front();

  ASSERT_EQ(A.dataMembers().size(), 3);

  ASSERT_EQ(A.dataMembers().front().name, "x");
  ASSERT_EQ(A.dataMembers().front().accessibility(), AccessSpecifier::Private);

  ASSERT_EQ(A.dataMembers().at(1).name, "y");
  ASSERT_EQ(A.dataMembers().at(1).accessibility(), AccessSpecifier::Protected);

  ASSERT_EQ(A.dataMembers().back().name, "z");
  ASSERT_EQ(A.dataMembers().back().accessibility(), AccessSpecifier::Public);

  ASSERT_EQ(A.staticDataMembers().size(), 3);
  ASSERT_EQ(A.staticDataMembers().at("a").accessibility(), AccessSpecifier::Private);
  ASSERT_EQ(A.staticDataMembers().at("b").accessibility(), AccessSpecifier::Protected);
  ASSERT_EQ(A.staticDataMembers().at("c").accessibility(), AccessSpecifier::Public);
}

TEST(CompilerTests, friend_class) {
  using namespace script;

  const char *source =
    "  class A                        "
    "  {                              "
    "    friend class B;              "
    "  };                             "
    "                                 "
    "  class B { };                   ";

  Engine engine;
  engine.setup();

  Script s = engine.newScript(SourceFile::fromString(source));
  bool success = s.compile();
  const auto & errors = s.messages();
  ASSERT_TRUE(success);

  const auto & classes = s.classes();
  ASSERT_EQ(classes.size(), 2);

  Class A = classes.front();
  ASSERT_EQ(A.name(), "A");

  ASSERT_EQ(A.friends(Class{}).size(), 1);
  ASSERT_EQ(A.friends(Class{}).front().name(), "B");
}


TEST(CompilerTests, function_template_full_spec) {
  using namespace script;

  const char* source_1 =
    "  template<typename T>                "
    "  int foo(T a) { return 1; }          "
    "                                      "
    "  template<>                          "
    "  int foo<int>(int a) { return 0; }   "
    "                                      "
    "  int a = foo<bool>(false);           "
    "  int b = foo<int>(0);                ";

  // template argument deduction for the win !
  const char* source_2 =
    "  template<typename T>                "
    "  int foo(T a) { return 1; }          "
    "                                      "
    "  template<>                          "
    "  int foo(int a) { return 0; }        "
    "                                      "
    "  int a = foo(false);                 "
    "  int b = foo(0);                     ";


  Engine engine;
  engine.setup();

  std::array<const char*, 2> sources = { source_1, source_2 };

  for (const char* src : sources)
  {
    Script s = engine.newScript(SourceFile::fromString(src));
    bool success = s.compile();
    ASSERT_TRUE(success);

    ASSERT_EQ(s.rootNamespace().templates().size(), 1);

    FunctionTemplate foo = s.rootNamespace().templates().front().asFunctionTemplate();
    ASSERT_EQ(foo.instances().size(), 2);

    const auto& instances = foo.instances();
    auto it = instances.begin();
    ASSERT_EQ(it->first.at(0).type, script::Type::Boolean);

    ++it;
    ASSERT_EQ(it->first.at(0).type, script::Type::Int);
  }
}

TEST(CompilerTests, attributes) {
  using namespace script;

  const char* source =
    " [[no_discard]] int foo() { return 5; }         "
    " class [[maybe_unused]] A { };                  ";

  Engine engine;
  engine.setup();

  Script s = engine.newScript(SourceFile::fromString(source));
  bool success = s.compile();
  const auto& errors = s.messages();
  ASSERT_TRUE(success);
  ASSERT_EQ(s.rootNamespace().classes().size(), 1);
  ASSERT_EQ(s.rootNamespace().functions().size(), 1);

  {
    Class A = s.rootNamespace().classes().front();
    Attributes attrs = Symbol(A).attributes();
    ASSERT_EQ(attrs.size(), 1);
    ASSERT_EQ(attrs.at(0)->source().toString(), "maybe_unused");
  }

  {
    Function foo = s.rootNamespace().functions().front();
    Attributes attrs = foo.attributes();
    ASSERT_EQ(attrs.size(), 1);
    ASSERT_EQ(attrs.at(0)->source().toString(), "no_discard");
  }
}

TEST(CompilerTests, idattribute) {
  using namespace script;

  const char* source =
    " class [[id(\"ghi\")]] A { }; ";

  struct abc { };
  struct def { };
  struct ghi { };

  Engine engine;
  engine.setup();

  engine.registerType<abc>("abc");
  engine.registerType<def>("def");
  engine.registerType<ghi>("ghi");

  Script s = engine.newScript(SourceFile::fromString(source));
  bool success = s.compile();
  const auto& errors = s.messages();
  ASSERT_TRUE(success);
  ASSERT_EQ(s.rootNamespace().classes().size(), 1);

  {
    Class A = s.rootNamespace().classes().front();
    Type t = engine.getType<ghi>();
    ASSERT_EQ(A.id(), t.data());
  }
}

class MyFunction : public script::FunctionImpl
{
public:
  std::string m_name;
  script::DynamicPrototype proto;

public:
  explicit MyFunction(script::Symbol sym, std::string name)
    : FunctionImpl(sym.engine()),
    m_name(std::move(name))
  {
    script::Engine* e = sym.engine();
    enclosing_symbol = sym.impl();
    proto.setReturnType(script::Type::Int);
  }

  script::SymbolKind get_kind() const override
  {
    return script::SymbolKind::Function;
  }

  const std::string& name() const override
  {
    return m_name;
  }

  script::Name get_name() const override
  {
    return script::Name(script::SymbolKind::Function, name());
  }

  bool is_native() const override
  {
    return true;
  }

  void set_body(std::shared_ptr<script::program::Statement>) override
  {

  }

  const script::Prototype& prototype() const override
  {
    return proto;
  }

  script::Value invoke(script::FunctionCall* c) override
  {
    return c->engine()->newInt(6);
  }
};

class MyNativeFunctionCompiler : public script::FunctionCreator
{
public:

  script::Function create(script::FunctionBlueprint& blueprint, const std::shared_ptr<script::ast::FunctionDecl>& fdecl, std::vector<script::Attribute>& attrs) override
  {
    if (!attrs.empty() && attrs.front()->source() == "the_native_func")
    {
      return script::Function(std::make_shared<MyFunction>(blueprint.parent(), blueprint.name_.string()));
    }
    else
    {
      return FunctionCreator::create(blueprint, fdecl, attrs);
    }
  }
};

TEST(CompilerTests, nativefunction) {
  using namespace script;

  const char* source =
    " [[the_native_func]] int foo() = default; \n"
    " int bar() { return foo(); }               ";

  Engine engine;
  engine.setup();

  MyNativeFunctionCompiler funcompiler;

  Script s = engine.newScript(SourceFile::fromString(source));
  bool success = s.compile(CompileMode::Release, &funcompiler);
  const auto& errors = s.messages();
  ASSERT_TRUE(success);
  ASSERT_EQ(s.rootNamespace().functions().size(), 2);

  auto lookup = NameLookup::resolve("bar", s);
  ASSERT_EQ(lookup.functions().size(), 1);

  Function bar = lookup.functions().front();
  Locals locals;
  Value x = bar.call(locals);
  ASSERT_EQ(x.type(), Type::Int);
  ASSERT_EQ(x.toInt(), 6);
}
