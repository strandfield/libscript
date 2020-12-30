// Copyright (C) 2020 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/ast/visitor.h"

#include "script/script.h"

namespace script
{

namespace ast
{

struct AstVisitorDispatcher
{
  AstVisitor& visitor;

  typedef void return_type;

  AstVisitorDispatcher(AstVisitor& v)
    : visitor(v)
  {
  }

  void visit_token(parser::Token tok, AstVisitor::What w = AstVisitor::Child)
  {
    if (tok.isValid())
      visitor.visit(w, tok);
  }

  void recursive_visit(const std::vector<NodeRef>& nodes, AstVisitor::What w = AstVisitor::Child)
  {
    for (auto n : nodes)
    {
      visitor.visit(w, n);
    }
  }

  void recursive_visit(const std::vector<std::shared_ptr<Expression>>& exprs, AstVisitor::What w = AstVisitor::Child)
  {
    for (auto e : exprs)
    {
      visitor.visit(w, e);
    }
  }

  void recursive_visit(const std::vector<std::shared_ptr<Statement>>& stmts, AstVisitor::What w = AstVisitor::Child)
  {
    for (auto s : stmts)
    {
      visitor.visit(w, s);
    }
  }

  void visit(const ast::FunctionParameter& fp)
  {
    visit_qualtype(fp.type);
    visit_token(fp.name, AstVisitor::Name);

    if (fp.defaultValue)
    {
      visitor.visit(AstVisitor::LambdaCapture, fp.defaultValue);
    }
  }

  void recursive_visit(const std::vector<FunctionParameter>& params, AstVisitor::What w = AstVisitor::Child)
  {
    for (const auto& p : params)
    {
      visit(p);
    }
  }

  void recursive_visit(const std::vector<EnumValueDeclaration>& evs, AstVisitor::What w = AstVisitor::Child)
  {
    for (const auto& ev : evs)
    {
      visitor.visit(AstVisitor::Child, ev.name);

      if (ev.value)
      {
        visitor.visit(AstVisitor::Child, ev.value);
      }
    }
  }

  void visit(const ast::LambdaCapture& cap)
  {
    if (cap.reference.isValid())
    {
      visitor.visit(AstVisitor::LambdaCapture, cap.reference);
    }

    if (cap.byValueSign.isValid())
    {
      visitor.visit(AstVisitor::LambdaCapture, cap.byValueSign);
    }

    if (cap.name.isValid())
    {
      visitor.visit(AstVisitor::LambdaCapture, cap.name);
    }

    if (cap.assignmentSign.isValid())
    {
      visitor.visit(AstVisitor::LambdaCapture, cap.assignmentSign);
    }

    if (cap.value)
    {
      visitor.visit(AstVisitor::LambdaCapture, cap.value);
    }
  }

  void recursive_visit(const std::vector<ast::LambdaCapture>& caps)
  {
    for (const auto& c : caps)
    {
      visit(c);
    }
  }

  void visit_qualtype(const ast::QualifiedType& qt)
  {
    if (qt.isNull())
      return;

    if (qt.isFunctionType())
    {
      const ast::FunctionType& ft = *qt.functionType;

      visit_qualtype(ft.returnType);

      for (const auto& pt : ft.params)
      {
        visit_qualtype(pt);
      }
    }
    else
    {
      if (qt.constQualifier.isValid())
      {
        visitor.visit(AstVisitor::Type, qt.constQualifier);
      }

      visitor.visit(AstVisitor::Type, qt.type);

      if (qt.reference.isValid())
      {
        visitor.visit(AstVisitor::Type, qt.reference);
      }
    }
  }

  void visit(ast::Literal& n)
  {
    visitor.visit(AstVisitor::Child, n.token);
  }

  void visit(ast::SimpleIdentifier& id)
  {
    visitor.visit(AstVisitor::Name, id.name);
  }
  
  void visit(ast::TemplateIdentifier& id)
  {
    visitor.visit(AstVisitor::Name, id.name);
    visitor.visit(AstVisitor::TemplateLeftAngle, id.leftAngle);
    recursive_visit(id.arguments, AstVisitor::TemplateArgument);
    visitor.visit(AstVisitor::TemplateRightAngle, id.rightAngle);
  }

  void visit(ast::ScopedIdentifier& id)
  {
    visitor.visit(AstVisitor::NameQualifier, id.lhs);
    visitor.visit(AstVisitor::NameResolutionOperator, id.scopeResolution);
    visitor.visit(AstVisitor::Name, id.rhs);
  }

  void visit(ast::OperatorName& id)
  {
    visitor.visit(AstVisitor::OperatorKeyword, id.keyword);
    visitor.visit(AstVisitor::OperatorSymbol, id.symbol);
  }

  void visit(ast::LiteralOperatorName& id)
  {
    visitor.visit(AstVisitor::OperatorKeyword, id.keyword);
    visitor.visit(AstVisitor::LiteralOperatorDoubleQuotes, id.doubleQuotes);
    visitor.visit(AstVisitor::LiteralOperatorSuffix, id.suffix);
  }

  void visit(ast::TypeNode& tn)
  {
    visit_qualtype(tn.value);
  }

  void visit(ast::FunctionCall& fc)
  {
    visitor.visit(AstVisitor::FunctionCallee, fc.callee);
    visitor.visit(AstVisitor::LeftPar, fc.leftPar);
    recursive_visit(fc.arguments, AstVisitor::FunctionArgument);
    visitor.visit(AstVisitor::RightPar, fc.rightPar);
  }

  void visit(ast::BraceConstruction& bc)
  {
    visitor.visit(AstVisitor::Type, bc.temporary_type);
    visitor.visit(AstVisitor::LeftBrace, bc.left_brace);
    recursive_visit(bc.arguments, AstVisitor::FunctionArgument);
    visitor.visit(AstVisitor::RightBrace, bc.right_brace);
  }

  void visit(ast::ArraySubscript& as)
  {
    visitor.visit(AstVisitor::ArrayObject, as.array);
    visitor.visit(AstVisitor::LeftBracket, as.leftBracket);
    visitor.visit(AstVisitor::ArrayIndex, as.index);
    visitor.visit(AstVisitor::RightBracket, as.rightBracket);
  }

  void visit(ast::Operation& op)
  {
    if (op.isBinary())
    {
      visitor.visit(AstVisitor::OperationLhs, op.arg1);
      visitor.visit(AstVisitor::OperatorSymbol, op.operatorToken);
      visitor.visit(AstVisitor::OperationRhs, op.arg2);
    }
    else
    {
      if (op.isPostfix())
      {
        visitor.visit(AstVisitor::OperationLhs, op.arg1);
        visitor.visit(AstVisitor::OperatorSymbol, op.operatorToken);
      }
      else
      {
        visitor.visit(AstVisitor::OperatorSymbol, op.operatorToken);
        visitor.visit(AstVisitor::OperationRhs, op.arg1);
      }
    }
  }

  void visit(ast::ConditionalExpression& condop)
  {
    visitor.visit(AstVisitor::Condition, condop.condition);
    visitor.visit(AstVisitor::Punctuator, condop.questionMark);
    visitor.visit(AstVisitor::TernaryTrueExpression, condop.onTrue);
    visitor.visit(AstVisitor::Punctuator, condop.colon);
    visitor.visit(AstVisitor::TernaryFalseExpression, condop.onFalse);
  }

  void visit(ast::ArrayExpression& ae)
  {
    visitor.visit(AstVisitor::LeftBracket, ae.leftBracket);
    recursive_visit(ae.elements, AstVisitor::FunctionArgument);
    visitor.visit(AstVisitor::RightBracket, ae.rightBracket);
  }

  void visit(ast::ListExpression& le)
  {
    visitor.visit(AstVisitor::LeftBrace, le.left_brace);
    recursive_visit(le.elements, AstVisitor::FunctionArgument);
    visitor.visit(AstVisitor::RightBrace, le.right_brace);
  }

  void visit(ast::LambdaExpression& le)
  {
    visitor.visit(AstVisitor::LeftBracket, le.leftBracket);
    recursive_visit(le.captures);
    visitor.visit(AstVisitor::RightBracket, le.rightBracket);
    visitor.visit(AstVisitor::LeftPar, le.leftPar);
    recursive_visit(le.params, AstVisitor::LambdaParameter);
    visitor.visit(AstVisitor::RightPar, le.rightPar);
    visitor.visit(AstVisitor::Body, le.body);
  }

  void visit(ast::NullStatement&)
  {
  }

  void visit(ast::ExpressionStatement& es)
  {
    visitor.visit(AstVisitor::Expression, es.expression);
    visitor.visit(AstVisitor::Punctuator, es.semicolon);
  }

  void visit(ast::CompoundStatement& cs)
  {
    visitor.visit(AstVisitor::LeftBrace, cs.openingBrace);
    recursive_visit(cs.statements);
    visitor.visit(AstVisitor::RightBrace, cs.closingBrace);
  }

  void visit(ast::IfStatement& is)
  {
    visitor.visit(AstVisitor::Keyword, is.keyword);
    visitor.visit(AstVisitor::Condition, is.condition);
    visitor.visit(AstVisitor::Body, is.body);

    if (is.elseClause)
    {
      visitor.visit(AstVisitor::Keyword, is.elseKeyword);
      visitor.visit(AstVisitor::Body, is.elseClause);
    }
  }

  void visit(ast::WhileLoop& wl)
  {
    visitor.visit(AstVisitor::Keyword, wl.keyword);
    visitor.visit(AstVisitor::Condition, wl.condition);
    visitor.visit(AstVisitor::Body, wl.body);
  }

  void visit(ast::ForLoop& fl)
  {
    visitor.visit(AstVisitor::Keyword, fl.keyword);
    if (fl.initStatement) visitor.visit(AstVisitor::InitStatement, fl.initStatement);
    if (fl.condition) visitor.visit(AstVisitor::Condition, fl.condition);
    if (fl.loopIncrement) visitor.visit(AstVisitor::Child, fl.loopIncrement);
    visitor.visit(AstVisitor::LoopIncrement, fl.body);
  }

  void visit(ast::ReturnStatement& rs)
  {
    visitor.visit(AstVisitor::Keyword, rs.keyword);

    if (rs.expression)
    {
      visitor.visit(AstVisitor::Expression, rs.expression);
    }
  }

  void visit(ast::JumpStatement& js)
  {
    visitor.visit(AstVisitor::Keyword, js.keyword);
  }

  void visit(ast::EnumDeclaration& ed)
  {
    visitor.visit(AstVisitor::Keyword, ed.enumKeyword);

    if (ed.classKeyword.isValid())
    {
      visitor.visit(AstVisitor::Keyword, ed.classKeyword);
    }

    visitor.visit(AstVisitor::Name, ed.name);

    visitor.visit(AstVisitor::LeftBrace, ed.leftBrace);

    recursive_visit(ed.values);

    visitor.visit(AstVisitor::RightBrace, ed.rightBrace);
  }

  void visit(ast::VariableDecl& vd)
  {
    if (vd.staticSpecifier.isValid())
    {
      visitor.visit(AstVisitor::Type, vd.staticSpecifier);
    }

    visit_qualtype(vd.variable_type);

    visitor.visit(AstVisitor::Name, vd.name);

    if (vd.init)
    {
      visitor.visit(AstVisitor::VarInit, vd.init);
    }

    visitor.visit(AstVisitor::Punctuator, vd.semicolon);
  }

  void visit(ast::ClassDecl& cd)
  {
    visitor.visit(AstVisitor::Keyword, cd.classKeyword);

    visitor.visit(AstVisitor::Name, cd.name);

    if (cd.colon.isValid())
    {
      visitor.visit(AstVisitor::Punctuator, cd.colon);

      visitor.visit(AstVisitor::Name, cd.parent);
    }

    visitor.visit(AstVisitor::LeftBrace, cd.openingBrace);

    recursive_visit(cd.content);

    visitor.visit(AstVisitor::RightBrace, cd.closingBrace);

    visitor.visit(AstVisitor::Punctuator, cd.endingSemicolon);
  }

  void visit(ast::FunctionDecl& fd)
  {
    if (fd.explicitKeyword.isValid())
    {
      visitor.visit(AstVisitor::Type, fd.explicitKeyword);
    }

    if (fd.staticKeyword.isValid())
    {
      visitor.visit(AstVisitor::Type, fd.staticKeyword);
    }

    if (fd.virtualKeyword.isValid())
    {
      visitor.visit(AstVisitor::Type, fd.virtualKeyword);
    }

    visit_qualtype(fd.returnType);

    if(fd.name) // fd.name can be nullptr (e.g. for a conversion function)
      visitor.visit(AstVisitor::Name, fd.name);

    recursive_visit(fd.params);

    if (fd.body)
    {
      visitor.visit(AstVisitor::Body, fd.body);
    }
    else
    {
      visitor.visit(AstVisitor::Punctuator, fd.equalSign);

      if (fd.defaultKeyword.isValid())
      {
        visitor.visit(AstVisitor::Keyword, fd.defaultKeyword);
      }

      if (fd.deleteKeyword.isValid())
      {
        visitor.visit(AstVisitor::Keyword, fd.deleteKeyword);
      }

      if (fd.virtualPure.isValid())
      {
        visitor.visit(AstVisitor::Keyword, fd.virtualPure);
      }
    }
  }

  void vec_visit(std::vector<MemberInitialization>& minits)
  {
    for (auto& i : minits)
    {
      visitor.visit(AstVisitor::Name, i.name);
      visitor.visit(AstVisitor::VarInit, i.init);
    }
  }

  void visit(ast::ConstructorDecl& fd)
  {
    if (fd.explicitKeyword.isValid())
    {
      visitor.visit(AstVisitor::Type, fd.explicitKeyword);
    }

    visitor.visit(AstVisitor::Name, fd.name);

    recursive_visit(fd.params);

    if (fd.body)
    {
      vec_visit(fd.memberInitializationList);

      visitor.visit(AstVisitor::Body, fd.body);
    }
    else
    {
      visitor.visit(AstVisitor::Punctuator, fd.equalSign);

      if (fd.defaultKeyword.isValid())
      {
        visitor.visit(AstVisitor::Keyword, fd.defaultKeyword);
      }

      if (fd.deleteKeyword.isValid())
      {
        visitor.visit(AstVisitor::Keyword, fd.deleteKeyword);
      }
    }
  }

  void visit(ast::OperatorOverloadDecl& overdecl)
  {
    return this->visit(static_cast<ast::FunctionDecl&>(overdecl));
  }

  void visit(ast::CastDecl& castdecl)
  {
    visitor.visit(AstVisitor::OperatorKeyword, castdecl.operatorKw);
    return this->visit(static_cast<ast::FunctionDecl&>(castdecl));
  }

  void visit(ast::AccessSpecifier& aspec)
  {
    visitor.visit(AstVisitor::Keyword, aspec.visibility);
    visitor.visit(AstVisitor::Punctuator, aspec.colon);
  }

  void visit(ast::ConstructorInitialization& ctorinit)
  {
    visitor.visit(AstVisitor::LeftPar, ctorinit.left_par);
    recursive_visit(ctorinit.args);
    visitor.visit(AstVisitor::RightPar, ctorinit.right_par);
  }

  void visit(ast::BraceInitialization& braceinit)
  {
    visitor.visit(AstVisitor::LeftBrace, braceinit.left_brace);
    recursive_visit(braceinit.args);
    visitor.visit(AstVisitor::RightBrace, braceinit.right_brace);
  }

  void visit(ast::AssignmentInitialization& assigninit)
  {
    visitor.visit(AstVisitor::OperatorSymbol, assigninit.equalSign);
    visitor.visit(AstVisitor::Expression, assigninit.value);
  }

  void visit(ast::Typedef& td)
  {
    visitor.visit(AstVisitor::Keyword, td.typedef_token);
    visit_qualtype(td.qualified_type);
    visitor.visit(AstVisitor::Name, td.name);
  }

  void visit(ast::NamespaceDeclaration& nd)
  {
    visitor.visit(AstVisitor::Keyword, nd.namespace_token);
    visitor.visit(AstVisitor::Name, nd.namespace_name);
    visitor.visit(AstVisitor::LeftBrace, nd.left_brace);
    recursive_visit(nd.statements);
    visitor.visit(AstVisitor::RightBrace, nd.right_brace);
  }

  void visit(ast::ClassFriendDeclaration& cfd)
  {
    visitor.visit(AstVisitor::Keyword, cfd.friend_token);
    visitor.visit(AstVisitor::Keyword, cfd.class_token);
    visitor.visit(AstVisitor::Name, cfd.class_name);
  }

  void visit(ast::UsingDeclaration& ud)
  {
    visitor.visit(AstVisitor::Keyword, ud.using_keyword);
    visitor.visit(AstVisitor::Name, ud.used_name);
  }

  void visit(ast::UsingDirective& ud)
  {
    visitor.visit(AstVisitor::Keyword, ud.using_keyword);
    visitor.visit(AstVisitor::Keyword, ud.namespace_keyword);
    visitor.visit(AstVisitor::Name, ud.namespace_name);
  }

  void visit(ast::NamespaceAliasDefinition& nad)
  {
    visitor.visit(AstVisitor::Keyword, nad.namespace_keyword);
    visitor.visit(AstVisitor::Name, nad.alias_name);
    visitor.visit(AstVisitor::OperatorSymbol, nad.equal_token);
    visitor.visit(AstVisitor::Name, nad.aliased_namespace);
  }

  void visit(ast::TypeAliasDeclaration& tad)
  {
    visitor.visit(AstVisitor::Keyword, tad.using_keyword);
    visitor.visit(AstVisitor::Name, tad.alias_name);
    visitor.visit(AstVisitor::Type, tad.aliased_type);
  }

  void vec_visit(std::vector<parser::Token>& toks)
  {
    for(auto& t : toks)
      visitor.visit(AstVisitor::Child, t);
  }

  void visit(ast::ImportDirective& impd)
  {
    if(impd.export_keyword.isValid())
      visitor.visit(AstVisitor::Keyword, impd.export_keyword);

    visitor.visit(AstVisitor::Keyword, impd.import_keyword);
    vec_visit(impd.names);
  }

  void vec_visit(std::vector<ast::TemplateParameter>& tparams)
  {
    for (auto& t : tparams)
    {
      visitor.visit(AstVisitor::Type, t.kind);
      visit_token(t.name);
      visit_token(t.eq);
      if(t.default_value)
        visitor.visit(AstVisitor::Expression, t.default_value);
    }
  }

  void visit(ast::TemplateDeclaration& tdecl)
  {
    visitor.visit(AstVisitor::Keyword, tdecl.template_keyword);
    visitor.visit(AstVisitor::TemplateLeftAngle, tdecl.left_angle_bracket);
    vec_visit(tdecl.parameters);
    visitor.visit(AstVisitor::TemplateRightAngle, tdecl.right_angle_bracket);
    visitor.visit(AstVisitor::Body, tdecl.declaration);
  }

  void visit(ast::ScriptRootNode& root)
  {
    for (auto n : root.statements)
      visitor.visit(AstVisitor::Child, n);
  }
};

AstVisitor::~AstVisitor()
{

}

/*!
 * \fn virtual void AstVisitor::visit(What w, parser::Token tok)
 * \brief function receiving the children of a node
 * \param enumeration describing the child
 * \param the child token
 *
 * The default implementation does nothing.
 */
void AstVisitor::visit(What /* w */, parser::Token /* tok */)
{
  // no-op
}

/*!
 * \fn void recurse(NodeRef n)
 * \brief performs visitation of the node children
 * \param the node
 * 
 * This effectively calls \c{ast::visit()} with this visitor and the given node.
 */
void AstVisitor::recurse(NodeRef n)
{
  script::ast::visit(*this, n);
}

void visit(AstVisitor& v, NodeRef n)
{
  AstVisitorDispatcher dispatcher{ v };
  script::ast::dispatch(dispatcher, *n);
}

} // namespace ast

} // namespace script
