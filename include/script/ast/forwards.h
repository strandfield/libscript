// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_AST_FORWARDS_H
#define LIBSCRIPT_AST_FORWARDS_H

namespace script
{

namespace ast
{
class AST;

class ArrayExpression;
class ArraySubscript;
class AssignmentInitialization;
class BoolLiteral;
class BraceConstruction;
class BraceInitialization;
class BreakStatement;
class CastDecl;
class ClassDecl;
class CompoundStatement;
class ConditionalExpression;
class ConstructorDecl;
class ConstructorInitialization;
class ContinueStatement;
class Declaration;
class DestructorDecl;
class EnumDeclaration;
class Expression;
class ExpressionStatement;
class ForLoop;
class FriendDeclaration;
class FunctionCall;
class FunctionDecl;
struct FunctionParameter;
struct FunctionType;
class Identifier;
class IfStatement;
class ImportDirective;
class IntegerLiteral;
class JumpStatement;
class LambdaExpression;
class ListExpression;
class Literal;
class NamespaceAliasDefinition;
class NamespaceDeclaration;
class Node;
class Operation;
class OperatorOverloadDecl;
class QualifiedType;
class ReturnStatement;
class ScopedIdentifier;
class Statement;
class StringLiteral;
class TemplateDeclaration;
class TypeAliasDeclaration;
class Typedef;
class UserDefinedLiteral;
class UsingDeclaration;
class UsingDirective;
class VariableDecl;
class WhileLoop;
} // namespace ast

} // namespace script

#endif // LIBSCRIPT_AST_FORWARDS_H
