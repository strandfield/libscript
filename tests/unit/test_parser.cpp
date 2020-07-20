// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include <gtest/gtest.h>

#include "script/parser/lexer.h"
#include "script/parser/token.h"
#include "script/parser/parser.h"
#include "script/parser/parsererrors.h"

#include "script/ast/node.h"
#include "script/ast/ast_p.h"


std::shared_ptr<script::parser::ParserContext> parser_context(const char *source)
{
  auto ret = std::make_shared<script::parser::ParserContext>(source);
  return ret;
}

TEST(ParserTests, identifier1) {
  using namespace script;
  using namespace parser;
  using namespace ast;

  const char *source =
    "foo qux::bar foo<4> qux::foo<4+4> foo<4,5> qux::bar::foo foo<bar,qux<foo>> foo<int>::n";

  auto c = parser_context(source);
  IdentifierParser parser{ c, Fragment(*c) };

  auto id = parser.parse();
  ASSERT_TRUE(id->is<ast::Identifier>());

  id = parser.parse();
  ASSERT_TRUE(id->is<ast::ScopedIdentifier>());

  id = parser.parse();
  ASSERT_TRUE(id->is<ast::TemplateIdentifier>());

  id = parser.parse();
  ASSERT_TRUE(id->is<ast::ScopedIdentifier>());
  {
    const auto & qualid = id->as<ast::ScopedIdentifier>();
    ASSERT_TRUE(qualid.lhs->is<ast::Identifier>());
    ASSERT_TRUE(qualid.rhs->is<ast::TemplateIdentifier>());
    const auto & tid = qualid.rhs->as<ast::TemplateIdentifier>();
    ASSERT_TRUE(tid.arguments.size() == 1);
    ASSERT_TRUE(tid.arguments.front()->is<ast::Operation>());
  }

  id = parser.parse();
  ASSERT_TRUE(id->is<ast::TemplateIdentifier>());
  {
    const auto & tid = id->as<ast::TemplateIdentifier>();
    ASSERT_TRUE(tid.arguments.size() == 2);
  }

  id = parser.parse();
  ASSERT_TRUE(id->is<ast::ScopedIdentifier>());
  {
    const auto & qualid = id->as<ast::ScopedIdentifier>();
    ASSERT_TRUE(qualid.lhs->is<ast::ScopedIdentifier>());
    ASSERT_TRUE(qualid.rhs->is<ast::Identifier>());
  }

  id = parser.parse();
  ASSERT_TRUE(id->is<ast::TemplateIdentifier>());
  {
    const auto & tid = id->as<ast::TemplateIdentifier>();
    ASSERT_TRUE(tid.arguments.size() == 2);
    ASSERT_TRUE(tid.arguments.at(1)->is<ast::TypeNode>());
    ASSERT_TRUE(tid.arguments.at(1)->as<ast::TypeNode>().value.type->is<ast::TemplateIdentifier>());
  }

  id = parser.parse();
  ASSERT_TRUE(id->is<ast::ScopedIdentifier>());
  {
    const auto & scpid = id->as<ast::ScopedIdentifier>();
    ASSERT_TRUE(scpid.rhs->type() == ast::NodeType::SimpleIdentifier);
    ASSERT_TRUE(scpid.lhs->is<ast::TemplateIdentifier>());
    {
      const auto & tid = scpid.lhs->as<ast::TemplateIdentifier>();
      ASSERT_TRUE(tid.arguments.size() == 1);
      ASSERT_TRUE(tid.arguments.at(0)->is<ast::TypeNode>());
    }
  }

  ASSERT_TRUE(parser.atEnd());
}

TEST(ParserTests, identifier2) {
  using namespace script;
  using namespace parser;
  using namespace ast;

  const char *source =
    "int<bool>";

  auto c = parser_context(source);
  IdentifierParser parser{ c, Fragment(*c) };

  auto id = parser.parse();
  ASSERT_TRUE(id->is<ast::Identifier>());
  ASSERT_FALSE(id->is<ast::TemplateIdentifier>());
}

TEST(ParserTests, function_types) {
  using namespace script;
  using namespace parser;
  using namespace ast;

  const char *source =
    " void(const int) | "
    " const int(int & ) | "
    " int(int) const  |"
    " int(int) const & | ";

  auto c = parser_context(source);
  TypeParser tp{ c, Fragment(*c) };

  auto t = tp.parse();
  ASSERT_TRUE(t.functionType != nullptr);
  ASSERT_EQ(t.functionType->params.size(), 1);
  ASSERT_TRUE(t.functionType->params.at(0).constQualifier.isValid());

  tp.reset(c, tp.fragment().mid(tp.iterator() + 1));

  t = tp.parse();
  ASSERT_TRUE(t.functionType != nullptr);
  ASSERT_TRUE(t.functionType->returnType.constQualifier.isValid());
  ASSERT_EQ(t.functionType->params.size(), 1);
  ASSERT_TRUE(t.functionType->params.at(0).reference.isValid());

  tp.reset(c, tp.fragment().mid(tp.iterator() + 1));

  t = tp.parse();
  ASSERT_TRUE(t.functionType != nullptr);
  ASSERT_FALSE(t.functionType->returnType.constQualifier.isValid());
  ASSERT_TRUE(t.constQualifier.isValid());
  ASSERT_FALSE(t.reference.isValid());
  ASSERT_EQ(t.functionType->params.size(), 1);

  tp.reset(c, tp.fragment().mid(tp.iterator() + 1));

  t = tp.parse();
  ASSERT_TRUE(t.functionType != nullptr);
  ASSERT_FALSE(t.functionType->returnType.constQualifier.isValid());
  ASSERT_TRUE(t.constQualifier.isValid());
  ASSERT_TRUE(t.reference.isValid());
  ASSERT_EQ(t.functionType->params.size(), 1);
}

TEST(ParserTests, expr1) {
  using namespace script;
  using namespace parser;
  using namespace ast;

  const char *source =
    " 3 * 4 + 5 ";

  auto c = parser_context(source);
  ExpressionParser parser{ c, Fragment(*c) };

  auto expr = parser.parse();
  ASSERT_TRUE(expr->is<Operation>());
  const Operation & op = expr->as<Operation>();
  ASSERT_EQ(op.operatorToken, Token::Plus);
  ASSERT_TRUE(op.arg1->is<Operation>());
  {
    const Operation & lhs = op.arg1->as<Operation>();
    ASSERT_EQ(lhs.operatorToken, Token::Mul);
    ASSERT_TRUE(lhs.arg1->is<IntegerLiteral>());
    ASSERT_TRUE(lhs.arg2->is<IntegerLiteral>());

  }
  ASSERT_TRUE(op.arg2->is<IntegerLiteral>());
}


TEST(ParserTests, operations) {
  using namespace script;
  using namespace parser;
  using namespace ast;

  {
    const char* source =
      "a < b + 3;";

    auto c = parser_context(source);
    Fragment fragment{ *c };
    Fragment sentinel{ fragment, Fragment::Type<Fragment::Statement>() };
    ExpressionParser parser{ c, sentinel };

    // input will first produce a hard failure to read identifier...
    auto expr = parser.parse();
    ASSERT_TRUE(expr->is<Operation>());
    {
      const Operation& op = expr->as<Operation>();
      ASSERT_EQ(op.operatorToken, Token::Less);
      ASSERT_TRUE(op.arg1->is<Identifier>());
      ASSERT_TRUE(op.arg2->is<Operation>());
    }
  }

  {
    const char* source =
      "a < b && d > c;";

    auto c = parser_context(source);
    Fragment fragment{ *c };
    Fragment sentinel{ fragment, Fragment::Type<Fragment::Statement>() };
    ExpressionParser parser{ c, sentinel };

    // input will be first parsed as template identifier than identifier
    auto expr = parser.parse();
    ASSERT_TRUE(expr->is<Operation>());
    {
      const Operation& op = expr->as<Operation>();
      ASSERT_EQ(op.operatorToken, Token::LogicalAnd);
      ASSERT_TRUE(op.arg1->is<Operation>());
      ASSERT_TRUE(op.arg2->is<Operation>());
    }
  }
}


TEST(ParserTests, expr2) {
  using namespace script;
  using namespace parser;
  using namespace ast;

  {
    const char* source =
      " f(a, b, c); ";

    auto c = parser_context(source);
    Fragment fragment{ *c };
    Fragment sentinel{ fragment, Fragment::Type<Fragment::Statement>() };
    ExpressionParser parser{ c, sentinel };

    auto expr = parser.parse();
    ASSERT_TRUE(expr->is<ast::FunctionCall>());
    {
      const auto& fcall = expr->as<ast::FunctionCall>();
      ASSERT_EQ(fcall.arguments.size(), 3);
    }
  }

  {
    const char* source =
      " a.b(); ";

    auto c = parser_context(source);
    Fragment fragment{ *c };
    Fragment sentinel{ fragment, Fragment::Type<Fragment::Statement>() };
    ExpressionParser parser{ c, sentinel };

    auto expr = parser.parse();
    ASSERT_TRUE(expr->is<ast::FunctionCall>());
    {
      const auto& fcall = expr->as<ast::FunctionCall>();
      ASSERT_EQ(fcall.arguments.size(), 0);
      ASSERT_TRUE(fcall.callee->is<Operation>());
    }
  }

  {
    const char* source =
      " (a+b)(c); ";

    auto c = parser_context(source);
    Fragment fragment{ *c };
    Fragment sentinel{ fragment, Fragment::Type<Fragment::Statement>() };
    ExpressionParser parser{ c, sentinel };

    auto expr = parser.parse();
    ASSERT_TRUE(expr->is<ast::FunctionCall>());
    {
      const auto& fcall = expr->as<ast::FunctionCall>();
      ASSERT_EQ(fcall.arguments.size(), 1);
      ASSERT_TRUE(fcall.callee->is<Operation>());
    }
  }
}


TEST(ParserTests, nested_calls) {
  using namespace script;
  using namespace ast;

  const char *source =
    " f(a, g(y(x))); ";

  parser::Parser parser{ script::SourceFile::fromString(source) };

  auto actual = parser.parseStatement();

  ASSERT_TRUE(actual->is<ast::ExpressionStatement>());
  auto expr = actual->as<ast::ExpressionStatement>().expression;
  ASSERT_TRUE(expr->is<ast::FunctionCall>());
  auto & call = expr->as<ast::FunctionCall>();
  ASSERT_EQ(call.arguments.size(), 2);
  auto arg = call.arguments.back();
  ASSERT_TRUE(arg->is<ast::FunctionCall>());
  {
    auto & nested_call = arg->as<ast::FunctionCall>();
    ASSERT_EQ(nested_call.arguments.size(), 1);
    ASSERT_TRUE(nested_call.arguments.front()->is<ast::FunctionCall>());
  }
}


TEST(ParserTests, arraysubscript) {
  using namespace script;
  using namespace parser;
  using namespace ast;

  const char *source =
    " array[index] ";

  auto c = parser_context(source);
  Fragment fragment{ *c };
  ExpressionParser parser{ c, fragment };

  auto expr = parser.parse();

  ASSERT_TRUE(expr->is<ast::ArraySubscript>());
  {
    const auto & asub = expr->as<ast::ArraySubscript>();
    ASSERT_TRUE(asub.array->is<ast::Identifier>());
    ASSERT_TRUE(asub.index->is<ast::Identifier>());
  }
}


TEST(ParserTests, arrays) {
  using script::parser::ExpressionParser;
  using namespace script::ast;

  const char *source = "[1, 2, 3, 4]";

  auto c = parser_context(source);
  script::parser::Fragment fragment{ *c };
  ExpressionParser parser{ c, fragment };

  auto actual = parser.parse();
  ASSERT_TRUE(actual->type() == NodeType::ArrayExpression);
  const auto & ae = actual->as<ArrayExpression>();
  ASSERT_EQ(ae.elements.size(), 4);
  for (size_t i(0); i < ae.elements.size(); ++i)
  {
    ASSERT_TRUE(ae.elements.at(i)->type() == NodeType::IntegerLiteral);
  }
}

TEST(ParserTests, lambdas) {
  using script::parser::ExpressionParser;
  using namespace script::ast;

  const char *source = "[x] () { }";

  auto c = parser_context(source);
  script::parser::Fragment fragment{ *c };
  ExpressionParser parser{ c, fragment };

  auto actual = parser.parse();

  ASSERT_TRUE(actual->type() == NodeType::LambdaExpression);
  const LambdaExpression & le = actual->as<LambdaExpression>();
  ASSERT_EQ(le.captures.size(), 1);
  ASSERT_EQ(le.captures.front().name, script::parser::Token::UserDefinedName);
  ASSERT_FALSE(le.captures.front().assignmentSign.isValid());
  ASSERT_FALSE(le.captures.front().reference.isValid());
  ASSERT_FALSE(le.captures.front().byValueSign.isValid());
}

TEST(ParserTests, vardecl1) {
  using namespace script;
  using namespace parser;
  using namespace ast;

  const char *source =
    " int a = 5; ";

  auto c = parser_context(source);
  Fragment fragment{ *c };
  DeclParser parser{ c, fragment };

  ASSERT_TRUE(parser.detectDecl());
  auto vardecl = parser.parse();

  ASSERT_TRUE(vardecl->type() == NodeType::VariableDeclaration);
  const VariableDecl & decl = vardecl->as<VariableDecl>();

  ASSERT_TRUE(decl.variable_type.type->as<ast::SimpleIdentifier>().name == Token::Int);
  ASSERT_TRUE(decl.init->is<AssignmentInitialization>());

  const AssignmentInitialization & init = decl.init->as<AssignmentInitialization>();
  ASSERT_TRUE(init.value->is<IntegerLiteral>());
}

TEST(ParserTests, fundecl1) {
  using namespace script;
  using namespace parser;
  using namespace ast;

  const char *source =
    " int foo(int a, int b) { return a + b; } ";

  auto c = parser_context(source);
  Fragment fragment{ *c };
  DeclParser parser{ c, fragment };

  ASSERT_TRUE(parser.detectDecl());
  auto decl = parser.parse();

  ASSERT_TRUE(decl->is<ast::FunctionDecl>());
  const FunctionDecl & fdecl = decl->as<FunctionDecl>();

  ASSERT_EQ(fdecl.params.size(), 2);

  ASSERT_EQ(fdecl.body->statements.size(), 1);
  ASSERT_TRUE(fdecl.body->statements.front()->is<ReturnStatement>());
}

TEST(ParserTests, fundecl2) {
  using namespace script;
  using namespace parser;
  using namespace ast;

  const char *source =
    " bar foo(qux, qux) { } ";

  auto c = parser_context(source);
  Fragment fragment{ *c };
  DeclParser parser{ c, fragment };

  ASSERT_TRUE(parser.detectDecl());
  auto decl = parser.parse();

  ASSERT_TRUE(decl->is<ast::FunctionDecl>());
}


TEST(ParserTests, vardecl2) {
  using namespace script;
  using namespace parser;
  using namespace ast;

  const char *source =
    " bar foo(qux, qux);";

  auto c = parser_context(source);
  Fragment fragment{ *c };
  DeclParser parser{ c, fragment };

  ASSERT_TRUE(parser.detectDecl());
  auto decl = parser.parse();

  ASSERT_TRUE(decl->is<ast::VariableDecl>());
}

TEST(ParserTests, empty_enum) {
  using namespace script;
  using namespace parser;
  using namespace ast;

  const char *source = "enum Foo{};";

  auto c = parser_context(source);
  Fragment fragment{ *c };
  EnumParser parser{ c, fragment };

  auto actual = parser.parse();

  ASSERT_TRUE(actual->is<EnumDeclaration>());
}

TEST(ParserTests, empty_enum_class) {
  using namespace script;
  using namespace parser;
  using namespace ast;

  const char *source = "enum class Foo{};";

  auto c = parser_context(source);
  Fragment fragment{ *c };
  EnumParser parser{ c, fragment };

  auto actual = parser.parse();

  ASSERT_TRUE(actual->is<EnumDeclaration>());
  const EnumDeclaration & ed = actual->as<EnumDeclaration>();
  ASSERT_TRUE(ed.classKeyword.isValid());
}

TEST(ParserTests, enum_with_values) {
  using namespace script;
  using namespace parser;
  using namespace ast;

  const char *source = "enum Foo{Field1, Field2};";

  auto c = parser_context(source);
  Fragment fragment{ *c };
  EnumParser parser{ c, fragment };

  auto actual = parser.parse();

  ASSERT_TRUE(actual->is<EnumDeclaration>());
  const EnumDeclaration & ed = actual->as<EnumDeclaration>();
  ASSERT_EQ(ed.values.size(), 2);
}


TEST(ParserTests, enum_with_assigned_value) {
  using namespace script;
  using namespace parser;
  using namespace ast;

  const char *source = "enum Foo{Field1 = 1, Field2};";

  auto c = parser_context(source);
  Fragment fragment{ *c };
  EnumParser parser{ c, fragment };

  auto actual = parser.parse();

  ASSERT_TRUE(actual->is<EnumDeclaration>());
  const EnumDeclaration & ed = actual->as<EnumDeclaration>();
  ASSERT_EQ(ed.values.size(), 2);
  ASSERT_TRUE(ed.values.front().value->is<IntegerLiteral>());
  ASSERT_TRUE(ed.values.back().value == nullptr);
}


TEST(ParserTests, enum_empty_field) {
  using namespace script;
  using namespace parser;
  using namespace ast;

  const char *source = "enum Foo{Field1, Field2, , Field3, };";

  auto c = parser_context(source);
  Fragment fragment{ *c };
  EnumParser parser{ c, fragment };

  auto actual = parser.parse();

  ASSERT_TRUE(actual->is<EnumDeclaration>());
  const EnumDeclaration & ed = actual->as<EnumDeclaration>();
  ASSERT_EQ(ed.values.size(), 3);
}

TEST(ParserTests, continue_break_return) {
  using namespace script;
  using namespace parser;
  using namespace ast;

  const char *source = "continue; break; return;";

  auto c = parser_context(source);
  Fragment fragment{ *c };
  ProgramParser parser{ c, fragment };

  auto actual = parser.parseStatement();
  ASSERT_TRUE(actual->is<ContinueStatement>());
  actual = parser.parseStatement();
  ASSERT_TRUE(actual->is<BreakStatement>());
  actual = parser.parseStatement();
  ASSERT_TRUE(actual->is<ReturnStatement>());
}

TEST(ParserTests, if_statement) {
  using namespace script;
  using namespace parser;
  using namespace ast;

  const char *source = "if(i == 0) return;";

  auto c = parser_context(source);
  Fragment fragment{ *c };
  ProgramParser parser{ c, fragment };

  auto actual = parser.parseStatement();
  ASSERT_TRUE(actual->is<IfStatement>());
  const IfStatement & ifs = actual->as<IfStatement>();
  ASSERT_TRUE(ifs.condition->is<Operation>());
  ASSERT_TRUE(ifs.body->is<ReturnStatement>());
}


TEST(ParserTests, while_loop) {
  using namespace script;
  using namespace parser;
  using namespace ast;

  const char *source = "while(true) { }";

  auto c = parser_context(source);
  Fragment fragment{ *c };
  ProgramParser parser{ c, fragment };

  auto actual = parser.parseStatement();
  ASSERT_TRUE(actual->is<WhileLoop>());
  const WhileLoop & wl = actual->as<WhileLoop>();
  ASSERT_TRUE(wl.condition->is<BoolLiteral>());
  ASSERT_TRUE(wl.body->is<CompoundStatement>());
}

TEST(ParserTests, for_loop) {
  using namespace script;
  using namespace parser;
  using namespace ast;

  const char *source = "for(int i = 0; i < 10; ++i) { }";

  auto c = parser_context(source);
  Fragment fragment{ *c };
  ProgramParser parser{ c, fragment };

  auto actual = parser.parseStatement();
  ASSERT_TRUE(actual->is<ForLoop>());
  const ForLoop & fl = actual->as<ForLoop>();
  ASSERT_TRUE(fl.initStatement->is<VariableDecl>());
  ASSERT_TRUE(fl.condition->is<Operation>());
  ASSERT_TRUE(fl.loopIncrement->is<Operation>());
  ASSERT_TRUE(fl.body->is<CompoundStatement>());
}

TEST(ParserTests, compound_statement) {
  using namespace script;
  using namespace parser;
  using namespace ast;

  const char *source = "{ continue; break; }";

  auto c = parser_context(source);
  Fragment fragment{ *c };
  ProgramParser parser{ c, fragment };

  auto actual = parser.parseStatement();
  ASSERT_TRUE(actual->is<CompoundStatement>());
  const CompoundStatement & cs = actual->as<CompoundStatement>();
  ASSERT_EQ(cs.statements.size(), 2);
}

TEST(ParserTests, var_decl_initializations) {
  using namespace script;
  using namespace parser;
  using namespace ast;

  const char *source = "int a = 5; int a(5); int a{5};";

  auto c = parser_context(source);
  Fragment fragment{ *c };
  ProgramParser parser{ c, fragment };

  auto actual = parser.parseStatement();
  ASSERT_TRUE(actual->is<VariableDecl>());
  ASSERT_TRUE(actual->as<VariableDecl>().init->is<AssignmentInitialization>());

  actual = parser.parseStatement();
  ASSERT_TRUE(actual->is<VariableDecl>());
  ASSERT_TRUE(actual->as<VariableDecl>().init->is<ConstructorInitialization>());

  actual = parser.parseStatement();
  ASSERT_TRUE(actual->is<VariableDecl>());
  ASSERT_TRUE(actual->as<VariableDecl>().init->is<BraceInitialization>());
}

TEST(ParserTests, class_decls_1) {
  using namespace script;
  using namespace parser;
  using namespace ast;

  const char *source = 
    " class A { }; "
    " class A { int a; }; "
    " class A { int a() { } }; "
    " class A  { public: int a; } ; "
    " class A  { A() { } } ; ";

  auto src_file = SourceFile::fromString(source);
  parser::Parser parser{ src_file };

  auto actual = parser.parseStatement();
  ASSERT_TRUE(actual->is<ClassDecl>());

  actual = parser.parseStatement();
  ASSERT_TRUE(actual->is<ClassDecl>());
  {
    const ClassDecl & cd = actual->as<ClassDecl>();
    ASSERT_EQ(cd.content.size(), 1);
    ASSERT_TRUE(cd.content.front()->is<VariableDecl>());
  }

  actual = parser.parseStatement();
  ASSERT_TRUE(actual->is<ClassDecl>());
  {
    const ClassDecl & cd = actual->as<ClassDecl>();
    ASSERT_EQ(cd.content.size(), 1);
    ASSERT_TRUE(cd.content.front()->is<FunctionDecl>());
  }

  actual = parser.parseStatement();
  ASSERT_TRUE(actual->is<ClassDecl>());
  {
    const ClassDecl & cd = actual->as<ClassDecl>();
    ASSERT_EQ(cd.content.size(), 2);
    ASSERT_TRUE(cd.content.front()->is<ast::AccessSpecifier>());
    ASSERT_TRUE(cd.content.back()->is<VariableDecl>());
  }

  actual = parser.parseStatement();
  ASSERT_TRUE(actual->is<ClassDecl>());
  {
    const ClassDecl & cd = actual->as<ClassDecl>();
    ASSERT_EQ(cd.content.size(), 1);
    ASSERT_TRUE(cd.content.front()->is<ConstructorDecl>());
  }
}

TEST(ParserTests, class_decls_2) {
  using namespace script;
  using namespace parser;
  using namespace ast;

  const char *source =
    " class A  { A() : b(0), c{0} { } } ; "
    " class A  { ~A() { } } ; "
    " class A  { operator int () { } } ; "
    " class A  { A & operator=(const A & other) { } } ; ";

  auto src_file = SourceFile::fromString(source);
  parser::Parser parser{ src_file };

  auto actual = parser.parseStatement();
  ASSERT_TRUE(actual->is<ClassDecl>());
  {
    const ClassDecl & cd = actual->as<ClassDecl>();
    ASSERT_EQ(cd.content.size(), 1);
    ASSERT_TRUE(cd.content.front()->is<ConstructorDecl>());
    const ConstructorDecl & ctor = cd.content.front()->as<ConstructorDecl>();
    ASSERT_EQ(ctor.memberInitializationList.size(), 2);
    ASSERT_TRUE(ctor.memberInitializationList.front().init->is<ConstructorInitialization>());
    ASSERT_TRUE(ctor.memberInitializationList.back().init->is<BraceInitialization>());
  }

  actual = parser.parseStatement();
  ASSERT_TRUE(actual->is<ClassDecl>());
  {
    const ClassDecl & cd = actual->as<ClassDecl>();
    ASSERT_EQ(cd.content.size(), 1);
    ASSERT_TRUE(cd.content.front()->is<DestructorDecl>());
  }

  actual = parser.parseStatement();
  ASSERT_TRUE(actual->is<ClassDecl>());
  {
    const ClassDecl & cd = actual->as<ClassDecl>();
    ASSERT_EQ(cd.content.size(), 1);
    ASSERT_TRUE(cd.content.front()->is<CastDecl>());
    const CastDecl & cast = cd.content.front()->as<CastDecl>();
    ASSERT_TRUE(cast.returnType.type->as<ast::SimpleIdentifier>().name == Token::Int);
  }

  actual = parser.parseStatement();
  ASSERT_TRUE(actual->is<ClassDecl>());
  {
    const ClassDecl & cd = actual->as<ClassDecl>();
    ASSERT_EQ(cd.content.size(), 1);
    ASSERT_TRUE(cd.content.front()->is<OperatorOverloadDecl>());
  }
}


TEST(ParserTests, lambda) {

  const char *source = "auto f = [](){};";

  auto src_file = script::SourceFile::fromString(source);
  script::parser::Parser parser{ src_file };

  auto actual = parser.parseStatement();

  using namespace script;
  using namespace ast;
  ASSERT_TRUE(actual->type() == NodeType::VariableDeclaration);
  const VariableDecl & decl = actual->as<ast::VariableDecl>();
  
  {
    ASSERT_TRUE(decl.init->is<ast::AssignmentInitialization>());
    const AssignmentInitialization & init = decl.init->as<ast::AssignmentInitialization>();

    ASSERT_TRUE(init.value->is<ast::LambdaExpression>());

    const LambdaExpression & lambda = init.value->as<ast::LambdaExpression>();
    ASSERT_TRUE(lambda.captures.empty());
    ASSERT_TRUE(lambda.params.empty());

  }
}


TEST(ParserTests, user_defined_literal) {

  const char *source = "Distance operator\"\"km(double x) { }";

  auto src_file = script::SourceFile::fromString(source);
  script::parser::Parser parser{ src_file };

  auto actual = parser.parseStatement();

  using namespace script;
  using namespace ast;
  ASSERT_TRUE(actual->type() == NodeType::OperatorOverloadDeclaration);
  const OperatorOverloadDecl & decl = actual->as<ast::OperatorOverloadDecl>();

  {
    ASSERT_TRUE(decl.name->is<ast::LiteralOperatorName>());
    ASSERT_EQ(decl.name->as<ast::LiteralOperatorName>().suffix_string(), std::string("km"));
  }
}

TEST(ParserTests, typedefs) {

  const char *source = 
    "typedef double Distance;"
    "typedef const double RealConstant;"
    "typedef double& DoubleRef;"
    "typedef Array<int> AInt;";

  using namespace script;
  
  auto src_file = SourceFile::fromString(source);
  parser::Parser parser{ src_file };

  auto actual = parser.parseStatement();
  ASSERT_TRUE(actual->type() == ast::NodeType::Typedef);

  actual = parser.parseStatement();
  ASSERT_TRUE(actual->type() == ast::NodeType::Typedef);
  {
    const auto & tdef = actual->as<ast::Typedef>();
    ASSERT_TRUE(tdef.qualified_type.constQualifier.isValid());
    ASSERT_EQ(tdef.name->getName(), "RealConstant");
  }

  actual = parser.parseStatement();
  ASSERT_TRUE(actual->type() == ast::NodeType::Typedef);
  {
    const auto & tdef = actual->as<ast::Typedef>();
    ASSERT_FALSE(tdef.qualified_type.constQualifier.isValid());
    ASSERT_TRUE(tdef.qualified_type.reference.isValid());
    ASSERT_EQ(tdef.name->getName(), "DoubleRef");
  }

  actual = parser.parseStatement();
  ASSERT_TRUE(actual->type() == ast::NodeType::Typedef);
  {
    const auto & tdef = actual->as<ast::Typedef>();
    ASSERT_FALSE(tdef.qualified_type.constQualifier.isValid());
    ASSERT_TRUE(tdef.qualified_type.type->is<ast::TemplateIdentifier>());
    ASSERT_EQ(tdef.qualified_type.type->as<ast::TemplateIdentifier>().getName(), "Array");
    ASSERT_EQ(tdef.name->getName(), "AInt");
  }
}

TEST(ParserTests, namespace_decl) {

  const char *source =
    "  namespace ns {         "
    "    int a;               "
    "    int foo() { }        "
    "    namespace bar { }    "
    "  }                      ";

  using namespace script;

  auto src_file = SourceFile::fromString(source);
  parser::Parser parser{ src_file };

  auto actual = parser.parseStatement();
  ASSERT_TRUE(actual->type() == ast::NodeType::NamespaceDecl);

  const auto & ndecl = actual->as<ast::NamespaceDeclaration>();
  ASSERT_EQ(ndecl.statements.size(), 3);

  ASSERT_EQ(ndecl.statements.front()->type(), ast::NodeType::VariableDeclaration);
  ASSERT_EQ(ndecl.statements.at(1)->type(), ast::NodeType::FunctionDeclaration);
  ASSERT_EQ(ndecl.statements.back()->type(), ast::NodeType::NamespaceDecl);
}

TEST(ParserTests, illegal_class_friend_decl) {

  const char *source =
    " friend class A; ";

  using namespace script;

  auto src_file = SourceFile::fromString(source);
  parser::Parser parser{ src_file };
  ASSERT_THROW(parser.parseStatement(), parser::SyntaxError);
}

TEST(ParserTests, class_friend_decl) {

  const char *source =
    " class A { friend class B; }; ";

  using namespace script;

  auto src_file = SourceFile::fromString(source);
  parser::Parser parser{ src_file };

  auto actual = parser.parseStatement();

  ASSERT_EQ(actual->type(), ast::NodeType::ClassDeclaration);
  {
    const auto & class_decl = actual->as<ast::ClassDecl>();
    ASSERT_EQ(class_decl.content.size(), 1);
    ASSERT_EQ(class_decl.content.front()->type(), ast::NodeType::ClassFriendDecl);
    {
      const auto & fdecl = class_decl.content.front()->as<ast::ClassFriendDeclaration>();
      ASSERT_EQ(fdecl.class_name->as<ast::SimpleIdentifier>().getName(), "B");
    }
  }
}

TEST(ParserTests, using_parser) {

  const char *source =
    " using namespace bar; "
    " using bar::foo; "
    " using alias = nested::name; ";

  using namespace script;

  auto src_file = SourceFile::fromString(source);
  parser::Parser parser{ src_file };

  auto actual = parser.parseStatement();

  ASSERT_EQ(actual->type(), ast::NodeType::UsingDirective);
  {
    const auto & ud = actual->as<ast::UsingDirective>();
    ASSERT_EQ(ud.namespace_name->as<ast::SimpleIdentifier>().getName(), "bar");
  }

  actual = parser.parseStatement();
  ASSERT_EQ(actual->type(), ast::NodeType::UsingDeclaration);
  {
    const auto & ud = actual->as<ast::UsingDeclaration>();
    ASSERT_TRUE(ud.used_name->is<ast::ScopedIdentifier>());
    const auto & scpid = ud.used_name->as<ast::ScopedIdentifier>();
    ASSERT_EQ(scpid.lhs->as<ast::SimpleIdentifier>().getName(), "bar");
    ASSERT_EQ(scpid.rhs->as<ast::SimpleIdentifier>().getName(), "foo");
  }

  actual = parser.parseStatement();
  ASSERT_EQ(actual->type(), ast::NodeType::TypeAliasDecl);
  {
    const auto & tad = actual->as<ast::TypeAliasDeclaration>();
    ASSERT_EQ(tad.alias_name->getName(), "alias");
    ASSERT_TRUE(tad.aliased_type->is<ast::ScopedIdentifier>());
  }
}

TEST(ParserTests, namespace_alias) {

  const char *source =
    " namespace fs = std::experimental::filesystem; ";

  using namespace script;

  auto src_file = SourceFile::fromString(source);
  parser::Parser parser{ src_file };

  auto actual = parser.parseStatement();
  ASSERT_EQ(actual->type(), ast::NodeType::NamespaceAliasDef);
  {
    const auto & ns_alias = actual->as<ast::NamespaceAliasDefinition>();
    ASSERT_EQ(ns_alias.alias_name->getName(), "fs");

    ASSERT_TRUE(ns_alias.aliased_namespace->is<ast::ScopedIdentifier>());
    const auto & scpid = ns_alias.aliased_namespace->as<ast::ScopedIdentifier>();
    ASSERT_TRUE(scpid.lhs->is<ast::ScopedIdentifier>());
    ASSERT_EQ(scpid.rhs->as<ast::SimpleIdentifier>().getName(), "filesystem");
  }
}

TEST(ParserTests, import_directives) {

  const char *source =
    " import foo.bar;    "
    " export import qux; ";

  using namespace script;

  auto src_file = SourceFile::fromString(source);
  parser::Parser parser{ src_file };

  auto actual = parser.parseStatement();
  ASSERT_EQ(actual->type(), ast::NodeType::ImportDirective);
  {
    const auto & ipd = actual->as<ast::ImportDirective>();
    ASSERT_FALSE(ipd.export_keyword.isValid());
    ASSERT_TRUE(ipd.import_keyword.isValid());
    ASSERT_EQ(ipd.size(), 2);
    ASSERT_EQ(ipd.at(0), "foo");
    ASSERT_EQ(ipd.at(1), "bar");
  }

  actual = parser.parseStatement();
  ASSERT_EQ(actual->type(), ast::NodeType::ImportDirective);
  {
    const auto & ipd = actual->as<ast::ImportDirective>();
    ASSERT_TRUE(ipd.export_keyword.isValid());
    ASSERT_TRUE(ipd.import_keyword.isValid());
    ASSERT_EQ(ipd.size(), 1);
    ASSERT_EQ(ipd.at(0), "qux");
  }
}

TEST(ParserTests, function_template) {

  const char *source =
    "  template<typename T>       "
    "  void swap(T & a, T & b)    "
    "  {                          "
    "    T temp = b;              "
    "    b = a;                   "
    "    a = temp;                "
    "  }                          "
    "                             "
    "  template<int I = 1>        "
    "  int incr(int n)            "
    "  {                          "
    "    return n +  I;           "
    "  }                          "
    "                             "
    "  template<bool B>           "
    "  bool logical_or(bool a)    "
    "  {                          "
    "    return a || B;           "
    "  }                          ";

  using namespace script;

  auto src_file = SourceFile::fromString(source);
  parser::Parser parser{ src_file };

  auto actual = parser.parseStatement();
  ASSERT_EQ(actual->type(), ast::NodeType::TemplateDecl);
  {
    const auto & td = actual->as<ast::TemplateDeclaration>();
    ASSERT_EQ(td.size(), 1);
    ASSERT_EQ(td.at(0).kind, parser::Token::Typename);
    ASSERT_EQ(td.parameter_name(0), "T");
    ASSERT_EQ(td.declaration->type(), ast::NodeType::FunctionDeclaration);
  }

  actual = parser.parseStatement();
  ASSERT_EQ(actual->type(), ast::NodeType::TemplateDecl);
  {
    const auto & td = actual->as<ast::TemplateDeclaration>();
    ASSERT_EQ(td.size(), 1);
    ASSERT_EQ(td.at(0).kind, parser::Token::Int);
    ASSERT_EQ(td.parameter_name(0), "I");
    ASSERT_EQ(td.declaration->type(), ast::NodeType::FunctionDeclaration);
  }

  actual = parser.parseStatement();
  ASSERT_EQ(actual->type(), ast::NodeType::TemplateDecl);
  {
    const auto & td = actual->as<ast::TemplateDeclaration>();
    ASSERT_EQ(td.size(), 1);
    ASSERT_EQ(td.at(0).kind, parser::Token::Bool);
    ASSERT_EQ(td.parameter_name(0), "B");
    ASSERT_EQ(td.declaration->type(), ast::NodeType::FunctionDeclaration);
  }
}

TEST(ParserTests, class_template) {

  const char *source =
    "  template<typename T, typename Allocator = std::allocator<T>>  "
    "  class vector { };                                              ";

  using namespace script;

  auto src_file = SourceFile::fromString(source);
  parser::Parser parser{ src_file };

  auto actual = parser.parseStatement();
  ASSERT_EQ(actual->type(), ast::NodeType::TemplateDecl);
  {
    const auto & td = actual->as<ast::TemplateDeclaration>();
    ASSERT_EQ(td.size(), 2);
    ASSERT_EQ(td.at(0).kind, parser::Token::Typename);
    ASSERT_EQ(td.parameter_name(0), "T");
    ASSERT_EQ(td.at(1).kind, parser::Token::Typename);
    ASSERT_EQ(td.parameter_name(1), "Allocator");
    ASSERT_TRUE(td.at(1).default_value != nullptr);
    ASSERT_EQ(td.at(1).default_value->type(), ast::NodeType::QualifiedType);
    ASSERT_EQ(td.declaration->type(), ast::NodeType::ClassDeclaration);
  }
}

TEST(ParserTests, template_specialization) {

  const char *source =
    "  template<>                              "
    "  int incr<1>(int a) { return a + 1; }    "

    "  template<>                              "
    "  class vector<bool> { };                 "

    "  template<typename T>                    "
    "  class pair<T, T> { };                   ";

  using namespace script;

  auto src_file = SourceFile::fromString(source);
  parser::Parser parser{ src_file };

  auto actual = parser.parseStatement();
  ASSERT_EQ(actual->type(), ast::NodeType::TemplateDecl);
  {
    const auto & td = actual->as<ast::TemplateDeclaration>();
    ASSERT_TRUE(td.is_full_specialization());
    ASSERT_FALSE(td.is_partial_specialization());
  }

  actual = parser.parseStatement();
  ASSERT_EQ(actual->type(), ast::NodeType::TemplateDecl);
  {
    const auto & td = actual->as<ast::TemplateDeclaration>();
    ASSERT_TRUE(td.is_full_specialization());
    ASSERT_FALSE(td.is_partial_specialization());
  }

  actual = parser.parseStatement();
  ASSERT_EQ(actual->type(), ast::NodeType::TemplateDecl);
  {
    const auto & td = actual->as<ast::TemplateDeclaration>();
    ASSERT_FALSE(td.is_full_specialization());
    ASSERT_TRUE(td.is_partial_specialization());
  }
}