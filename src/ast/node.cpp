// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/ast/node.h"

#include "script/ast/ast_p.h"

#include <map>

namespace script
{

namespace ast
{

parser::Token Operation::base_token() const
{
  return this->operatorToken;
}

parser::Token ConditionalExpression::base_token() const
{
  return this->questionMark;
}

parser::Token NullStatement::base_token() const
{
  return this->semicolon;
}

parser::Token ExpressionStatement::base_token() const
{
  return this->expression->base_token();
}

parser::Token CompoundStatement::base_token() const
{
  return this->openingBrace;
}

parser::Token IfStatement::base_token() const
{
  return this->keyword;
}

parser::Token IterationStatement::base_token() const
{
  return this->keyword;
}

parser::Token JumpStatement::base_token() const
{
  return this->keyword;
}


parser::Token TypeNode::base_token() const
{
  return value.type->base_token();
}

parser::Token FunctionDecl::base_token() const
{
  return name->base_token();
}


///////////////////////////////////////////////////////////////////////


std::string Literal::toString() const
{
  return this->token.toString();
}

std::string SimpleIdentifier::getName() const
{
  return this->name.toString();
}

std::string TemplateIdentifier::getName() const
{
  return this->name.toString();
}


script::OperatorName OperatorName::getOperatorId(const parser::Token & tok, BuiltInOpResol options)
{
  using parser::Token;

  static std::map<Token::Id, script::OperatorName> prefixOps = {
    { Token::PlusPlus, PreIncrementOperator },
    { Token::MinusMinus, PreDecrementOperator },
    { Token::LogicalNot, LogicalNotOperator },
    { Token::BitwiseNot, BitwiseNot },
    { Token::Plus, UnaryPlusOperator },
    { Token::Minus, UnaryMinusOperator },
  };

  static std::map<Token::Id, script::OperatorName> postFixOps = {
    { Token::PlusPlus, PostIncrementOperator },
    { Token::MinusMinus, PostDecrementOperator },
  };

  static std::map<Token::Id, script::OperatorName> infixOps = {
    { Token::ScopeResolution, ScopeResolutionOperator },
    { Token::Plus, AdditionOperator },
    { Token::Minus, SubstractionOperator },
    { Token::Mul, MultiplicationOperator },
    { Token::Div, DivisionOperator },
    { Token::Remainder, RemainderOperator },
    { Token::LeftShift, LeftShiftOperator },
    { Token::RightShift, RightShiftOperator },
    { Token::Less, LessOperator },
    { Token::GreaterThan, GreaterOperator },
    { Token::LessEqual, LessEqualOperator },
    { Token::GreaterThanEqual, GreaterEqualOperator },
    { Token::EqEq, EqualOperator },
    { Token::Neq, InequalOperator },
    { Token::BitwiseAnd, BitwiseAndOperator },
    { Token::BitwiseOr, BitwiseOrOperator },
    { Token::BitwiseXor, BitwiseXorOperator },
    { Token::LogicalAnd, LogicalAndOperator },
    { Token::LogicalOr, LogicalOrOperator },
    { Token::Eq, AssignmentOperator },
    { Token::MulEq, MultiplicationAssignmentOperator },
    { Token::DivEq, DivisionAssignmentOperator },
    { Token::AddEq, AdditionAssignmentOperator },
    { Token::SubEq, SubstractionAssignmentOperator },
    { Token::RemainderEq, RemainderAssignmentOperator },
    { Token::LeftShiftEq, LeftShiftAssignmentOperator },
    { Token::RightShiftEq, RightShiftAssignmentOperator },
    { Token::BitAndEq, BitwiseAndAssignmentOperator },
    { Token::BitOrEq, BitwiseOrAssignmentOperator },
    { Token::BitXorEq, BitwiseXorAssignmentOperator },
    { Token::Comma, CommaOperator },
  };

  if (options & BuiltInOpResol::PrefixOp) {
    auto it = prefixOps.find(tok.id);
    if (it != prefixOps.end())
      return it->second;
  }

  if (options & BuiltInOpResol::PostFixOp) {
    auto it = postFixOps.find(tok.id);
    if (it != postFixOps.end())
      return it->second;
  }

  if (options & BuiltInOpResol::InfixOp) {
    auto it = infixOps.find(tok.id);
    if (it != infixOps.end())
      return it->second;
  }

  if (tok == Token::LeftRightBracket)
    return SubscriptOperator;
  else if (tok == Token::LeftRightPar)
    return FunctionCallOperator;

  return InvalidOperator;
}

std::string LiteralOperatorName::suffix_string() const
{
  return this->suffix.toString();
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

bool Statement::isDeclaration() const
{
  return false;
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

bool Declaration::isDeclaration() const
{
  return true;
}

EnumDeclaration::EnumDeclaration(const parser::Token & ek, const parser::Token & ck, const std::shared_ptr<SimpleIdentifier> & n, const std::vector<EnumValueDeclaration> && vals)
  : enumKeyword(ek)
  , classKeyword(ck)
  , name(n)
  , values(std::move(vals))
{

}

std::shared_ptr<EnumDeclaration> EnumDeclaration::New(const parser::Token & ek, const parser::Token & ck, const std::shared_ptr<SimpleIdentifier> & n, const std::vector<EnumValueDeclaration> && vals)
{
  return std::make_shared<EnumDeclaration>(ek, ck, n, std::move(vals));
}



ConstructorInitialization::ConstructorInitialization(const parser::Token &lp, std::vector<std::shared_ptr<Expression>> && args, const parser::Token &rp)
  : left_par(lp)
  , args(std::move(args))
  , right_par(rp)
{

}

std::shared_ptr<ConstructorInitialization> ConstructorInitialization::New(const parser::Token &lp, std::vector<std::shared_ptr<Expression>> && args, const parser::Token &rp)
{
  return std::make_shared<ConstructorInitialization>(lp, std::move(args), rp);
}

BraceInitialization::BraceInitialization(const parser::Token & lb, std::vector<std::shared_ptr<Expression>> && a, const parser::Token & rb)
  : left_brace(lb)
  , args(std::move(a))
  , right_brace(rb)
{

}

std::shared_ptr<BraceInitialization> BraceInitialization::New(const parser::Token & lb, std::vector<std::shared_ptr<Expression>> && args, const parser::Token & rb)
{
  return std::make_shared<BraceInitialization>(lb, std::move(args), rb);
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

VariableDecl::VariableDecl(const QualifiedType & t, const std::shared_ptr<SimpleIdentifier> & name)
  : variable_type(t)
  , name(name)
{

}

std::shared_ptr<VariableDecl> VariableDecl::New(const QualifiedType & t, const std::shared_ptr<SimpleIdentifier> & name)
{
  return std::make_shared<VariableDecl>(t, name);
}

bool QualifiedType::isAmbiguous() const
{
  using namespace parser;

  if (type->is<SimpleIdentifier>())
  {
    switch (type->as<SimpleIdentifier>().name.id)
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


FunctionDecl::FunctionDecl(const std::shared_ptr<Identifier> & name)
  : name(name)
{

}

std::string FunctionDecl::parameterName(int index) const
{
  return this->params.at(index).name.toString();
}

std::shared_ptr<FunctionDecl> FunctionDecl::New(const std::shared_ptr<Identifier> & name)
{
  return std::make_shared<FunctionDecl>(name);
}

std::shared_ptr<FunctionDecl> FunctionDecl::New()
{
  return std::make_shared<FunctionDecl>();
}


MemberInitialization::MemberInitialization(const std::shared_ptr<ast::Identifier> & n, const std::shared_ptr<Initialization> & i)
  : name(n)
  , init(i)
{
  assert(n->is<SimpleIdentifier>() || n->is<TemplateIdentifier>());
}

ConstructorDecl::ConstructorDecl(const std::shared_ptr<Identifier> & name)
  : FunctionDecl(name)
{ }

std::shared_ptr<ConstructorDecl> ConstructorDecl::New(const std::shared_ptr<Identifier> & name)
{
  return std::make_shared<ConstructorDecl>(name);
}

DestructorDecl::DestructorDecl(const std::shared_ptr<Identifier> & name)
  : FunctionDecl(name)
{ }

std::shared_ptr<DestructorDecl> DestructorDecl::New(const std::shared_ptr<Identifier> & name)
{
  return std::make_shared<DestructorDecl>(name);
}

OperatorOverloadDecl::OperatorOverloadDecl(const std::shared_ptr<Identifier> & name)
  : FunctionDecl(name)
{ }

std::shared_ptr<OperatorOverloadDecl> OperatorOverloadDecl::New(const std::shared_ptr<Identifier> & name)
{
  return std::make_shared<OperatorOverloadDecl>(name);
}

CastDecl::CastDecl(const QualifiedType & rt)
  : FunctionDecl(nullptr)
{
  this->returnType = rt;
}

std::shared_ptr<CastDecl> CastDecl::New(const QualifiedType & rt)
{
  return std::make_shared<CastDecl>(rt);
}



LambdaExpression::LambdaExpression(const parser::Token & lb)
  : leftBracket(lb)
{

}

std::string LambdaExpression::parameterName(int index) const
{
  return this->params.at(index).name.toString();
}

std::shared_ptr<LambdaExpression> LambdaExpression::New(const parser::Token & lb)
{
  return std::make_shared<LambdaExpression>(lb);
}


Typedef::Typedef(const parser::Token & typedef_tok, const QualifiedType & qtype, const std::shared_ptr<ast::SimpleIdentifier> & n)
  : typedef_token(typedef_tok)
  , qualified_type(qtype)
  , name(n)
{

}

std::shared_ptr<Typedef> Typedef::New(const parser::Token & typedef_tok, const QualifiedType & qtype, const std::shared_ptr<ast::SimpleIdentifier> & n)
{
  return std::make_shared<Typedef>(typedef_tok, qtype, n);
}



NamespaceDeclaration::NamespaceDeclaration(const parser::Token & ns_tok, const std::shared_ptr<ast::SimpleIdentifier> & n, const parser::Token & lb, std::vector<std::shared_ptr<Statement>> && stats, const parser::Token & rb)
  : namespace_token(ns_tok)
  , namespace_name(n)
  , left_brace(lb)
  , statements(std::move(stats))
  , right_brace(rb)
{

}

std::shared_ptr<NamespaceDeclaration> NamespaceDeclaration::New(const parser::Token & ns_tok, const std::shared_ptr<ast::SimpleIdentifier> & n, const parser::Token & lb, std::vector<std::shared_ptr<Statement>> && stats, const parser::Token & rb)
{
  return std::make_shared<NamespaceDeclaration>(ns_tok, n, lb, std::move(stats), rb);
}



FriendDeclaration::FriendDeclaration(const parser::Token & friend_tok)
  : friend_token(friend_tok)
{

}


ClassFriendDeclaration::ClassFriendDeclaration(const parser::Token & friend_tok, const parser::Token & class_tok, const std::shared_ptr<Identifier> & cname)
  : FriendDeclaration(friend_tok)
  , class_token(class_tok)
  , class_name(cname)
{

}

std::shared_ptr<ClassFriendDeclaration> ClassFriendDeclaration::New(const parser::Token & friend_tok, const parser::Token & class_tok, const std::shared_ptr<Identifier> & cname)
{
  return std::make_shared<ClassFriendDeclaration>(friend_tok, class_tok, cname);
}



UsingDeclaration::UsingDeclaration(const parser::Token & using_tok, const std::shared_ptr<ScopedIdentifier> & name)
  : using_keyword(using_tok)
  , used_name(name)
{

}

std::shared_ptr<UsingDeclaration> UsingDeclaration::New(const parser::Token & using_tok, const std::shared_ptr<ScopedIdentifier> & name)
{
  return std::make_shared<UsingDeclaration>(using_tok, name);
}



UsingDirective::UsingDirective(const parser::Token & using_tok, const parser::Token & namespace_tok, const std::shared_ptr<Identifier> & name)
  : using_keyword(using_tok)
  , namespace_keyword(namespace_tok)
  , namespace_name(name)
{

}

std::shared_ptr<UsingDirective> UsingDirective::New(const parser::Token & using_tok, const parser::Token & namespace_tok, const std::shared_ptr<Identifier> & name)
{
  return std::make_shared<UsingDirective>(using_tok, namespace_tok, name);
}



NamespaceAliasDefinition::NamespaceAliasDefinition(const parser::Token & namespace_tok, const std::shared_ptr<SimpleIdentifier> & a, const parser::Token & equal_tok, const std::shared_ptr<Identifier> & b)
  : namespace_keyword(namespace_tok)
  , alias_name(a)
  , equal_token(equal_tok)
  , aliased_namespace(b)
{

}

std::shared_ptr<NamespaceAliasDefinition> NamespaceAliasDefinition::New(const parser::Token & namespace_tok, const std::shared_ptr<SimpleIdentifier> & a, const parser::Token & equal_tok, const std::shared_ptr<Identifier> & b)
{
  return std::make_shared<NamespaceAliasDefinition>(namespace_tok, a, equal_tok, b);
}



TypeAliasDeclaration::TypeAliasDeclaration(const parser::Token & using_tok, const std::shared_ptr<SimpleIdentifier> & a, const parser::Token & equal_tok, const std::shared_ptr<Identifier> & b)
  : using_keyword(using_tok)
  , alias_name(a)
  , equal_token(equal_tok)
  , aliased_type(b)
{

}

std::shared_ptr<TypeAliasDeclaration> TypeAliasDeclaration::New(const parser::Token & using_tok, const std::shared_ptr<SimpleIdentifier> & a, const parser::Token & equal_tok, const std::shared_ptr<Identifier> & b)
{
  return std::make_shared<TypeAliasDeclaration>(using_tok, a, equal_tok, b);
}



ImportDirective::ImportDirective(const parser::Token & exprt, const parser::Token & imprt, std::vector<parser::Token> && nms)
  : export_keyword(exprt)
  , import_keyword(imprt)
  , names(std::move(nms))
{

}

std::shared_ptr<ImportDirective> ImportDirective::New(const parser::Token & exprt, const parser::Token & imprt, std::vector<parser::Token> && nms)
{
  return std::make_shared<ImportDirective>(exprt, imprt, std::move(nms));
}

std::string ImportDirective::at(size_t i) const 
{ 
  return names.at(i).toString();
}

TemplateDeclaration::TemplateDeclaration(const parser::Token & tmplt_k, const parser::Token & left_angle_b, std::vector<TemplateParameter> && params, const parser::Token & right_angle_b, const std::shared_ptr<Declaration> & decl)
  : template_keyword(tmplt_k)
  , left_angle_bracket(left_angle_b)
  , parameters(std::move(params))
  , right_angle_bracket(right_angle_b)
  , declaration(decl)
{

}

std::string TemplateDeclaration::parameter_name(size_t i) const
{
  return parameters.at(i).name.toString();
}

const TemplateParameter & TemplateDeclaration::at(size_t i) const
{
  return parameters.at(i);
}

bool TemplateDeclaration::is_class_template() const
{
  return declaration->is<ast::ClassDecl>();
}

bool TemplateDeclaration::is_full_specialization() const
{
  return parameters.empty();
}

bool TemplateDeclaration::is_partial_specialization() const
{
  if (!declaration->is<ast::ClassDecl>())
    return false;

  const ast::ClassDecl & class_decl = declaration->as<ast::ClassDecl>();
  return class_decl.name->is<ast::TemplateIdentifier>() && !is_full_specialization();
}

std::shared_ptr<TemplateDeclaration> TemplateDeclaration::New(const parser::Token& tmplt_k, const parser::Token& left_angle_b, std::vector<TemplateParameter>&& params, const parser::Token& right_angle_b, const std::shared_ptr<Declaration>& decl)
{
  return std::make_shared<TemplateDeclaration>(tmplt_k, left_angle_b, std::move(params), right_angle_b, decl);
}

ScriptRootNode::ScriptRootNode(const std::shared_ptr<AST> & st)
  : ast(st)
{

}

std::shared_ptr<ScriptRootNode> ScriptRootNode::New(const std::shared_ptr<AST> & syntaxtree)
{
  return std::make_shared<ScriptRootNode>(syntaxtree);
}

} // namespace ast

} // namespace script

