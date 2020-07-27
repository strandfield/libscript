// Copyright (C) 2018-2020 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_PARSER_H
#define LIBSCRIPT_PARSER_H

#include "script/parser/parser-base.h"

namespace script
{

namespace parser
{

class LIBSCRIPT_API ProgramParser : public ParserBase
{
public:
  explicit ProgramParser(std::shared_ptr<ParserContext> shared_context);
  ProgramParser(std::shared_ptr<ParserContext> shared_context, const TokenReader& reader);

  std::shared_ptr<ast::Statement> parse();

  std::vector<std::shared_ptr<ast::Statement>> parseProgram();
  std::shared_ptr<ast::Statement> parseStatement();

protected:
  std::shared_ptr<ast::Statement> parseAmbiguous();
  std::shared_ptr<ast::ClassDecl> parseClassDeclaration();
  std::shared_ptr<ast::EnumDeclaration> parseEnumDeclaration();
  std::shared_ptr<ast::BreakStatement> parseBreakStatement();
  std::shared_ptr<ast::ContinueStatement> parseContinueStatement();
  std::shared_ptr<ast::ReturnStatement> parseReturnStatement();
  std::shared_ptr<ast::CompoundStatement> parseCompoundStatement();
  std::shared_ptr<ast::IfStatement> parseIfStatement();
  std::shared_ptr<ast::WhileLoop> parseWhileLoop();
  std::shared_ptr<ast::ForLoop> parseForLoop();
  std::shared_ptr<ast::Typedef> parseTypedef();
  std::shared_ptr<ast::Declaration> parseNamespace();
  std::shared_ptr<ast::Declaration> parseUsing();
  std::shared_ptr<ast::ImportDirective> parseImport();
  std::shared_ptr<ast::TemplateDeclaration> parseTemplate();
};

class LIBSCRIPT_API Parser : public ProgramParser
{
public:
  Parser();
  explicit Parser(const std::string& str);
  explicit Parser(const char* str);
  ~Parser() = default;

  const std::vector<Token>& tokens() const;
};

LIBSCRIPT_API std::shared_ptr<ast::AST> parse(const SourceFile& source);

LIBSCRIPT_API std::shared_ptr<ast::Expression> parseExpression(const std::string& src);
LIBSCRIPT_API std::shared_ptr<ast::Expression> parseExpression(const char* src);

LIBSCRIPT_API std::shared_ptr<ast::Identifier> parseIdentifier(const std::string& src);

} // namespace parser

} // namespace script

#endif // LIBSCRIPT_PARSER_H
