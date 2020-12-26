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
    visit_token(fp.name);

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
        visitor.visit(AstVisitor::Child, qt.constQualifier);
      }

      visitor.visit(AstVisitor::Child, qt.type);

      if (qt.reference.isValid())
      {
        visitor.visit(AstVisitor::Child, qt.reference);
      }
    }
  }

  void visit(ast::Literal& n)
  {
    visitor.visit(AstVisitor::Child, n.token);
  }

  void visit(ast::SimpleIdentifier& id)
  {
    visitor.visit(AstVisitor::Child, id.name);
  }
  
  void visit(ast::TemplateIdentifier& id)
  {
    visitor.visit(AstVisitor::Child, id.name);
    visitor.visit(AstVisitor::Child, id.leftAngle);
    recursive_visit(id.arguments, AstVisitor::TemplateArgument);
    visitor.visit(AstVisitor::Child, id.rightAngle);
  }

  void visit(ast::ScopedIdentifier& id)
  {
    visitor.visit(AstVisitor::Child, id.lhs);
    visitor.visit(AstVisitor::Child, id.scopeResolution);
    visitor.visit(AstVisitor::Child, id.rhs);
  }

  void visit(ast::OperatorName& id)
  {
    visitor.visit(AstVisitor::Child, id.keyword);
    visitor.visit(AstVisitor::Child, id.symbol);
  }

  void visit(ast::LiteralOperatorName& id)
  {
    visitor.visit(AstVisitor::Child, id.keyword);
    visitor.visit(AstVisitor::Child, id.doubleQuotes);
    visitor.visit(AstVisitor::Child, id.suffix);
  }

  void visit(ast::TypeNode& tn)
  {
    visit_qualtype(tn.value);
  }

  void visit(ast::FunctionCall& fc)
  {
    visitor.visit(AstVisitor::Child, fc.callee);
    visitor.visit(AstVisitor::Child, fc.leftPar);
    recursive_visit(fc.arguments, AstVisitor::FunctionArgument);
    visitor.visit(AstVisitor::Child, fc.rightPar);
  }

  void visit(ast::BraceConstruction& bc)
  {
    visitor.visit(AstVisitor::Child, bc.temporary_type);
    visitor.visit(AstVisitor::Child, bc.left_brace);
    recursive_visit(bc.arguments, AstVisitor::FunctionArgument);
    visitor.visit(AstVisitor::Child, bc.right_brace);
  }

  void visit(ast::ArraySubscript& as)
  {
    visitor.visit(AstVisitor::Child, as.array);
    visitor.visit(AstVisitor::Child, as.leftBracket);
    visitor.visit(AstVisitor::Child, as.index);
    visitor.visit(AstVisitor::Child, as.rightBracket);
  }

  void visit(ast::Operation& op)
  {
    if (op.isBinary())
    {
      visitor.visit(AstVisitor::Child, op.arg1);
      visitor.visit(AstVisitor::Child, op.operatorToken);
      visitor.visit(AstVisitor::Child, op.arg2);
    }
    else
    {
      if (op.isPostfix())
      {
        visitor.visit(AstVisitor::Child, op.arg1);
        visitor.visit(AstVisitor::Child, op.operatorToken);
      }
      else
      {
        visitor.visit(AstVisitor::Child, op.operatorToken);
        visitor.visit(AstVisitor::Child, op.arg1);
      }
    }
  }

  void visit(ast::ConditionalExpression& condop)
  {
    visitor.visit(AstVisitor::Child, condop.condition);
    visitor.visit(AstVisitor::Child, condop.questionMark);
    visitor.visit(AstVisitor::Child, condop.onTrue);
    visitor.visit(AstVisitor::Child, condop.colon);
    visitor.visit(AstVisitor::Child, condop.onFalse);
  }

  void visit(ast::ArrayExpression& ae)
  {
    visitor.visit(AstVisitor::Child, ae.leftBracket);
    recursive_visit(ae.elements, AstVisitor::Child);
    visitor.visit(AstVisitor::Child, ae.rightBracket);
  }

  void visit(ast::ListExpression& le)
  {
    visitor.visit(AstVisitor::Child, le.left_brace);
    recursive_visit(le.elements, AstVisitor::Child);
    visitor.visit(AstVisitor::Child, le.right_brace);
  }

  void visit(ast::LambdaExpression& le)
  {
    visitor.visit(AstVisitor::Child, le.leftBracket);
    recursive_visit(le.captures);
    visitor.visit(AstVisitor::Child, le.rightBracket);
    visitor.visit(AstVisitor::Child, le.leftPar);
    recursive_visit(le.params, AstVisitor::LambdaParameter);
    visitor.visit(AstVisitor::Child, le.rightPar);
    visitor.visit(AstVisitor::Child, le.body);
  }

  void visit(ast::NullStatement&)
  {
  }

  void visit(ast::ExpressionStatement& es)
  {
    visitor.visit(AstVisitor::Child, es.expression);
    visitor.visit(AstVisitor::Child, es.semicolon);
  }

  void visit(ast::CompoundStatement& cs)
  {
    visitor.visit(AstVisitor::Child, cs.openingBrace);
    recursive_visit(cs.statements);
    visitor.visit(AstVisitor::Child, cs.closingBrace);
  }

  void visit(ast::IfStatement& is)
  {
    visitor.visit(AstVisitor::Child, is.keyword);
    visitor.visit(AstVisitor::Child, is.condition);
    visitor.visit(AstVisitor::Child, is.body);

    if (is.elseClause)
    {
      visitor.visit(AstVisitor::Child, is.elseKeyword);
      visitor.visit(AstVisitor::Child, is.elseClause);
    }
  }

  void visit(ast::WhileLoop& wl)
  {
    visitor.visit(AstVisitor::Child, wl.keyword);
    visitor.visit(AstVisitor::Child, wl.condition);
    visitor.visit(AstVisitor::Child, wl.body);
  }

  void visit(ast::ForLoop& fl)
  {
    visitor.visit(AstVisitor::Child, fl.keyword);
    visitor.visit(AstVisitor::Child, fl.initStatement);
    visitor.visit(AstVisitor::Child, fl.condition);
    visitor.visit(AstVisitor::Child, fl.loopIncrement);
    visitor.visit(AstVisitor::Child, fl.body);
  }

  void visit(ast::ReturnStatement& rs)
  {
    visitor.visit(AstVisitor::Child, rs.keyword);

    if (rs.expression)
    {
      visitor.visit(AstVisitor::Child, rs.expression);
    }
  }

  void visit(ast::JumpStatement& js)
  {
    visitor.visit(AstVisitor::Child, js.keyword);
  }

  void visit(ast::EnumDeclaration& ed)
  {
    visitor.visit(AstVisitor::Child, ed.enumKeyword);

    if (ed.classKeyword.isValid())
    {
      visitor.visit(AstVisitor::Child, ed.classKeyword);
    }

    visitor.visit(AstVisitor::Child, ed.name);

    visitor.visit(AstVisitor::Child, ed.leftBrace);

    recursive_visit(ed.values);

    visitor.visit(AstVisitor::Child, ed.rightBrace);
  }

  void visit(ast::VariableDecl& vd)
  {
    if (vd.staticSpecifier.isValid())
    {
      visitor.visit(AstVisitor::Child, vd.staticSpecifier);
    }

    visit_qualtype(vd.variable_type);

    visitor.visit(AstVisitor::Child, vd.name);

    if (vd.init)
    {
      visitor.visit(AstVisitor::Child, vd.init);
    }

    visitor.visit(AstVisitor::Child, vd.semicolon);
  }

  void visit(ast::ClassDecl& cd)
  {
    visitor.visit(AstVisitor::Child, cd.classKeyword);

    visitor.visit(AstVisitor::Child, cd.name);

    if (cd.colon.isValid())
    {
      visitor.visit(AstVisitor::Child, cd.colon);

      visitor.visit(AstVisitor::Child, cd.parent);
    }

    visitor.visit(AstVisitor::Child, cd.openingBrace);

    recursive_visit(cd.content);

    visitor.visit(AstVisitor::Child, cd.closingBrace);

    visitor.visit(AstVisitor::Child, cd.endingSemicolon);
  }

  void visit(ast::FunctionDecl& fd)
  {
    if (fd.explicitKeyword.isValid())
    {
      visitor.visit(AstVisitor::Child, fd.explicitKeyword);
    }

    if (fd.staticKeyword.isValid())
    {
      visitor.visit(AstVisitor::Child, fd.staticKeyword);
    }

    if (fd.virtualKeyword.isValid())
    {
      visitor.visit(AstVisitor::Child, fd.virtualKeyword);
    }

    visit_qualtype(fd.returnType);

    visitor.visit(AstVisitor::Child, fd.name);

    recursive_visit(fd.params);

    if (fd.body)
    {
      visitor.visit(AstVisitor::Child, fd.body);
    }
    else
    {
      visitor.visit(AstVisitor::Child, fd.equalSign);

      if (fd.defaultKeyword.isValid())
      {
        visitor.visit(AstVisitor::Child, fd.defaultKeyword);
      }

      if (fd.deleteKeyword.isValid())
      {
        visitor.visit(AstVisitor::Child, fd.deleteKeyword);
      }

      if (fd.virtualPure.isValid())
      {
        visitor.visit(AstVisitor::Child, fd.virtualPure);
      }
    }
  }

  void vec_visit(std::vector<MemberInitialization>& minits)
  {
    for (auto& i : minits)
    {
      visitor.visit(AstVisitor::Child, i.name);
      visitor.visit(AstVisitor::Child, i.init);
    }
  }

  void visit(ast::ConstructorDecl& fd)
  {
    if (fd.explicitKeyword.isValid())
    {
      visitor.visit(AstVisitor::Child, fd.explicitKeyword);
    }

    visitor.visit(AstVisitor::Child, fd.name);

    recursive_visit(fd.params);

    if (fd.body)
    {
      vec_visit(fd.memberInitializationList);

      visitor.visit(AstVisitor::Child, fd.body);
    }
    else
    {
      visitor.visit(AstVisitor::Child, fd.equalSign);

      if (fd.defaultKeyword.isValid())
      {
        visitor.visit(AstVisitor::Child, fd.defaultKeyword);
      }

      if (fd.deleteKeyword.isValid())
      {
        visitor.visit(AstVisitor::Child, fd.deleteKeyword);
      }
    }
  }

  void visit(ast::OperatorOverloadDecl& overdecl)
  {
    return this->visit(static_cast<ast::FunctionDecl&>(overdecl));
  }

  void visit(ast::CastDecl& castdecl)
  {
    return this->visit(static_cast<ast::FunctionDecl&>(castdecl));
  }

  void visit(ast::AccessSpecifier& aspec)
  {
    visitor.visit(AstVisitor::Child, aspec.visibility);
    visitor.visit(AstVisitor::Child, aspec.colon);
  }

  void visit(ast::ConstructorInitialization& ctorinit)
  {
    visitor.visit(AstVisitor::Child, ctorinit.left_par);
    recursive_visit(ctorinit.args);
    visitor.visit(AstVisitor::Child, ctorinit.right_par);
  }

  void visit(ast::BraceInitialization& braceinit)
  {
    visitor.visit(AstVisitor::Child, braceinit.left_brace);
    recursive_visit(braceinit.args);
    visitor.visit(AstVisitor::Child, braceinit.right_brace);
  }

  void visit(ast::AssignmentInitialization& assigninit)
  {
    visitor.visit(AstVisitor::Child, assigninit.equalSign);
    visitor.visit(AstVisitor::Child, assigninit.value);
  }

  void visit(ast::Typedef& td)
  {
    visitor.visit(AstVisitor::Child, td.typedef_token);
    visit_qualtype(td.qualified_type);
    visitor.visit(AstVisitor::Child, td.name);
  }

  void visit(ast::NamespaceDeclaration& nd)
  {
    visitor.visit(AstVisitor::Child, nd.namespace_token);
    visitor.visit(AstVisitor::Child, nd.namespace_name);
    visitor.visit(AstVisitor::Child, nd.left_brace);
    recursive_visit(nd.statements);
    visitor.visit(AstVisitor::Child, nd.right_brace);
  }

  void visit(ast::ClassFriendDeclaration& cfd)
  {
    visitor.visit(AstVisitor::Child, cfd.friend_token);
    visitor.visit(AstVisitor::Child, cfd.class_token);
    visitor.visit(AstVisitor::Child, cfd.class_name);
  }

  void visit(ast::UsingDeclaration& ud)
  {
    visitor.visit(AstVisitor::Child, ud.using_keyword);
    visitor.visit(AstVisitor::Child, ud.used_name);
  }

  void visit(ast::UsingDirective& ud)
  {
    visitor.visit(AstVisitor::Child, ud.using_keyword);
    visitor.visit(AstVisitor::Child, ud.namespace_keyword);
    visitor.visit(AstVisitor::Child, ud.namespace_name);
  }

  void visit(ast::NamespaceAliasDefinition& nad)
  {
    visitor.visit(AstVisitor::Child, nad.namespace_keyword);
    visitor.visit(AstVisitor::Child, nad.alias_name);
    visitor.visit(AstVisitor::Child, nad.equal_token);
    visitor.visit(AstVisitor::Child, nad.aliased_namespace);
  }

  void visit(ast::TypeAliasDeclaration& tad)
  {
    visitor.visit(AstVisitor::Child, tad.using_keyword);
    visitor.visit(AstVisitor::Child, tad.alias_name);
    visitor.visit(AstVisitor::Child, tad.aliased_type);
  }

  void vec_visit(std::vector<parser::Token>& toks)
  {
    for(auto& t : toks)
      visitor.visit(AstVisitor::Child, t);
  }

  void visit(ast::ImportDirective& impd)
  {
    if(impd.export_keyword.isValid())
      visitor.visit(AstVisitor::Child, impd.export_keyword);

    visitor.visit(AstVisitor::Child, impd.import_keyword);
    vec_visit(impd.names);
  }

  void vec_visit(std::vector<ast::TemplateParameter>& tparams)
  {
    for (auto& t : tparams)
    {
      visitor.visit(AstVisitor::Child, t.kind);
      visit_token(t.name);
      visit_token(t.eq);
      if(t.default_value)
        visitor.visit(AstVisitor::Child, t.default_value);
    }
  }

  void visit(ast::TemplateDeclaration& tdecl)
  {
    visitor.visit(AstVisitor::Child, tdecl.template_keyword);
    visitor.visit(AstVisitor::Child, tdecl.left_angle_bracket);
    visitor.visit(AstVisitor::Child, tdecl.right_angle_bracket);
    vec_visit(tdecl.parameters);
    visitor.visit(AstVisitor::Child, tdecl.declaration);
  }
};

AstVisitor::~AstVisitor()
{

}

void AstVisitor::visit(What /* w */, parser::Token /* tok */)
{
  // no-op
}

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
