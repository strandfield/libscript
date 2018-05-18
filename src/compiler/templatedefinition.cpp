// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/compiler/templatedefinition.h"

#include "script/ast/visitor.h"

namespace script
{

namespace compiler
{

static size_t ast_end_pos(const std::shared_ptr<ast::TemplateDeclaration> & decl)
{
  if (decl->declaration->is<ast::ClassDecl>())
    return decl->declaration->as<ast::ClassDecl>().endingSemicolon.pos + 1;
  else if (decl->declaration->is<ast::FunctionDecl>())
  {
    const auto & fun = decl->declaration->as<ast::FunctionDecl>();
    if (fun.body != nullptr)
    {
      const auto & body = fun.body->as<ast::CompoundStatement>();
      return body.closingBrace.pos + 1;
    }
    else if (fun.virtualPure.isValid())
      return fun.virtualPure.pos + 1;
    else if (fun.defaultKeyword.isValid())
      return fun.defaultKeyword.pos + 1;
    else if (fun.deleteKeyword.isValid())
      return fun.deleteKeyword.pos + 1;
  }

  throw std::runtime_error{ "TemplateDefinition creation error" };
}

struct CutPasteVisitor
{
  size_t line_offset;
  size_t source_offset;
  std::shared_ptr<ast::AST> ast;

  typedef void return_type;

  void apply(parser::Token & t)
  {
    t.line -= line_offset;
    t.pos -= source_offset;
  }

  void apply(std::vector<parser::Token> & n)
  {
    for (auto & nn : n)
      apply(nn);
  }


  void apply(const std::shared_ptr<ast::Node> & n)
  {
    if (n == nullptr)
      return;
    ast::visit(*this, n);
  }

  void apply(std::vector<std::shared_ptr<ast::Node>> & nodes)
  {
    for (auto & n : nodes)
      ast::visit(*this, n);
  }

  void apply(std::vector<std::shared_ptr<ast::Expression>> & nodes)
  {
    for (auto & n : nodes)
      ast::visit(*this, n);
  }

  void apply(std::vector<std::shared_ptr<ast::Statement>> & nodes)
  {
    for (auto & n : nodes)
      ast::visit(*this, n);
  }

  void visit(const std::shared_ptr<ast::Literal> & l)
  {
    l->ast = ast;
    apply(l->token);
  }

  void visit(const std::shared_ptr<ast::Identifier> & i)
  {
    i->ast = ast;
    apply(i->name);
  }

  void visit(const std::shared_ptr<ast::TemplateIdentifier> & id)
  {
    id->ast = ast;
    apply(id->name);
    apply(id->leftAngle);
    apply(id->rightAngle);
    apply(id->arguments);
  }

  void visit(const std::shared_ptr<ast::OperatorName> & id)
  {
    id->ast = ast;
    apply(id->name);
    apply(id->operatorKeyword);
  }

  void visit(const std::shared_ptr<ast::LiteralOperatorName> & id)
  {
    id->ast = ast;
    apply(id->name);
    apply(id->doubleQuotes);
  }

  void visit(const std::shared_ptr<ast::ScopedIdentifier> & id)
  {
    id->ast = ast;
    apply(id->name);
    apply(id->lhs);
    apply(id->rhs);
  }

  void visit(const std::shared_ptr<ast::FunctionCall> & n)
  {
    apply(n->callee);
    apply(n->arguments);
    apply(n->leftPar);
    apply(n->rightPar);
  }

  void visit(const std::shared_ptr<ast::BraceConstruction> & n)
  {
    apply(n->arguments);
    apply(n->left_brace);
    apply(n->right_brace);
    apply(n->temporary_type);
  }

  void visit(const std::shared_ptr<ast::ArraySubscript> & n)
  {
    apply(n->array);
    apply(n->index);
    apply(n->leftBracket);
    apply(n->rightBracket);
  }

  void visit(const std::shared_ptr<ast::Operation> & n)
  {
    apply(n->arg1);
    apply(n->arg2);
    apply(n->operatorToken);
  }

  void visit(const std::shared_ptr<ast::ConditionalExpression> & n)
  {
    apply(n->colon);
    apply(n->condition);
    apply(n->onFalse);
    apply(n->onTrue);
    apply(n->questionMark);
  }

  void visit(const std::shared_ptr<ast::ArrayExpression> & n)
  {
    apply(n->elements);
    apply(n->leftBracket);
    apply(n->rightBracket);
  }

  void visit(const std::shared_ptr<ast::ListExpression> & n)
  {
    apply(n->elements);
    apply(n->left_brace);
    apply(n->right_brace);
  }

  void visit(const std::shared_ptr<ast::NullStatement> & n)
  {
    apply(n->semicolon);
  }

  void visit(const std::shared_ptr<ast::ExpressionStatement> & n)
  {
    apply(n->expression);
    apply(n->semicolon);
  }

  void visit(const std::shared_ptr<ast::CompoundStatement> & n)
  {
    apply(n->closingBrace);
    apply(n->openingBrace);
    apply(n->statements);
  }

  void visit(const std::shared_ptr<ast::IfStatement> & n)
  {
    apply(n->body);
    apply(n->condition);
    apply(n->elseClause);
    apply(n->elseKeyword);
    apply(n->initStatement);
    apply(n->keyword);
  }

  void visit(const std::shared_ptr<ast::WhileLoop> & n)
  {
    apply(n->body);
    apply(n->condition);
    apply(n->keyword);
  }

  void visit(const std::shared_ptr<ast::ForLoop> & n)
  {
    apply(n->body);
    apply(n->condition);
    apply(n->initStatement);
    apply(n->keyword);
    apply(n->loopIncrement);
  }

  void visit(const std::shared_ptr<ast::JumpStatement> & n)
  {
    apply(n->keyword);
  }

  void visit(const std::shared_ptr<ast::ReturnStatement> & n)
  {
    apply(n->expression);
    apply(n->keyword);
  }

  void apply(ast::EnumValueDeclaration & n)
  {
    apply(n.name);
    apply(n.value);
  }


  void apply(std::vector<ast::EnumValueDeclaration> & n)
  {
    for (auto & nn : n)
      apply(nn);
  }

  void visit(const std::shared_ptr<ast::EnumDeclaration> & n)
  {
    apply(n->classKeyword);
    apply(n->enumKeyword);
    apply(n->name);
    apply(n->values);
  }

  void visit(const std::shared_ptr<ast::ConstructorInitialization> & n)
  {
    apply(n->args);
    apply(n->left_par);
    apply(n->right_par);
  }

  void visit(const std::shared_ptr<ast::BraceInitialization> & n)
  {
    apply(n->args);
    apply(n->left_brace);
    apply(n->right_brace);
  }

  void visit(const std::shared_ptr<ast::AssignmentInitialization> & n)
  {
    apply(n->equalSign);
    apply(n->value);
  }

  void apply(const std::shared_ptr<ast::FunctionType> & n)
  {
    if (n == nullptr)
      return;

    apply(n->params);
    apply(n->returnType);
  }


  void apply(ast::QualifiedType & n)
  {
    apply(n.constQualifier);
    apply(n.functionType);
    apply(n.reference);
    apply(n.type);
  }


  void apply(std::vector<ast::QualifiedType> & n)
  {
    for (auto & nn : n)
      apply(nn);
  }


  void visit(const std::shared_ptr<ast::TypeNode> & n)
  {
    apply(n->value);
  }

  void visit(const std::shared_ptr<ast::VariableDecl> & n)
  {
    apply(n->init);
    apply(n->name);
    apply(n->semicolon);
    apply(n->staticSpecifier);
    apply(n->variable_type);
  }

  void visit(const std::shared_ptr<ast::ClassDecl> & n)
  {
    apply(n->classKeyword);
    apply(n->closingBrace);
    apply(n->colon);
    apply(n->content);
    apply(n->endingSemicolon);
    apply(n->name);
    apply(n->openingBrace);
    apply(n->parent);
  }

  void visit(const std::shared_ptr<ast::AccessSpecifier> & n)
  {
    apply(n->visibility);
    apply(n->colon);
  }

  void apply(ast::FunctionParameter & n)
  {
    apply(n.defaultValue);
    apply(n.name);
    apply(n.type);
  }

  void apply(std::vector<ast::FunctionParameter> & n)
  {
    for (auto & nn : n)
      apply(nn);
  }

  void visit(const std::shared_ptr<ast::FunctionDecl> & n)
  {
    n->ast = ast;
    apply(n->body);
    apply(n->constQualifier);
    apply(n->defaultKeyword);
    apply(n->deleteKeyword);
    apply(n->equalSign);
    apply(n->explicitKeyword);
    apply(n->name);
    apply(n->params);
    apply(n->returnType);
    apply(n->virtualKeyword);
    apply(n->virtualPure);
  }

  void apply(ast::MemberInitialization & n)
  {
    apply(n.init);
    apply(n.name);
  }

  void apply(std::vector<ast::MemberInitialization> & n)
  {
    for (auto & nn : n)
      apply(nn);
  }

  void visit(const std::shared_ptr<ast::ConstructorDecl> & n)
  {
    visit(std::static_pointer_cast<ast::FunctionDecl>(n));
    apply(n->memberInitializationList);
  }

  void visit(const std::shared_ptr<ast::DestructorDecl> & n)
  {
    visit(std::static_pointer_cast<ast::FunctionDecl>(n));
    apply(n->tilde);
  }

  void visit(const std::shared_ptr<ast::CastDecl> & n)
  {
    visit(std::static_pointer_cast<ast::FunctionDecl>(n));
    apply(n->operatorKw);
  }

  void apply(ast::LambdaCapture & n)
  {
    apply(n.assignmentSign);
    apply(n.byValueSign);
    apply(n.name);
    apply(n.reference);
    apply(n.value);
  }

  void apply(std::vector<ast::LambdaCapture> & n)
  {
    for (auto & nn : n)
      apply(nn);
  }

  void visit(const std::shared_ptr<ast::LambdaExpression> & n)
  {
    n->ast = ast;
    apply(n->body);
    apply(n->captures);
    apply(n->leftBracket);
    apply(n->leftPar);
    apply(n->params);
    apply(n->rightBracket);
    apply(n->rightPar);
  }

  void visit(const std::shared_ptr<ast::Typedef> & n)
  {
    apply(n->name);
    apply(n->qualified_type);
    apply(n->typedef_token);
  }

  void visit(const std::shared_ptr<ast::NamespaceDeclaration> & n)
  {
    apply(n->left_brace);
    apply(n->namespace_name);
    apply(n->namespace_token);
    apply(n->right_brace);
    apply(n->statements);
  }

  void visit(const std::shared_ptr<ast::ClassFriendDeclaration> & n)
  {
    apply(n->class_name);
    apply(n->class_token);
    apply(n->friend_token);
  }

  void visit(const std::shared_ptr<ast::UsingDeclaration> & n)
  {
    apply(n->used_name);
    apply(n->using_keyword);
  }

  void visit(const std::shared_ptr<ast::UsingDirective> & n)
  {
    apply(n->namespace_keyword);
    apply(n->namespace_name);
    apply(n->using_keyword);
  }

  void visit(const std::shared_ptr<ast::NamespaceAliasDefinition> & n)
  {
    apply(n->aliased_namespace);
    apply(n->alias_name);
    apply(n->equal_token);
    apply(n->namespace_keyword);
  }

  void visit(const std::shared_ptr<ast::TypeAliasDeclaration> & n)
  {
    apply(n->aliased_type);
    apply(n->alias_name);
    apply(n->equal_token);
    apply(n->using_keyword);
  }

  void visit(const std::shared_ptr<ast::ImportDirective> & n)
  {
    n->ast = ast;
    apply(n->export_keyword);
    apply(n->import_keyword);
    apply(n->names);
  }

  void apply(ast::TemplateParameter & n)
  {
    apply(n.default_value);
    apply(n.eq);
    apply(n.kind);
    apply(n.name);
  }

  void apply(std::vector<ast::TemplateParameter> & n)
  {
    for (auto & nn : n)
      apply(nn);
  }

  void visit(const std::shared_ptr<ast::TemplateDeclaration> & n)
  {
    n->ast = ast;
    apply(n->declaration);
    apply(n->left_angle_bracket);
    apply(n->parameters);
    apply(n->right_angle_bracket);
    apply(n->template_keyword);
  }

};

std::shared_ptr<ast::ClassDecl> TemplateDefinition::get_class_decl() const
{
  return std::static_pointer_cast<ast::ClassDecl>(decl_->declaration);
}

std::shared_ptr<ast::FunctionDecl> TemplateDefinition::get_function_decl() const
{
  return std::static_pointer_cast<ast::FunctionDecl>(decl_->declaration);
}

TemplateDefinition TemplateDefinition::make(const std::shared_ptr<ast::TemplateDeclaration> & decl)
{
  TemplateDefinition ret;

  ret.line_offset_ = decl->template_keyword.line;
  ret.source_offset_ = decl->template_keyword.pos - decl->template_keyword.column;
  ret.decl_ = decl;

  std::string source_code;
  source_code += std::string((size_t)decl->template_keyword.column, ' ');
  parser::Token tok = decl->template_keyword;
  tok.length = ast_end_pos(decl) - tok.pos;
  source_code += decl->ast.lock()->text(tok);
  ret.ast_ = std::make_shared<ast::AST>(SourceFile::fromString(source_code));

  CutPasteVisitor visitor{ ret.line_offset_, ret.source_offset_, ret.ast_ };
  ast::visit(visitor, decl);

  return ret;
}

} // namespace compiler

} // namespace script

