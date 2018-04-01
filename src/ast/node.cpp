// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/ast/node.h"

#include "script/ast/ast.h"

#include <map>

namespace script
{

namespace ast
{

parser::Lexer::Position Operation::pos() const
{
  return parser::Lexer::position(this->operatorToken);
}

parser::Lexer::Position ConditionalExpression::pos() const
{
  return parser::Lexer::position(this->questionMark);
}

parser::Lexer::Position NullStatement::pos() const
{
  return parser::Lexer::position(this->semicolon);
}

parser::Lexer::Position ExpressionStatement::pos() const
{
  return this->expression->pos();
}

parser::Lexer::Position CompoundStatement::pos() const
{
  return parser::Lexer::position(this->openingBrace);
}

parser::Lexer::Position IfStatement::pos() const
{
  return parser::Lexer::position(this->keyword);
}

parser::Lexer::Position IterationStatement::pos() const
{
  return parser::Lexer::position(this->keyword);
}

parser::Lexer::Position JumpStatement::pos() const
{
  return parser::Lexer::position(this->keyword);
}


parser::Lexer::Position TypeNode::pos() const
{
  return value.type->pos();
}

parser::Lexer::Position FunctionDecl::pos() const
{
  return name->pos();
}


///////////////////////////////////////////////////////////////////////


std::string Literal::toString() const
{
  return this->ast.lock()->text(this->token);
}

std::string Identifier::getName() const
{
  auto ast = this->ast.lock();
  return ast->text(this->name);
}


Operator::BuiltInOperator OperatorName::getOperatorId(const parser::Token & tok, BuiltInOpResol options)
{
  using parser::Token;

  static std::map<Token::Type, Operator::BuiltInOperator> prefixOps = {
    { Token::PlusPlus, Operator::PreIncrementOperator },
  { Token::MinusMinus, Operator::PreDecrementOperator },
  { Token::LogicalNot, Operator::LogicalNotOperator },
  { Token::BitwiseNot, Operator::BitwiseNot },
  { Token::Plus, Operator::UnaryPlusOperator },
  { Token::Minus, Operator::UnaryMinusOperator },
  };

  static std::map<Token::Type, Operator::BuiltInOperator> postFixOps = {
    { Token::PlusPlus, Operator::PostIncrementOperator },
  { Token::MinusMinus, Operator::PostDecrementOperator },
  };

  static std::map<Token::Type, Operator::BuiltInOperator> infixOps = {
    { Token::ScopeResolution, Operator::ScopeResolutionOperator },
  { Token::Plus, Operator::AdditionOperator },
  { Token::Minus, Operator::SubstractionOperator },
  { Token::Mul, Operator::MultiplicationOperator },
  { Token::Div, Operator::DivisionOperator },
  { Token::Remainder, Operator::RemainderOperator },
  { Token::LeftShift, Operator::LeftShiftOperator },
  { Token::RightShift, Operator::RightShiftOperator },
  { Token::Less, Operator::LessOperator },
  { Token::GreaterThan, Operator::GreaterOperator },
  { Token::LessEqual, Operator::LessEqualOperator },
  { Token::GreaterThanEqual, Operator::GreaterEqualOperator },
  { Token::EqEq, Operator::EqualOperator },
  { Token::Neq, Operator::InequalOperator },
  { Token::BitwiseAnd, Operator::BitwiseAndOperator },
  { Token::BitwiseOr, Operator::BitwiseOrOperator },
  { Token::BitwiseXor, Operator::BitwiseXorOperator },
  { Token::LogicalAnd, Operator::LogicalAndOperator },
  { Token::LogicalOr, Operator::LogicalOrOperator },
  { Token::Eq, Operator::AssignmentOperator },
  { Token::MulEq, Operator::MultiplicationAssignmentOperator },
  { Token::DivEq, Operator::DivisionAssignmentOperator },
  { Token::AddEq, Operator::AdditionAssignmentOperator },
  { Token::SubEq, Operator::SubstractionAssignmentOperator },
  { Token::RemainderEq, Operator::RemainderAssignmentOperator },
  { Token::LeftShiftEq, Operator::LeftShiftAssignmentOperator },
  { Token::RightShiftEq, Operator::RightShiftAssignmentOperator },
  { Token::BitAndEq, Operator::BitwiseAndAssignmentOperator },
  { Token::BitOrEq, Operator::BitwiseOrAssignmentOperator },
  { Token::BitXorEq, Operator::BitwiseXorAssignmentOperator },
  { Token::Comma, Operator::CommaOperator },
  };

  if (options & BuiltInOpResol::PrefixOp) {
    auto it = prefixOps.find(tok.type);
    if (it != prefixOps.end())
      return it->second;
  }

  if (options & BuiltInOpResol::PostFixOp) {
    auto it = postFixOps.find(tok.type);
    if (it != postFixOps.end())
      return it->second;
  }

  if (options & BuiltInOpResol::InfixOp) {
    auto it = infixOps.find(tok.type);
    if (it != infixOps.end())
      return it->second;
  }

  if (tok == Token::LeftRightBracket)
    return Operator::SubscriptOperator;
  else if (tok == Token::LeftRightPar)
    return Operator::FunctionCallOperator;

  return Operator::Null;
}

std::string LiteralOperatorName::suffix_string() const
{
  return ast.lock()->text(this->name);
}

std::shared_ptr<ScopedIdentifier> ScopedIdentifier::New(const std::vector<std::shared_ptr<Identifier>>::const_iterator & begin, const std::vector<std::shared_ptr<Identifier>>::const_iterator & end)
{
  auto d = std::distance(begin, end);
  if (d == 2)
    return ScopedIdentifier::New(*begin, parser::Token{}, *(begin + 1));
  auto lhs = ScopedIdentifier::New(begin, begin + (d - 1));
  return ScopedIdentifier::New(lhs, parser::Token{}, *(begin + (d - 1)));
}


FunctionCall::FunctionCall(const std::shared_ptr<Expression> & f,
  const parser::Token & lp,
  std::vector<std::shared_ptr<Expression>> && args,
  const parser::Token & rp)
  : callee(f)
  , leftPar(lp)
  , arguments(std::move(args))
  , rightPar(rp)
{

}

FunctionCall::~FunctionCall()
{

}

std::shared_ptr<FunctionCall> FunctionCall::New(const std::shared_ptr<Expression> & callee,
  const parser::Token & leftPar,
  std::vector<std::shared_ptr<Expression>> && arguments,
  const parser::Token & rightPar)
{
  return std::make_shared<FunctionCall>(callee, leftPar, std::move(arguments), rightPar);
}



BraceConstruction::BraceConstruction(const std::shared_ptr<Identifier> & t, const parser::Token & lb, std::vector<std::shared_ptr<Expression>> && args, const parser::Token & rb)
  : temporary_type(t)
  , left_brace(lb)
  , arguments(std::move(args))
  , right_brace(rb)
{

}

std::shared_ptr<BraceConstruction> BraceConstruction::New(const std::shared_ptr<Identifier> & t, const parser::Token & lb, std::vector<std::shared_ptr<Expression>> && args, const parser::Token & rb)
{
  return std::make_shared<BraceConstruction>(t, lb, std::move(args), rb);
}


ArraySubscript::ArraySubscript(const std::shared_ptr<Expression> & a,
  const parser::Token & lb,
  const std::shared_ptr<Expression> & i,
  const parser::Token & rb)
  : array(a)
  , leftBracket(lb)
  , index(i)
  , rightBracket(rb)
{

}

ArraySubscript::~ArraySubscript()
{

}

std::shared_ptr<ArraySubscript> ArraySubscript::New(const std::shared_ptr<Expression> & a,
  const parser::Token & lb,
  const std::shared_ptr<Expression> & i,
  const parser::Token & rb)
{
  return std::make_shared<ArraySubscript>(a, lb, i, rb);
}


Operation::Operation(const parser::Token & opTok, const std::shared_ptr<Expression> & arg)
  : operatorToken(opTok)
  , arg1(arg)
{

}

Operation::Operation(const parser::Token & opTok, const std::shared_ptr<Expression> & a1, const std::shared_ptr<Expression> & a2)
  : operatorToken(opTok)
  , arg1(a1)
  , arg2(a2)
{

}


std::shared_ptr<Operation> Operation::New(const parser::Token & opTok, const std::shared_ptr<Expression> & arg)
{
  return std::make_shared<Operation>(opTok, arg);
}

std::shared_ptr<Operation> Operation::New(const parser::Token & opTok, const std::shared_ptr<Expression> & a1, const std::shared_ptr<Expression> & a2)
{
  return std::make_shared<Operation>(opTok, a1, a2);
}




ConditionalExpression::ConditionalExpression(const std::shared_ptr<Expression> & cond, const parser::Token & question,
  const std::shared_ptr<Expression> & ifTrue, const parser::Token & colon,
  const std::shared_ptr<Expression> & ifFalse)
  : condition(cond)
  , questionMark(question)
  , onTrue(ifTrue)
  , colon(colon)
  , onFalse(ifFalse)
{

}

ConditionalExpression::~ConditionalExpression()
{

}

std::shared_ptr<ConditionalExpression> ConditionalExpression::New(const std::shared_ptr<Expression> & cond, const parser::Token & question,
  const std::shared_ptr<Expression> & ifTrue, const parser::Token & colon,
  const std::shared_ptr<Expression> & ifFalse)
{
  return std::make_shared<ConditionalExpression>(cond, question, ifTrue, colon, ifFalse);
}



ArrayExpression::ArrayExpression(const parser::Token & lb)
  : leftBracket(lb)
{

}

std::shared_ptr<ArrayExpression> ArrayExpression::New(const parser::Token & lb)
{
  return std::make_shared<ArrayExpression>(lb);
}



ListExpression::ListExpression(const parser::Token & lb)
  : left_brace(lb)
{

}

std::shared_ptr<ListExpression> ListExpression::New(const parser::Token & lb)
{
  return std::make_shared<ListExpression>(lb);
}



NullStatement::NullStatement(const parser::Token & semicolon)
  : semicolon(semicolon)
{

}

std::shared_ptr<NullStatement> NullStatement::New(const parser::Token & semicolon)
{
  return std::make_shared<NullStatement>(semicolon);
}



ExpressionStatement::ExpressionStatement(const std::shared_ptr<Expression> & expr, const parser::Token & semicolon)
  : expression(expr)
  , semicolon(semicolon)
{
  
}

ExpressionStatement::~ExpressionStatement()
{

}

std::shared_ptr<ExpressionStatement> ExpressionStatement::New(const std::shared_ptr<Expression> & expr, const parser::Token & semicolon)
{
  return std::make_shared<ExpressionStatement>(expr, semicolon);
}


CompoundStatement::CompoundStatement(const parser::Token & leftBrace, const parser::Token & rightBrace)
  : openingBrace(leftBrace)
  , closingBrace(rightBrace)
{

}

CompoundStatement::~CompoundStatement()
{

}

std::shared_ptr<CompoundStatement> CompoundStatement::New(const parser::Token & leftBrace, const parser::Token & rightBrace)
{
  return std::make_shared<CompoundStatement>(leftBrace, rightBrace);
}


SelectionStatement::SelectionStatement(const parser::Token & kw)
  : keyword(kw)
{

}

IfStatement::IfStatement(const parser::Token & keyword)
  : SelectionStatement(keyword)
{

}

IfStatement::~IfStatement()
{

}

std::shared_ptr<IfStatement> IfStatement::New(const parser::Token & keyword)
{
  return std::make_shared<IfStatement>(keyword);
}


IterationStatement::IterationStatement(const parser::Token & k)
  : keyword(k)
{

}

WhileLoop::WhileLoop(const parser::Token & whileKw)
  : IterationStatement(whileKw)
{

}

std::shared_ptr<WhileLoop> WhileLoop::New(const parser::Token & keyword)
{
  return std::make_shared<WhileLoop>(keyword);
}

ForLoop::ForLoop(const parser::Token & forKw)
  : IterationStatement(forKw)
{

}

std::shared_ptr<ForLoop> ForLoop::New(const parser::Token & keyword)
{
  return std::make_shared<ForLoop>(keyword);
}


JumpStatement::JumpStatement(const parser::Token & k)
  : keyword(k)
{

}


BreakStatement::BreakStatement(const parser::Token & kw)
  : JumpStatement(kw)
{

}

std::shared_ptr<BreakStatement> BreakStatement::New(const parser::Token & keyword)
{
  return std::make_shared<BreakStatement>(keyword);
}

ContinueStatement::ContinueStatement(const parser::Token & kw)
  : JumpStatement(kw)
{

}

std::shared_ptr<ContinueStatement> ContinueStatement::New(const parser::Token & keyword)
{
  return std::make_shared<ContinueStatement>(keyword);
}

ReturnStatement::ReturnStatement(const parser::Token & kw)
  : JumpStatement(kw)
{

}

std::shared_ptr<ReturnStatement> ReturnStatement::New(const parser::Token & keyword)
{
  return std::make_shared<ReturnStatement>(keyword);
}

std::shared_ptr<ReturnStatement> ReturnStatement::New(const parser::Token & keyword, const std::shared_ptr<Expression> & value)
{
  auto ret = ReturnStatement::New(keyword);
  ret->expression = value;
  return ret;
}


EnumDeclaration::EnumDeclaration(const parser::Token & ek, const parser::Token & ck, const std::shared_ptr<Identifier> & n, const std::vector<EnumValueDeclaration> && vals)
  : enumKeyword(ek)
  , classKeyword(ck)
  , name(n)
  , values(std::move(vals))
{

}

std::shared_ptr<EnumDeclaration> EnumDeclaration::New(const parser::Token & ek, const parser::Token & ck, const std::shared_ptr<Identifier> & n, const std::vector<EnumValueDeclaration> && vals)
{
  return std::make_shared<EnumDeclaration>(ek, ck, n, std::move(vals));
}



ConstructorInitialization::ConstructorInitialization(std::vector<std::shared_ptr<Expression>> && args)
  : args(std::move(args))
{

}

std::shared_ptr<ConstructorInitialization> ConstructorInitialization::New(std::vector<std::shared_ptr<Expression>> && args)
{
  return std::make_shared<ConstructorInitialization>(std::move(args));
}

BraceInitialization::BraceInitialization(std::vector<std::shared_ptr<Expression>> && a)
  : args(std::move(a))
{

}

std::shared_ptr<BraceInitialization> BraceInitialization::New(std::vector<std::shared_ptr<Expression>> && args)
{
  return std::make_shared<BraceInitialization>(std::move(args));
}

AssignmentInitialization::AssignmentInitialization(const parser::Token & eq, const std::shared_ptr<Expression> & val)
  : equalSign(eq)
  , value(val)
{

}

std::shared_ptr<AssignmentInitialization> AssignmentInitialization::New(const parser::Token & eq, const std::shared_ptr<Expression> & val)
{
  return std::make_shared<AssignmentInitialization>(eq, val);
}

VariableDecl::VariableDecl(const QualifiedType & t, const std::shared_ptr<Identifier> & name)
  : variable_type(t)
  , name(name)
{

}

std::shared_ptr<VariableDecl> VariableDecl::New(const QualifiedType & t, const std::shared_ptr<Identifier> & name)
{
  return std::make_shared<VariableDecl>(t, name);
}

bool QualifiedType::isAmbiguous() const
{
  using namespace parser;

  switch (type->name.type)
  {
  case Token::Bool:
  case Token::Char:
  case Token::Int:
  case Token::Float:
  case Token::Double:
    return false;
  default:
    break;
  }

  if (this->constQualifier.isValid() || this->reference.isValid() || this->isFunctionType())
    return false;

  return true;
}

bool QualifiedType::isFunctionType() const
{
  return this->functionType != nullptr;
}

FunctionDecl::FunctionDecl()
{ 
}


FunctionDecl::FunctionDecl(const std::shared_ptr<AST> & a, const std::shared_ptr<Identifier> & name)
  : name(name)
{
  if(name != nullptr)
    ast = name->ast;
  if (a != nullptr)
    ast = a;
}

std::string FunctionDecl::parameterName(int index) const
{
  return ast.lock()->text(this->params.at(index).name);
}

std::shared_ptr<FunctionDecl> FunctionDecl::New(const std::shared_ptr<Identifier> & name)
{
  return std::make_shared<FunctionDecl>(nullptr, name);
}

std::shared_ptr<FunctionDecl> FunctionDecl::New(const std::shared_ptr<AST> a)
{
  return std::make_shared<FunctionDecl>(a, nullptr);
}


MemberInitialization::MemberInitialization(const std::shared_ptr<ast::Identifier> & n, const std::shared_ptr<Initialization> & i)
  : name(n)
  , init(i)
{
}

std::string MemberInitialization::getMemberName() const
{
  return name->getName();
}

ConstructorDecl::ConstructorDecl(const std::shared_ptr<Identifier> & name)
  : FunctionDecl(nullptr, name)
{ }

std::shared_ptr<ConstructorDecl> ConstructorDecl::New(const std::shared_ptr<Identifier> & name)
{
  return std::make_shared<ConstructorDecl>(name);
}

DestructorDecl::DestructorDecl(const std::shared_ptr<Identifier> & name)
  : FunctionDecl(nullptr, name)
{ }

std::shared_ptr<DestructorDecl> DestructorDecl::New(const std::shared_ptr<Identifier> & name)
{
  return std::make_shared<DestructorDecl>(name);
}

OperatorOverloadDecl::OperatorOverloadDecl(std::shared_ptr<AST> a, const std::shared_ptr<Identifier> & name)
  : FunctionDecl(a, name)
{ }

std::shared_ptr<OperatorOverloadDecl> OperatorOverloadDecl::New(std::shared_ptr<AST> a, const std::shared_ptr<Identifier> & name)
{
  return std::make_shared<OperatorOverloadDecl>(a, name);
}

CastDecl::CastDecl(const QualifiedType & rt)
  : FunctionDecl(nullptr, nullptr)
{
  this->returnType = rt;
}

std::shared_ptr<CastDecl> CastDecl::New(const QualifiedType & rt)
{
  return std::make_shared<CastDecl>(rt);
}



LambdaExpression::LambdaExpression(std::shared_ptr<AST> a, const parser::Token & lb)
  : leftBracket(lb)
  , ast(a)
{

}

std::string LambdaExpression::parameterName(int index) const
{
  return ast.lock()->text(this->params.at(index).name);
}

std::string LambdaExpression::captureName(int index) const
{
  return ast.lock()->text(this->captures.at(index).name);
}

std::string LambdaExpression::captureName(const LambdaCapture & cap) const
{
  return ast.lock()->text(cap.name);
}


std::shared_ptr<LambdaExpression> LambdaExpression::New(std::shared_ptr<AST> a, const parser::Token & lb)
{
  return std::make_shared<LambdaExpression>(a, lb);
}

} // namespace ast

} // namespace script

