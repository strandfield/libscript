// Copyright (C) 2018-2020 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_AST_VISITOR_H
#define LIBSCRIPT_AST_VISITOR_H

#include "script/ast/node.h"

namespace script
{

namespace ast
{

/*!
 * \fn Visitor::return_type dispatch(Visitor& v, const std::shared_ptr<ast::Node>& n)
 * \brief performs a virtual-like dispatch of the ast node
 * \param the visitor
 * \param a non-null shared pointer to the ast node
 * 
 * The Visitor class must provide a typedef to a \c{return_type} and one or more overloads 
 * of a \c{visit} function having the following form:
 * 
 * \begin{code}
 * return_type visit(std::shared_ptr<T> node)
 * {
 *   static_assert(std::is_base_of<ast::Node, T>::value);
 *   ...
 * }
 * \end{code}
 */
template<typename Visitor>
typename Visitor::return_type dispatch(Visitor & v, const std::shared_ptr<ast::Node> & n)
{
  switch (n->type())
  {
  case NodeType::BoolLiteral:
    return v.visit(std::static_pointer_cast<ast::BoolLiteral>(n));
  case NodeType::IntegerLiteral:
    return v.visit(std::static_pointer_cast<ast::IntegerLiteral>(n));
  case NodeType::FloatingPointLiteral:
    return v.visit(std::static_pointer_cast<ast::FloatingPointLiteral>(n));
  case NodeType::StringLiteral:
    return v.visit(std::static_pointer_cast<ast::StringLiteral>(n));
  case NodeType::UserDefinedLiteral:
    return v.visit(std::static_pointer_cast<ast::UserDefinedLiteral>(n));
  case NodeType::SimpleIdentifier:
    return v.visit(std::static_pointer_cast<ast::SimpleIdentifier>(n));
  case NodeType::TemplateIdentifier:
    return v.visit(std::static_pointer_cast<ast::TemplateIdentifier>(n));
  case NodeType::QualifiedIdentifier:
    return v.visit(std::static_pointer_cast<ast::ScopedIdentifier>(n));
  case NodeType::OperatorName:
    return v.visit(std::static_pointer_cast<ast::OperatorName>(n));
  case NodeType::LiteralOperatorName:
    return v.visit(std::static_pointer_cast<ast::LiteralOperatorName>(n));
  case NodeType::QualifiedType:
    return v.visit(std::static_pointer_cast<ast::TypeNode>(n));
  case NodeType::FunctionCall:
    return v.visit(std::static_pointer_cast<ast::FunctionCall>(n));
  case NodeType::BraceConstruction:
    return v.visit(std::static_pointer_cast<ast::BraceConstruction>(n));
  case NodeType::ArraySubscript:
    return v.visit(std::static_pointer_cast<ast::ArraySubscript>(n));
  case NodeType::Operation:
    return v.visit(std::static_pointer_cast<ast::Operation>(n));
  case NodeType::ConditionalExpression:
    return v.visit(std::static_pointer_cast<ast::ConditionalExpression>(n));
  case NodeType::ArrayExpression:
    return v.visit(std::static_pointer_cast<ast::ArrayExpression>(n));
  case NodeType::ListExpression:
    return v.visit(std::static_pointer_cast<ast::ListExpression>(n));
  case NodeType::LambdaExpression:
    return v.visit(std::static_pointer_cast<ast::LambdaExpression>(n));
  case NodeType::NullStatement:
    return v.visit(std::static_pointer_cast<ast::NullStatement>(n));
  case NodeType::ExpressionStatement:
    return v.visit(std::static_pointer_cast<ast::ExpressionStatement>(n));
  case NodeType::CompoundStatement:
    return v.visit(std::static_pointer_cast<ast::CompoundStatement>(n));
  case NodeType::IfStatement:
    return v.visit(std::static_pointer_cast<ast::IfStatement>(n));
  case NodeType::WhileLoop:
    return v.visit(std::static_pointer_cast<ast::WhileLoop>(n));
  case NodeType::ForLoop:
    return v.visit(std::static_pointer_cast<ast::ForLoop>(n));
  case NodeType::ReturnStatement:
    return v.visit(std::static_pointer_cast<ast::ReturnStatement>(n));
  case NodeType::ContinueStatement:
    return v.visit(std::static_pointer_cast<ast::ContinueStatement>(n));
  case NodeType::BreakStatement:
    return v.visit(std::static_pointer_cast<ast::BreakStatement>(n));
  case NodeType::EnumDeclaration:
    return v.visit(std::static_pointer_cast<ast::EnumDeclaration>(n));
  case NodeType::VariableDeclaration:
    return v.visit(std::static_pointer_cast<ast::VariableDecl>(n));
  case NodeType::ClassDeclaration:
    return v.visit(std::static_pointer_cast<ast::ClassDecl>(n));
  case NodeType::FunctionDeclaration:
    return v.visit(std::static_pointer_cast<ast::FunctionDecl>(n));
  case NodeType::ConstructorDeclaration:
    return v.visit(std::static_pointer_cast<ast::ConstructorDecl>(n));
  case NodeType::DestructorDeclaration:
    return v.visit(std::static_pointer_cast<ast::DestructorDecl>(n));
  case NodeType::OperatorOverloadDeclaration:
    return v.visit(std::static_pointer_cast<ast::OperatorOverloadDecl>(n));
  case NodeType::CastDeclaration:
    return v.visit(std::static_pointer_cast<ast::CastDecl>(n));
  case NodeType::AccessSpecifier:
    return v.visit(std::static_pointer_cast<ast::AccessSpecifier>(n));
  case NodeType::ConstructorInitialization:
    return v.visit(std::static_pointer_cast<ast::ConstructorInitialization>(n));
  case NodeType::BraceInitialization:
    return v.visit(std::static_pointer_cast<ast::BraceInitialization>(n));
  case NodeType::AssignmentInitialization:
    return v.visit(std::static_pointer_cast<ast::AssignmentInitialization>(n));
  case NodeType::Typedef:
    return v.visit(std::static_pointer_cast<ast::Typedef>(n));
  case NodeType::NamespaceDecl:
    return v.visit(std::static_pointer_cast<ast::NamespaceDeclaration>(n));
  case NodeType::ClassFriendDecl:
    return v.visit(std::static_pointer_cast<ast::ClassFriendDeclaration>(n));
  case NodeType::UsingDeclaration:
    return v.visit(std::static_pointer_cast<ast::UsingDeclaration>(n));
  case NodeType::UsingDirective:
    return v.visit(std::static_pointer_cast<ast::UsingDirective>(n));
  case NodeType::NamespaceAliasDef:
    return v.visit(std::static_pointer_cast<ast::NamespaceAliasDefinition>(n));
  case NodeType::TypeAliasDecl:
    return v.visit(std::static_pointer_cast<ast::TypeAliasDeclaration>(n));
  case NodeType::ImportDirective:
    return v.visit(std::static_pointer_cast<ast::ImportDirective>(n));
  case NodeType::TemplateDecl:
    return v.visit(std::static_pointer_cast<ast::TemplateDeclaration>(n));
  }
}

/*!
 * \fn Visitor::return_type dispatch(Visitor& v, ast::Node& n)
 * \brief performs a virtual-like dispatch of the ast node
 * \param the visitor
 * \param a non-const reference to the ast node
 *
 * This overload works in a similar way as the one taking a shared_ptr but takes 
 * a reference instead.
 */
template<typename Visitor>
typename Visitor::return_type dispatch(Visitor & v, ast::Node & n)
{
  switch (n.type())
  {
  case NodeType::BoolLiteral:
    return v.visit(n.as<ast::BoolLiteral>());
  case NodeType::IntegerLiteral:
    return v.visit(n.as<ast::IntegerLiteral>());
  case NodeType::FloatingPointLiteral:
    return v.visit(n.as<ast::FloatingPointLiteral>());
  case NodeType::StringLiteral:
    return v.visit(n.as<ast::StringLiteral>());
  case NodeType::UserDefinedLiteral:
    return v.visit(n.as<ast::UserDefinedLiteral>());
  case NodeType::SimpleIdentifier:
    return v.visit(n.as<ast::SimpleIdentifier>());
  case NodeType::TemplateIdentifier:
    return v.visit(n.as<ast::TemplateIdentifier>());
  case NodeType::QualifiedIdentifier:
    return v.visit(n.as<ast::ScopedIdentifier>());
  case NodeType::OperatorName:
    return v.visit(n.as<ast::OperatorName>());
  case NodeType::LiteralOperatorName:
    return v.visit(n.as<ast::LiteralOperatorName>());
  case NodeType::QualifiedType:
    return v.visit(n.as<ast::TypeNode>());
  case NodeType::FunctionCall:
    return v.visit(n.as<ast::FunctionCall>());
  case NodeType::BraceConstruction:
    return v.visit(n.as<ast::BraceConstruction>());
  case NodeType::ArraySubscript:
    return v.visit(n.as<ast::ArraySubscript>());
  case NodeType::Operation:
    return v.visit(n.as<ast::Operation>());
  case NodeType::ConditionalExpression:
    return v.visit(n.as<ast::ConditionalExpression>());
  case NodeType::ArrayExpression:
    return v.visit(n.as<ast::ArrayExpression>());
  case NodeType::ListExpression:
    return v.visit(n.as<ast::ListExpression>());
  case NodeType::LambdaExpression:
    return v.visit(n.as<ast::LambdaExpression>());
  case NodeType::NullStatement:
    return v.visit(n.as<ast::NullStatement>());
  case NodeType::ExpressionStatement:
    return v.visit(n.as<ast::ExpressionStatement>());
  case NodeType::CompoundStatement:
    return v.visit(n.as<ast::CompoundStatement>());
  case NodeType::IfStatement:
    return v.visit(n.as<ast::IfStatement>());
  case NodeType::WhileLoop:
    return v.visit(n.as<ast::WhileLoop>());
  case NodeType::ForLoop:
    return v.visit(n.as<ast::ForLoop>());
  case NodeType::ReturnStatement:
    return v.visit(n.as<ast::ReturnStatement>());
  case NodeType::ContinueStatement:
    return v.visit(n.as<ast::ContinueStatement>());
  case NodeType::BreakStatement:
    return v.visit(n.as<ast::BreakStatement>());
  case NodeType::EnumDeclaration:
    return v.visit(n.as<ast::EnumDeclaration>());
  case NodeType::VariableDeclaration:
    return v.visit(n.as<ast::VariableDecl>());
  case NodeType::ClassDeclaration:
    return v.visit(n.as<ast::ClassDecl>());
  case NodeType::FunctionDeclaration:
    return v.visit(n.as<ast::FunctionDecl>());
  case NodeType::ConstructorDeclaration:
    return v.visit(n.as<ast::ConstructorDecl>());
  case NodeType::DestructorDeclaration:
    return v.visit(n.as<ast::DestructorDecl>());
  case NodeType::OperatorOverloadDeclaration:
    return v.visit(n.as<ast::OperatorOverloadDecl>());
  case NodeType::CastDeclaration:
    return v.visit(n.as<ast::CastDecl>());
  case NodeType::AccessSpecifier:
    return v.visit(n.as<ast::AccessSpecifier>());
  case NodeType::ConstructorInitialization:
    return v.visit(n.as<ast::ConstructorInitialization>());
  case NodeType::BraceInitialization:
    return v.visit(n.as<ast::BraceInitialization>());
  case NodeType::AssignmentInitialization:
    return v.visit(n.as<ast::AssignmentInitialization>());
  case NodeType::Typedef:
    return v.visit(n.as<ast::Typedef>());
  case NodeType::NamespaceDecl:
    return v.visit(n.as<ast::NamespaceDeclaration>());
  case NodeType::ClassFriendDecl:
    return v.visit(n.as<ast::ClassFriendDeclaration>());
  case NodeType::UsingDeclaration:
    return v.visit(n.as<ast::UsingDeclaration>());
  case NodeType::UsingDirective:
    return v.visit(n.as<ast::UsingDirective>());
  case NodeType::NamespaceAliasDef:
    return v.visit(n.as<ast::NamespaceAliasDefinition>());
  case NodeType::TypeAliasDecl:
    return v.visit(n.as<ast::TypeAliasDeclaration>());
  case NodeType::ImportDirective:
    return v.visit(n.as<ast::ImportDirective>());
  case NodeType::TemplateDecl:
    return v.visit(n.as<ast::TemplateDeclaration>());
  }
}

/*!
 * \class AstVisitor
 * \brief generic ast visitor
 * 
 * This class can be used to traverse an abstract syntax tree (AST) in 
 * a generic way, i.e. without considering the actual type of the ast nodes.
 */
class LIBSCRIPT_API AstVisitor
{
public:
  AstVisitor() = default;
  virtual ~AstVisitor();
  
  enum What
  {
    Child = 0,
    Name,
    NameQualifier,
    NameResolutionOperator,
    Type,
    TemplateLeftAngle,
    TemplateRightAngle,
    TemplateArgument,
    FunctionArgument,
    LambdaCapture,
    LambdaParameter,
    OperatorKeyword,
    OperatorSymbol,
    LiteralOperatorDoubleQuotes,
    LiteralOperatorSuffix,
    FunctionCallee,
    LeftPar,
    RightPar,
    LeftBrace,
    RightBrace,
    LeftBracket,
    RightBracket,
    ArrayObject,
    ArrayIndex,
    OperationLhs,
    OperationRhs,
    Condition,
    TernaryTrueExpression,
    TernaryFalseExpression,
    Punctuator,
    Body,
    Expression,
    Keyword,
    InitStatement,
    LoopIncrement,
    VarInit,
  };

  /*!
   * \fn virtual void visit(What w, NodeRef n) = 0
   * \brief function receiving the children of a node
   * \param enumeration describing the child
   * \param the child
   */
  virtual void visit(What w, NodeRef n) = 0;

  virtual void visit(What w, parser::Token tok);

  void recurse(NodeRef);
};

/*!
 * \fn void visit(AstVisitor& visitor, NodeRef node);
 * \brief feed the visitor with the children of an ast node
 * \param the visitor
 * \param non-null shared pointer to the node
 */
LIBSCRIPT_API void visit(AstVisitor&, NodeRef);

} // namespace ast

} // namespace script

#endif // LIBSCRIPT_AST_VISITOR_H
