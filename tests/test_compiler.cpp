// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include <gtest/gtest.h>

#include "script/engine.h"
#include "script/functionbuilder.h"
#include "script/functiontype.h"
#include "script/enumvalue.h"

#include "script/compiler/compiler.h"
#include "script/compiler/functioncompiler.h"
#include "script/compiler/scriptcompiler.h"

#include "script/program/expression.h"
#include "script/program/statements.h"

#include "script/parser/parser.h"


void test_operation(const char *source, script::Operator::BuiltInOperator op1, script::Operator::BuiltInOperator op2, script::Operator::BuiltInOperator op3)
{
  using namespace script;

  Engine engine;
  engine.setup();

  compiler::Compiler compiler{ &engine };
  auto expr = compiler.compile(source, engine.currentContext());

  ASSERT_TRUE(expr->is<program::FunctionCall>());
  const program::FunctionCall & call = dynamic_cast<const program::FunctionCall &>(*expr);
  ASSERT_TRUE(call.callee.isOperator());
  Operator op = call.callee.toOperator();
  ASSERT_EQ(op.operatorId(), op1);

  ASSERT_EQ(call.args.size(), 2);


  if (op2 != Operator::Null)
  {
    ASSERT_TRUE(call.args.front()->is<program::FunctionCall>());
    const program::FunctionCall & rhs = dynamic_cast<const program::FunctionCall &>(*(call.args.front()));
    ASSERT_TRUE(rhs.callee.isOperator());
    op = rhs.callee.toOperator();
    ASSERT_EQ(op.operatorId(), op2);
  }

  if (op3 != Operator::Null)
  {
    ASSERT_TRUE(call.args.back()->is<program::FunctionCall>());
    const program::FunctionCall & rhs = dynamic_cast<const program::FunctionCall &>(*(call.args.back()));
    ASSERT_TRUE(rhs.callee.isOperator());
    op = rhs.callee.toOperator();
    ASSERT_EQ(op.operatorId(), op3);
  }

}

TEST(CompilerTests, expressions) {
  using namespace script;

  auto None = Operator::Null;

  test_operation(" 2+3*5 ", Operator::AdditionOperator, None, Operator::MultiplicationOperator);
  test_operation(" 3*5 + 2 ", Operator::AdditionOperator, Operator::MultiplicationOperator, None);
  test_operation(" 1 << 2 + 3  ", Operator::LeftShiftOperator, None, Operator::AdditionOperator);
  test_operation(" true && false || true ", Operator::LogicalOrOperator, Operator::LogicalAndOperator, None);
  test_operation(" true || false && true ", Operator::LogicalOrOperator, None, Operator::LogicalAndOperator);
  test_operation(" 1 ^ 3 | 1 & 3 ", Operator::BitwiseOrOperator, Operator::BitwiseXorOperator, Operator::BitwiseAndOperator);

}

TEST(CompilerTests, bind_expression) {
  using namespace script;

  const char *source =
    " a= 5 ";

  Engine engine;
  engine.setup();

  compiler::Compiler compiler{ &engine };
  auto expr = compiler.compile(source, engine.currentContext());

  ASSERT_TRUE(expr->is<program::BindExpression>());
  const program::BindExpression & bind = dynamic_cast<const program::BindExpression &>(*expr);
  ASSERT_EQ(bind.name, "a");
}

TEST(CompilerTests, function1) {
  using namespace script;

  const char *source =
    "int f(int a, int b) { return 0; } ";

  Engine engine;
  engine.setup();

  Script s = engine.newScript(SourceFile::fromString(source));
  bool success = s.compile();
  const auto & errors = s.messages();
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
  engine.manage(input);
  Value val = engine.call(f, { input , input });
  ASSERT_TRUE(val.type() == Type::Int);
  ASSERT_EQ(val.toInt(), 0);
}



TEST(CompilerTests, function2) {
  using namespace script;

  const char *source =
    "int max(int a, int b) { return a > b ? a : b; } ";

  Engine engine;
  engine.setup();

  Script s = engine.newScript(SourceFile::fromString(source));
  bool success = s.compile();
  const auto & errors = s.messages();
  ASSERT_TRUE(success);

  ASSERT_EQ(s.rootNamespace().functions().size(), 1);
  Function f = s.rootNamespace().functions().front();

  ASSERT_EQ(f.returnType(), Type::Int);
  ASSERT_EQ(f.prototype().argc(), 2);
  ASSERT_EQ(f.parameter(0), Type::Int);
  ASSERT_EQ(f.parameter(1), Type::Int);

  auto code = f.program();

  Value a = engine.newInt(3);
  engine.manage(a);
  Value b = engine.newInt(4);
  engine.manage(b);
  Value val = engine.call(f, { a , b });
  ASSERT_TRUE(val.type() == Type::Int);
  ASSERT_EQ(val.toInt(), 4);
}

TEST(CompilerTests, expr_statement1) {
  using namespace script;

  const char *source =
    " int n = 0; n = n+1; ";

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

  Value x = s.globals().front();
  ASSERT_EQ(x.type(), Type::Int);
  ASSERT_EQ(x.toInt(), 1);
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
  ASSERT_EQ(f.prototype().argc(), 1);
  ASSERT_EQ(f.prototype().argv(0), Type::Int);
  ASSERT_TRUE(f.isDeleted());
}


TEST(CompilerTests, call_deleted_function) {
  using namespace script;

  const char *source =
    "int f(int) = delete; f(5); ";

  Engine engine;
  engine.setup();

  Script s = engine.newScript(SourceFile::fromString(source));
  bool success = s.compile();
  const auto & errors = s.messages();
  ASSERT_FALSE(success);
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



TEST(CompilerTests, enum_assignment) {
  using namespace script;

  const char *source =
    " enum A{AA, AB, AC}; "
    " A a = AA;           "
    " a = AB;             ";

  Engine engine;
  engine.setup();

  Script s = engine.newScript(SourceFile::fromString(source));
  bool success = s.compile();
  const auto & errors = s.messages();
  ASSERT_TRUE(success);
  ASSERT_EQ(s.rootNamespace().enums().size(), 1);

  Enum A = s.rootNamespace().enums().front();
  
  s.run();

  Value a = s.globals().front();
  ASSERT_EQ(a.type(), A.id());
  EnumValue ev = a.toEnumValue();
  ASSERT_EQ(ev.enumeration().getValue("AB"), ev.value());
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

TEST(CompilerTests, two_functions1) {
  using namespace script;

  const char *source =
    " const int & clamp(const int & v, const int & lo, const int & hi)    "
    " { if(v < lo) return lo; else if(v > hi) return hi; else return v; } "
    " "
    " int clamp_ten(int a) { return clamp(a, 0, 10); }                    ";

  Engine engine;
  engine.setup();

  Script s = engine.newScript(SourceFile::fromString(source));
  bool success = s.compile();
  const auto & errors = s.messages();
  ASSERT_TRUE(success);
  ASSERT_EQ(s.rootNamespace().functions().size(), 2);

  Value a = engine.eval("clamp_ten(5)", s);
  ASSERT_EQ(a.type(), Type::Int);
  ASSERT_EQ(a.toInt(), 5);
  engine.destroy(a);
}

TEST(CompilerTests, var_decl_auto) {
  using namespace script;

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
  LambdaObject lambda = f.toLambda();
  ASSERT_EQ(lambda.closureType().function().returnType(), Type::Int);


  Value a = s.globals().back();
  ASSERT_EQ(a.type(), Type::Int);
  ASSERT_EQ(a.toInt(), 42);
}

TEST(CompilerTests, lambda_with_capture) {
  using namespace script;

  const char *source =
    " int x = 0;                     "
    " auto f = [&x](){ ++x;       }; "
    " f(); f();                      ";

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

  Value x = s.globals().front();
  ASSERT_EQ(x.type(), Type::Int);
  ASSERT_EQ(x.toInt(), 2);
}


TEST(CompilerTests, lambda_capture_all_by_value) {
  using namespace script;

  const char *source =
    " int x = 57;                    "
    " auto f = [=](){ return x; };   "
    " int y = f();                   ";

  Engine engine;
  engine.setup();

  Script s = engine.newScript(SourceFile::fromString(source));
  bool success = s.compile();
  const auto & errors = s.messages();
  ASSERT_TRUE(success);
  ASSERT_EQ(s.globalNames().size(), 3);

  ASSERT_EQ(s.globals().size(), 0);

  s.run();

  ASSERT_EQ(s.globals().size(), 3);

  Value y = s.globals().back();
  ASSERT_EQ(y.type(), Type::Int);
  ASSERT_EQ(y.toInt(), 57);
}

TEST(CompilerTests, lambda_capture_all_by_ref) {
  using namespace script;

  const char *source =
    " int x = 57;                    "
    " auto f = [&](){ return x++; };   "
    " int y = f();                   ";

  Engine engine;
  engine.setup();

  Script s = engine.newScript(SourceFile::fromString(source));
  bool success = s.compile();
  const auto & errors = s.messages();
  ASSERT_TRUE(success);
  ASSERT_EQ(s.globalNames().size(), 3);

  ASSERT_EQ(s.globals().size(), 0);

  s.run();

  ASSERT_EQ(s.globals().size(), 3);

  Value x = s.globals().front();
  ASSERT_EQ(x.type(), Type::Int);
  ASSERT_EQ(x.toInt(), 58);

  Value y = s.globals().back();
  ASSERT_EQ(y.type(), Type::Int);
  ASSERT_EQ(y.toInt(), 57);
}

TEST(CompilerTests, lambda_capture_all_by_value_and_one_by_ref) {
  using namespace script;

  const char *source =
    " int x = 1;                                         "
    " int y = 2;                                         "
    " int z = 3;                                         "
    " auto f = [=, &z](){ z = z + x + y; y = y + 1; };   "
    " f();                                               ";

  Engine engine;
  engine.setup();

  Script s = engine.newScript(SourceFile::fromString(source));
  bool success = s.compile();
  const auto & errors = s.messages();
  ASSERT_TRUE(success);
  ASSERT_EQ(s.globalNames().size(), 4);

  ASSERT_EQ(s.globals().size(), 0);

  s.run();

  ASSERT_EQ(s.globals().size(), 4);

  Value x = s.globals().front();
  ASSERT_EQ(x.type(), Type::Int);
  ASSERT_EQ(x.toInt(), 1);

  Value y = s.globals().at(1);
  ASSERT_EQ(y.type(), Type::Int);
  ASSERT_EQ(y.toInt(), 2);

  Value z = s.globals().at(2);
  ASSERT_EQ(z.type(), Type::Int);
  ASSERT_EQ(z.toInt(), 6);
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
  ASSERT_EQ(op.operatorId(), Operator::AdditionOperator);
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
  ASSERT_EQ(op.operatorId(), Operator::FunctionCallOperator);
  ASSERT_EQ(op.returnType(), Type::Int);
  ASSERT_EQ(op.prototype().argc(), 4);
  ASSERT_EQ(op.prototype().argv(0), Type::ref(A.id()));
  ASSERT_EQ(op.prototype().argv(1), Type::Int);
  ASSERT_EQ(op.prototype().argv(2), Type::Int);
  ASSERT_EQ(op.prototype().argv(3), Type::Int);
}

TEST(CompilerTests, calling_functor) {
  using namespace script;

  const char *source =
    "  class A {                                               "
    "  public:                                                 "
    "    A() { }                                               "
    "    int operator()(int a, int b, int c) { return a-c; }   "
    "  };                                                      "
    "  A a;                                                    "
    "  int n = a(1, 2, 3);                                     ";

  Engine engine;
  engine.setup();

  Script s = engine.newScript(SourceFile::fromString(source));
  bool success = s.compile();
  const auto & errors = s.messages();
  ASSERT_TRUE(success);

  ASSERT_EQ(s.globalNames().size(), 2);

  s.run();

  ASSERT_EQ(s.globals().size(), 2);

  Value n = s.globals().back();
  ASSERT_EQ(n.type(), Type::Int);
  ASSERT_EQ(n.toInt(), -2);
}


TEST(CompilerTests, user_defined_literals) {
  using namespace script;

  const char *source =
    " double operator\"\"km (double x) { return x; } "
    " auto d = 3km;                                  ";

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

  Value d = s.globals().front();
  ASSERT_EQ(d.type(), Type::Double);
  ASSERT_EQ(d.toDouble(), 3.0);
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


TEST(CompilerTests, member_function_and_cast) {
  using namespace script;

  const char *source =
    "  class A                                 "
    "  {                                       "
    "  public:                                 "
    "    int a;                                "
    "    A() : a(0) { }                        "
    "    ~A() { }                              "
    "    void incr(int n) { a += n; }          "
    "    operator int() const { return a; }    "
    "  };                                      "
    "                                          "
    "  A a;                                    "
    "  a.incr(2);                              "
    "  int b = a;                              ";

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

  Value b = s.globals().back();
  ASSERT_EQ(b.type(), Type::Int);
  ASSERT_EQ(b.toInt(), 2);
}


TEST(CompilerTests, converting_constructor) {
  using namespace script;

  const char *source =
    "  class A            \n"
    "  {                  \n"
    "    A(float x) { }   \n"
    "  };                 \n"
    "  A a = 3.14f;       \n";

  Engine engine;
  engine.setup();

  Script s = engine.newScript(SourceFile::fromString(source));
  bool success = s.compile();
  const auto & errors = s.messages();
  ASSERT_TRUE(success);

  Class A = s.classes().front();

  ASSERT_EQ(s.globalNames().size(), 1);

  s.run();

  ASSERT_EQ(s.globals().size(), 1);
  Value a = s.globals().front();
  ASSERT_EQ(a.type(), A.id());
}



TEST(CompilerTests, generated_default_ctor) {
  using namespace script;

  const char *source =
    "  class A             "
    "  {                   "
    "  public:             "
    "    float x;          "
    "    A() = default;    "
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
  ASSERT_EQ(true_random.prototype().argc(), 1);
  ASSERT_EQ(true_random.prototype().argv(0), Type::Int);
  ASSERT_TRUE(true_random.accepts(0));
  ASSERT_TRUE(true_random.accepts(1));

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

  Prototype proto{ Type::Int, Type::Int };
  ASSERT_EQ(func.type(), engine.getFunctionType(proto).type());
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

  Prototype proto{ Type::Int, Type::Int };
  ASSERT_EQ(func.type(), engine.getFunctionType(proto).type());

  ASSERT_EQ(func.toFunction(), bar);
}


TEST(CompilerTests, brace_initialization) {
  using namespace script;

  /// TODO : remove the need of a copy constructor
  const char *source =
    "  int a{5};                   "
    "  int & ref{a};               "
    "  class A {                   "
    "    int n;                    "
    "    A(const A &) = default;   "
    "    A(int val) : n(val) { }   "
    "  };                          "
    "  A b{5};                     "
    "  A c = A{5};                 ";

  Engine engine;
  engine.setup();

  Script s = engine.newScript(SourceFile::fromString(source));
  bool success = s.compile();
  const auto & errors = s.messages();
  ASSERT_TRUE(success);
  s.run();
}

TEST(CompilerTests, ctor_initialization) {
  using namespace script;

  /// TODO : remove the need of a copy constructor
  const char *source =
    "  int a(5);                   "
    "  int & ref(a);               "
    "  class A {                   "
    "    int n;                    "
    "    A(const A &) = default;   "
    "    A(int val) : n(val) { }   "
    "  };                          "
    "  A b(5);                     "
    "  A c = A(5);                 ";

  Engine engine;
  engine.setup();

  Script s = engine.newScript(SourceFile::fromString(source));
  bool success = s.compile();
  const auto & errors = s.messages();
  ASSERT_TRUE(success);
  s.run();
}


TEST(CompilerTests, access_global) {
  using namespace script;

  const char *source =
    "  int n = 5;                 "
    "  int get_n() { return n; }  "
    "  int a = get_n() + 5;       ";

  Engine engine;
  engine.setup();

  Script s = engine.newScript(SourceFile::fromString(source));
  bool success = s.compile();
  const auto & errors = s.messages();
  ASSERT_TRUE(success);
  s.run();

  ASSERT_EQ(s.globals().size(), 2);
  Value n = s.globals().front();
  ASSERT_EQ(n.toInt(), 5);
  Value a = s.globals().back();
  ASSERT_EQ(a.type(), Type::Int);
  ASSERT_EQ(a.toInt(), 10);
}

TEST(CompilerTests, typedef_script_scope) {
  using namespace script;

  const char *source =
    "  typedef double Distance;   "
    "  Distance d = 3.0;          ";

  Engine engine;
  engine.setup();

  Script s = engine.newScript(SourceFile::fromString(source));
  bool success = s.compile();
  const auto & errors = s.messages();
  ASSERT_TRUE(success);

  ASSERT_EQ(s.rootNamespace().typedefs().size(), 1);
  ASSERT_EQ(s.rootNamespace().typedefs().front(), Typedef("Distance", Type::Double));

  s.run();

  ASSERT_EQ(s.globals().size(), 1);
  Value d = s.globals().front();
  ASSERT_EQ(d.type(), Type::Double);
  ASSERT_EQ(d.toDouble(), 3.0);
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
  ASSERT_TRUE(it->second.value.isInitialized());
  ASSERT_EQ(it->second.value.type(), Type::Int);
  ASSERT_EQ(it->second.value.toInt(), 3);

  it = sdm.find("p");
  ASSERT_TRUE(it != sdm.end());
  ASSERT_TRUE(it->second.value.isInitialized());
  ASSERT_EQ(it->second.value.type(), Type::Int);
  ASSERT_EQ(it->second.value.toInt(), 4);
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
  ASSERT_TRUE(it->second.isInitialized());
  ASSERT_EQ(it->second.type(), Type::Int);
  ASSERT_EQ(it->second.toInt(), 4);
}

TEST(CompilerTests, access_specifier_function_1) {
  using namespace script;

  const char *source =
    "  class A                        "
    "  {                              "
    "  public:                        "
    "    A() = default;               "
    "    ~A() = default;              "
    "                                 "
    "  private:                       "
    "    int bar() { return 57; }     "
    "  };                             "
    "                                 "
    "  A a;                           "
    "  int n = a.bar();               ";

  Engine engine;
  engine.setup();

  Script s = engine.newScript(SourceFile::fromString(source));
  bool success = s.compile();
  const auto & errors = s.messages();
  ASSERT_FALSE(success);
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

TEST(CompilerTests, access_specifier_data_member_2) {
  using namespace script;

  const char *source =
    "  class A                        "
    "  {                              "
    "  public:                        "
    "    A() = default;               "
    "    ~A() = default;              "
    "                                 "
    "  private:                       "
    "    int n;                       "
    "  };                             "
    "                                 "
    "  A a;                           "
    "  int n = a.n;                   ";

  Engine engine;
  engine.setup();

  Script s = engine.newScript(SourceFile::fromString(source));
  bool success = s.compile();
  const auto & errors = s.messages();
  ASSERT_FALSE(success);
}

TEST(CompilerTests, access_specifier_data_member_3) {
  using namespace script;

  const char *source =
    "  class A                        "
    "  {                              "
    "  public:                        "
    "    A() = default;               "
    "    ~A() = default;              "
    "                                 "
    "  private:                       "
    "    static int a = 0;            "
    "  };                             "
    "                                 "
    "  int n = A::a;                  ";

  Engine engine;
  engine.setup();

  Script s = engine.newScript(SourceFile::fromString(source));
  bool success = s.compile();
  const auto & errors = s.messages();
  ASSERT_FALSE(success);
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

TEST(CompilerTests, for_loop_1) {
  using namespace script;

  const char *source =
    "  int n = 0;                   "
    "  for(int i(0); i < 10; ++i)   "
    "    n = n + i;                 ";

  Engine engine;
  engine.setup();

  Script s = engine.newScript(SourceFile::fromString(source));
  bool success = s.compile();
  const auto & errors = s.messages();
  ASSERT_TRUE(success);

  ASSERT_EQ(s.globalNames().size(), 1);

  s.run();

  ASSERT_EQ(s.globals().size(), 1);

  Value x = s.globals().front();
  ASSERT_EQ(x.type(), Type::Int);
  ASSERT_EQ(x.toInt(), 45);
}

TEST(CompilerTests, for_loop_continue) {
  using namespace script;

  const char *source =
    "  int n = 0;                  "
    "  for(int i(0); i < 10; ++i)  "
    "  {                           "
    "    if(i == 5)                "
    "      continue;               "
    "    n = n + i;                "
    "  }                           ";

  Engine engine;
  engine.setup();

  Script s = engine.newScript(SourceFile::fromString(source));
  bool success = s.compile();
  const auto & errors = s.messages();
  ASSERT_TRUE(success);

  ASSERT_EQ(s.globalNames().size(), 1);

  s.run();

  ASSERT_EQ(s.globals().size(), 1);

  Value x = s.globals().front();
  ASSERT_EQ(x.type(), Type::Int);
  ASSERT_EQ(x.toInt(), 40);
}

TEST(CompilerTests, for_loop_break) {
  using namespace script;

  const char *source =
    "  int n = 0;                  "
    "  for(int i(0); i < 10; ++i)  "
    "  {                           "
    "    if(i == 5)                "
    "      break;                  "
    "    n = n + i;                "
    "  }                           ";

  Engine engine;
  engine.setup();

  Script s = engine.newScript(SourceFile::fromString(source));
  bool success = s.compile();
  const auto & errors = s.messages();
  ASSERT_TRUE(success);

  ASSERT_EQ(s.globalNames().size(), 1);

  s.run();

  ASSERT_EQ(s.globals().size(), 1);

  Value x = s.globals().front();
  ASSERT_EQ(x.type(), Type::Int);
  ASSERT_EQ(x.toInt(), 10);
}

TEST(CompilerTests, type_alias_1) {
  using namespace script;

  const char *source =
    "  using Distance = double;  "
    "  Distance d = 3.14;        ";

  Engine engine;
  engine.setup();

  Script s = engine.newScript(SourceFile::fromString(source));
  bool success = s.compile();
  const auto & errors = s.messages();
  ASSERT_TRUE(success);

  ASSERT_EQ(s.globalNames().size(), 1);

  s.run();

  ASSERT_EQ(s.globals().size(), 1);

  Value d = s.globals().front();
  ASSERT_EQ(d.type(), Type::Double);
  ASSERT_EQ(d.toDouble(), 3.14);
}

TEST(CompilerTests, using_declaration_1) {
  using namespace script;

  const char *source =
    "  namespace foo {             "
    "    int get() { return 4; }   "
    "  }                           "
    "                              "
    "  int bar()                   "
    "  {                           "
    "    using foo::get;           "
    "    return get();             "
    "  }                           "
    "                              "
    "  int n = bar();              ";

  Engine engine;
  engine.setup();

  Script s = engine.newScript(SourceFile::fromString(source));
  bool success = s.compile();
  const auto & errors = s.messages();
  ASSERT_TRUE(success);

  ASSERT_EQ(s.globalNames().size(), 1);

  s.run();

  ASSERT_EQ(s.globals().size(), 1);

  Value n = s.globals().front();
  ASSERT_EQ(n.type(), Type::Int);
  ASSERT_EQ(n.toInt(), 4);
}

TEST(CompilerTests, using_declaration_2) {
  using namespace script;

  const char *source =
    "  namespace foo {             "
    "    int get() { return 4; }   "
    "  }                           "
    "                              "
    "  using foo::get;             "
    "                              "
    "  int bar()                   "
    "  {                           "
    "    return get();             "
    "  }                           "
    "                              "
    "  int n = bar();              ";

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
  ASSERT_EQ(n.toInt(), 4);
}

TEST(CompilerTests, namespace_alias_1) {
  using namespace script;

  const char *source =
    "  namespace foo {             "
    "    int get() { return 4; }   "
    "  }                           "
    "                              "
    "  namespace qux = foo;        "
    "                              "
    "  int bar()                   "
    "  {                           "
    "    return qux::get();        "
    "  }                           "
    "                              "
    "  int n = bar();              ";

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
  ASSERT_EQ(n.toInt(), 4);
}

TEST(CompilerTests, unknown_type) {
  using namespace script;

  const char *source =
    "  size_t get_size() { return 42; } "
    "  typedef int size_t;              "
    "  size_t n = get_size();           ";

  Engine engine;
  engine.setup();

  Script s = engine.newScript(SourceFile::fromString(source));
  bool success = s.compile();
  const auto & errors = s.messages();

  // When processing get_size() the first time, size_t is not defined 
  // and get_size() is added to a list of function that needs a second pass.
  // The second pass correctly resolved size_t.
  // The old system worked with only a single pass by recording all functions 
  // and processing them later, but it could not handle using declarations and 
  // other constructs correctly. 
  ASSERT_TRUE(success);

  s.run();

  ASSERT_EQ(s.globals().size(), 1);
  Value n = s.globals().front();
  ASSERT_EQ(n.type(), Type::Int);
  ASSERT_EQ(n.toInt(), 42);
}