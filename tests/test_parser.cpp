// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include <gtest/gtest.h>

#include "script/parser/lexer.h"
#include "script/parser/token.h"
#include "script/parser/parser.h"
#include "script/parser/parsererrors.h"

#include "script/ast/node.h"
#include "script/ast/ast.h"


std::shared_ptr<script::parser::ParserData> parser_data(const char *source)
{
  return std::make_shared<script::parser::ParserData>(script::SourceFile::fromString(std::string(source)));
}

TEST(ParserTests, identifier1) {
  using namespace script;
  using namespace parser;
  using namespace ast;

  const char *source =
    "foo qux::bar foo<4> qux::foo<4+4> foo<4,5> qux::bar::foo foo<bar,qux<foo>>";

  ScriptFragment fragment{ parser_data(source) };
  IdentifierParser parser{ &fragment };

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


  ASSERT_TRUE(fragment.atEnd());
}

TEST(ParserTests, identifier2) {
  using namespace script;
  using namespace parser;
  using namespace ast;

  const char *source =
    "int<bool>";

  ScriptFragment fragment{ parser_data(source) };
  IdentifierParser parser{ &fragment };

  auto id = parser.parse();
  ASSERT_TRUE(id->is<ast::Identifier>());
  ASSERT_FALSE(id->is<ast::TemplateIdentifier>());

}

TEST(ParserTests, expr1) {
  using namespace script;
  using namespace parser;
  using namespace ast;

  const char *source =
    " 3 * 4 + 5 ";

  ScriptFragment fragment{ parser_data(source) };
  ExpressionParser parser{ &fragment };

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

  const char *source =
    "a < b + 3;"
    "a < b && d > c;";

  ScriptFragment fragment{ parser_data(source) };
  SentinelFragment sentinel{ Token::Semicolon, &fragment };
  ExpressionParser parser{ &sentinel };
  
  // input will first produce a hard failure to read identifier...
  auto expr = parser.parse();
  ASSERT_TRUE(expr->is<Operation>());
  {
    const Operation & op = expr->as<Operation>();
    ASSERT_EQ(op.operatorToken, Token::Less);
    ASSERT_TRUE(op.arg1->is<Identifier>());
    ASSERT_TRUE(op.arg2->is<Operation>());
  }

  sentinel.consumeSentinel();

  // input will be first parsed as template identifier than identifier
  expr = parser.parse();
  ASSERT_TRUE(expr->is<Operation>());
  {
    const Operation & op = expr->as<Operation>();
    ASSERT_EQ(op.operatorToken, Token::LogicalAnd);
    ASSERT_TRUE(op.arg1->is<Operation>());
    ASSERT_TRUE(op.arg2->is<Operation>());
  }
}






TEST(ParserTests, expr2) {
  using namespace script;
  using namespace parser;
  using namespace ast;

  const char *source =
    " f(a, b, c); "
    " a.b(); "
    " (a+b)(c); ";

  ScriptFragment fragment{ parser_data(source) };
  SentinelFragment sentinel{ Token::Semicolon, &fragment };
  ExpressionParser parser{ &sentinel };

  auto expr = parser.parse();
  ASSERT_TRUE(expr->is<ast::FunctionCall>());
  {
    const auto & fcall = expr->as<ast::FunctionCall>();
    ASSERT_EQ(fcall.arguments.size(), 3);
  }

  sentinel.consumeSentinel();
  expr = parser.parse();
  ASSERT_TRUE(expr->is<ast::FunctionCall>());
  {
    const auto & fcall = expr->as<ast::FunctionCall>();
    ASSERT_EQ(fcall.arguments.size(), 0);
    ASSERT_TRUE(fcall.callee->is<Operation>());
  }

  sentinel.consumeSentinel();
  expr = parser.parse();
  ASSERT_TRUE(expr->is<ast::FunctionCall>());
  {
    const auto & fcall = expr->as<ast::FunctionCall>();
    ASSERT_EQ(fcall.arguments.size(),1);
    ASSERT_TRUE(fcall.callee->is<Operation>());
  }
}

TEST(ParserTests, expr3) {
  using namespace script;
  using namespace parser;
  using namespace ast;

  const char *source =
    " array[index] ";

  ScriptFragment fragment{ parser_data(source) };
  ExpressionParser parser{ &fragment };

  auto expr = parser.parse();

  ASSERT_TRUE(expr->is<ast::ArraySubscript>());
  {
    const auto & asub = expr->as<ast::ArraySubscript>();
    ASSERT_TRUE(asub.array->is<ast::Identifier>());
    ASSERT_TRUE(asub.index->is<ast::Identifier>());
  }
}

TEST(ParserTests, arraysubscript) {
  using namespace script;
  using namespace parser;
  using namespace ast;

  const char *source =
    " array[index] ";

  ScriptFragment fragment{ parser_data(source) };
  ExpressionParser parser{ &fragment };

  auto expr = parser.parse();

  ASSERT_TRUE(expr->is<ast::ArraySubscript>());
  {
    const auto & asub = expr->as<ast::ArraySubscript>();
    ASSERT_TRUE(asub.array->is<ast::Identifier>());
    ASSERT_TRUE(asub.index->is<ast::Identifier>());
  }
}


TEST(ParserTests, arrays) {
  using script::parser::ScriptFragment;
  using script::parser::ExpressionParser;
  using namespace script::ast;

  const char *source = "[1, 2, 3, 4]";

  ScriptFragment fragment{ parser_data(source) };
  ExpressionParser parser{ &fragment };

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
  using script::parser::ScriptFragment;
  using script::parser::ExpressionParser;
  using namespace script::ast;

  const char *source = "[x] () { }";

  ScriptFragment fragment{ parser_data(source) };
  ExpressionParser parser{ &fragment };

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

  ScriptFragment fragment{ parser_data(source) };
  DeclParser parser{ &fragment };

  ASSERT_TRUE(parser.detectDecl());
  auto vardecl = parser.parse();

  ASSERT_TRUE(vardecl->type() == NodeType::VariableDeclaration);
  const VariableDecl & decl = vardecl->as<VariableDecl>();

  ASSERT_TRUE(decl.variable_type.type->name == Token::Int);
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

  ScriptFragment fragment{ parser_data(source) };
  DeclParser parser{ &fragment };

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

  ScriptFragment fragment{ parser_data(source) };
  DeclParser parser{ &fragment };

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

  ScriptFragment fragment{ parser_data(source) };
  DeclParser parser{ &fragment };

  ASSERT_TRUE(parser.detectDecl());
  auto decl = parser.parse();

  ASSERT_TRUE(decl->is<ast::VariableDecl>());
}

TEST(ParserTests, empty_enum) {
  using namespace script;
  using namespace parser;
  using namespace ast;

  const char *source = "enum Foo{};";

  ScriptFragment fragment{ parser_data(source) };
  EnumParser parser{ &fragment };

  auto actual = parser.parse();

  ASSERT_TRUE(actual->is<EnumDeclaration>());
}

TEST(ParserTests, empty_enum_class) {
  using namespace script;
  using namespace parser;
  using namespace ast;

  const char *source = "enum class Foo{};";

  ScriptFragment fragment{ parser_data(source) };
  EnumParser parser{ &fragment };

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

  ScriptFragment fragment{ parser_data(source) };
  EnumParser parser{ &fragment };

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

  ScriptFragment fragment{ parser_data(source) };
  EnumParser parser{ &fragment };

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

  ScriptFragment fragment{ parser_data(source) };
  EnumParser parser{ &fragment };

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

  ScriptFragment fragment{ parser_data(source) };
  ProgramParser parser{ &fragment };

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

  ScriptFragment fragment{ parser_data(source) };
  ProgramParser parser{ &fragment };

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

  ScriptFragment fragment{ parser_data(source) };
  ProgramParser parser{ &fragment };

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

  ScriptFragment fragment{ parser_data(source) };
  ProgramParser parser{ &fragment };

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

  ScriptFragment fragment{ parser_data(source) };
  ProgramParser parser{ &fragment };

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

  ScriptFragment fragment{ parser_data(source) };
  ProgramParser parser{ &fragment };

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

  Parser parser{ script::SourceFile::fromString(source) };

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
    ASSERT_TRUE(cd.content.front()->is<AccessSpecifier>());
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

  Parser parser{ script::SourceFile::fromString(source) };

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
    ASSERT_TRUE(cast.returnType.type->name == Token::Int);
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

  script::parser::Parser parser{ script::SourceFile::fromString(source) };

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

  script::parser::Parser parser{ script::SourceFile::fromString(source) };

  auto actual = parser.parseStatement();

  using namespace script;
  using namespace ast;
  ASSERT_TRUE(actual->type() == NodeType::OperatorOverloadDeclaration);
  const OperatorOverloadDecl & decl = actual->as<ast::OperatorOverloadDecl>();

  {
    ASSERT_TRUE(decl.name->is<ast::LiteralOperatorName>());
    ASSERT_EQ(decl.name->getName(), std::string("km"));
  }
}

TEST(ParserTests, typedefs) {

  const char *source = 
    "typedef double Distance;"
    "typedef const double RealConstant;"
    "typedef double& DoubleRef;"
    "typedef Array<int> AInt;";

  using namespace script;
  
  parser::Parser parser{ script::SourceFile::fromString(source) };

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
    ASSERT_EQ(tdef.qualified_type.type->getName(), "Array");
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

  parser::Parser parser{ script::SourceFile::fromString(source) };

  auto actual = parser.parseStatement();
  ASSERT_TRUE(actual->type() == ast::NodeType::NamespaceDecl);

  const auto & ndecl = actual->as<ast::NamespaceDeclaration>();
  ASSERT_EQ(ndecl.statements.size(), 3);

  ASSERT_EQ(ndecl.statements.front()->type(), ast::NodeType::VariableDeclaration);
  ASSERT_EQ(ndecl.statements.at(1)->type(), ast::NodeType::FunctionDeclaration);
  ASSERT_EQ(ndecl.statements.back()->type(), ast::NodeType::NamespaceDecl);
}
