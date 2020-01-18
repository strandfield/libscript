// Copyright (C) 2019 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/compiler/scopestatementprocessor.h"

#include "script/compiler/diagnostichelper.h"

#include <stdexcept>

namespace script
{

namespace compiler
{

void ScopeStatementProcessor::processUsingDirective(const std::shared_ptr<ast::UsingDirective>& decl)
{
  NameLookup lookup = name_.resolve(decl->namespace_name, *scope_);

  if (lookup.resultType() != NameLookup::NamespaceName)
    throw CompilationFailure{ CompilerError::InvalidNameInUsingDirective, errors::InvalidName{dstr(decl->namespace_name)} };

  scope_->inject(lookup.scopeResult());
}

void ScopeStatementProcessor::processUsingDeclaration(const std::shared_ptr<ast::UsingDeclaration> & decl)
{
  NameLookup lookup = name_.resolve(decl->used_name, *scope_);
  /// TODO : throw exception if nothing found
  scope_->inject(lookup.impl().get());
}

void ScopeStatementProcessor::processNamespaceAlias(const std::shared_ptr<ast::NamespaceAliasDefinition> & decl)
{
  /// TODO : check that alias_name is a simple identifier or enforce it in the parser
  const std::string& name = decl->alias_name->getName();

  std::vector<std::string> nested;
  auto target = decl->aliased_namespace;
  while (target->is<ast::ScopedIdentifier>())
  {
    const auto& scpid = target->as<ast::ScopedIdentifier>();
    nested.push_back(scpid.rhs->as<ast::SimpleIdentifier>().getName()); /// TODO : check that all names are simple ids
    target = scpid.lhs;
  }
  nested.push_back(target->as<ast::SimpleIdentifier>().getName());

  std::reverse(nested.begin(), nested.end());
  NamespaceAlias alias{ name, std::move(nested) };

  scope_->inject(alias); /// TODO : this may throw and we should handle that
}

void ScopeStatementProcessor::processTypeAlias(const std::shared_ptr<ast::TypeAliasDeclaration> & decl)
{
  /// TODO : check that alias_name is a simple identifier or enforce it in the parser
  const std::string& name = decl->alias_name->getName();

  NameLookup lookup = name_.resolve(decl->aliased_type, *scope_);
  if (lookup.typeResult().isNull())
    throw CompilationFailure{ CompilerError::InvalidTypeName, errors::InvalidName{dstr(decl->aliased_type)} };

  scope_->inject(name, lookup.typeResult());
}

void ScopeStatementProcessor::process(const std::shared_ptr<ast::Statement> & s)
{
  switch (s->type())
  {
  case ast::NodeType::UsingDirective:
    return processUsingDirective(std::static_pointer_cast<ast::UsingDirective>(s));
  case ast::NodeType::UsingDeclaration:
    return processUsingDeclaration(std::static_pointer_cast<ast::UsingDeclaration>(s));
  case ast::NodeType::TypeAliasDecl:
    return processTypeAlias(std::static_pointer_cast<ast::TypeAliasDeclaration>(s));
  case ast::NodeType::NamespaceAliasDef:
    return processNamespaceAlias(std::static_pointer_cast<ast::NamespaceAliasDefinition>(s));
  default:
    break;
  }

  assert(false);
  throw std::runtime_error{ "Bad call to ScopeStatementNameResolver::process()" };
}

} // namespace compiler

} // namespace script

