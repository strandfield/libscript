// Copyright (C) 2018-2020 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_PARSER_H
#define LIBSCRIPT_PARSER_H

#include "script/ast/ast_p.h"

#include "script/parser/parsererrors.h"
#include "script/parser/token-reader.h"

namespace script
{

namespace parser
{

struct ParserContext
{
private:
  const char* m_source;
  size_t m_size;
  std::vector<Token> m_tokens;
public:
  explicit ParserContext(const char* src);
  explicit ParserContext(const std::string& str);
  ParserContext(const char* src, size_t s);
  ParserContext(const char* src, std::vector<Token> tokens);
  ~ParserContext();

  const char* source() const { return m_source; }
  size_t source_length() const { return m_size; }

  const std::vector<Token>& tokens() const { return m_tokens; }
};

class ParserBase : protected TokenReader
{
public:
  ParserBase(std::shared_ptr<ParserContext> shared_context, const TokenReader& reader);
  virtual ~ParserBase();

  void reset(std::shared_ptr<ParserContext> shared_context, const TokenReader& reader);

  const Fragment& fragment() const;
  Fragment::iterator iterator() const;
  bool atEnd() const;

  size_t offset() const;

protected:
 
  const std::shared_ptr<ParserContext>& context() const;

  template<typename T>
  auto parse_and_seek(T& parser) -> decltype(parser.parse())
  {
    auto ret = parser.parse();
    seek(parser.iterator());
    return ret;
  }

  SyntaxError SyntaxErr(ParserError e)
  {
    SyntaxError err{ e };
    err.offset = offset();
    return err;
  }

  template<typename T>
  SyntaxError SyntaxErr(ParserError e, T&& d)
  {
    SyntaxError err{ e, std::forward<T>(d) };
    err.offset = offset();
    return err;
  }

protected:
  std::shared_ptr<ParserContext> m_context;
};

class LiteralParser : public ParserBase
{
public:
  LiteralParser(std::shared_ptr<ParserContext> shared_context, const TokenReader& reader);

  std::shared_ptr<ast::Literal> parse();
};

class ExpressionParser : public ParserBase
{
public:
  ExpressionParser(std::shared_ptr<ParserContext> shared_context, const TokenReader& reader);

  std::shared_ptr<ast::Expression> parse();
protected:
  std::shared_ptr<ast::Expression> readOperand();
  Token readBinaryOperator();
  std::shared_ptr<ast::Expression> buildExpression(const std::vector<std::shared_ptr<ast::Expression>> & operands, const std::vector<Token> & operators);
  std::shared_ptr<ast::Expression> buildExpression(std::vector<std::shared_ptr<ast::Expression>>::const_iterator exprBegin,
    std::vector<std::shared_ptr<ast::Expression>>::const_iterator exprEnd,
    std::vector<Token>::const_iterator opBegin, 
    std::vector<Token>::const_iterator opEnd);


  static bool isPrefixOperator(const Token & tok);
  static bool isInfixOperator(const Token & tok);
};

// parses a lambda or an array expression
class LambdaParser : public ParserBase
{
public:
  LambdaParser(std::shared_ptr<ParserContext> shared_context, const TokenReader& reader);

  std::shared_ptr<ast::Expression> parse();

protected:

  enum Decision {
    ParsingLambda = 1,
    ParsingArray = 2
  };

  Decision detect(const Fragment& frag) const;
  std::shared_ptr<ast::Expression> parseArray(TokenReader& bracket_content);
  void readCaptures(TokenReader& bracket_content);
  void readParams();
  std::shared_ptr<ast::CompoundStatement> readBody();

protected:
  std::shared_ptr<ast::LambdaExpression> mLambda;
};

class LambdaCaptureParser : public ParserBase
{
public:
  LambdaCaptureParser(std::shared_ptr<ParserContext> shared_context, const TokenReader& reader);

  bool detect() const;
  ast::LambdaCapture parse();

};

class ProgramParser : public ParserBase
{
public:
  explicit ProgramParser(std::shared_ptr<ParserContext> shared_context);
  ProgramParser(std::shared_ptr<ParserContext> shared_context, const TokenReader& reader);

  std::vector<std::shared_ptr<ast::Statement>> parseProgram();
  virtual std::shared_ptr<ast::Statement> parseStatement();

protected:
  virtual std::shared_ptr<ast::Statement> parseAmbiguous();
  virtual std::shared_ptr<ast::ClassDecl> parseClassDeclaration();
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

class IdentifierParser : public ParserBase
{
public:
  enum Options {
    ParseTemplateId = 1,
    ParseQualifiedId = 2,
    ParseOperatorName = 4,
    ParseAll = ParseTemplateId | ParseQualifiedId | ParseOperatorName,
    ParseSimpleId = 0,
    ParseOnlySimpleId = 0,
  };

  IdentifierParser(std::shared_ptr<ParserContext> shared_context, const TokenReader& reader, int options = ParseAll);

  int options() const { return mOptions; }
  void setOptions(int opts) { mOptions = opts; }
  inline bool testOption(int opt) const { return (mOptions & opt) != 0; }

  /// TODO : add support for things like ::foo
  std::shared_ptr<ast::Identifier> parse();

protected:
  std::shared_ptr<ast::Identifier> readOperatorName();
  std::shared_ptr<ast::Identifier> readUserDefinedName();
  std::shared_ptr<ast::Identifier> readTemplateArguments(const Token & base, TokenReader& reader);

protected:
  int mOptions;
};

class TemplateArgParser : public ParserBase
{
public:
  TemplateArgParser(std::shared_ptr<ParserContext> shared_context, const TokenReader& reader);

  std::shared_ptr<ast::Node> parse();
};

class TypeParser : public ParserBase
{
public:
  TypeParser(std::shared_ptr<ParserContext> shared_context, const TokenReader& reader);

  ast::QualifiedType parse();

  bool detect();

  inline bool readFunctionSignature() const { return mReadFunctionSignature;  }
  inline void setReadFunctionSignature(bool on) { mReadFunctionSignature = on; }

protected:
  bool mReadFunctionSignature;
  ast::QualifiedType tryReadFunctionSignature(const ast::QualifiedType & rt);
};

class FunctionParamParser : public ParserBase
{
public:
  FunctionParamParser(std::shared_ptr<ParserContext> shared_context, const TokenReader& reader);

  ast::FunctionParameter parse();
};

class ExpressionListParser : public ParserBase
{
public:
  ExpressionListParser(std::shared_ptr<ParserContext> shared_context, const TokenReader& reader);
  ~ExpressionListParser();

  std::vector<std::shared_ptr<ast::Expression>> parse();
};

// parses a variable or function declaration
class DeclParser : public ParserBase
{
public:
  DeclParser(std::shared_ptr<ParserContext> shared_context, const TokenReader& reader, std::shared_ptr<ast::Identifier> className = nullptr);
  ~DeclParser();

  int declaratorOptions() const { return mDeclaratorOptions; }
  void setDeclaratorOptions(int opts) { mDeclaratorOptions = opts; }

  // detects if input is a declaration, 
  // if not, it is probably an expression
  bool detectDecl();

  // principle:
  // we try to parse both a vardecl and a funcdecl at the same time,
  // the first that fails is discarded, only the survivor is considered
  // this starts as follow
  // parse a type (var-type or return type)
  // parse an identifier
  //   if its an operator name, vardecl is discarded and decision is ParsingFunction
  //   if its a normal name, we continue
  // if next token is { or =  or , (i.e. another variable) than funcdecl is discarded and decision is ParsingVariable
  // otherwise, if next token is ( we still consider the two possibilities
  // we parse the arguments / params
  // for each one, we first try to parse an expression (i.e. var decl ctor arg)
  // then we parse a param decl 
  // if either fails we set the decision, unless both fail in which case we return Error that we cannot decide
  // we read ')'
  // if next token is 'const' or '{' we got a func decl
  // otherwise if token is ',' or ';' we got a var decl
  std::shared_ptr<ast::Declaration> parse();

  enum Decision {
    Undecided = 0,
    NotADecl = 1,
    ParsingVariable = 2,
    ParsingFunction = 3,
    ParsingCastDecl,
    ParsingConstructor,
    ParsingDestructor,
  };

  Decision decision() const;
  void setDecision(Decision d);

  bool isParsingFunction() const;

  bool isParsingMember() const;

protected:
  // detectDecl() implementation
  void readOptionalDeclSpecifiers();
  bool detectBeforeReadingTypeSpecifier();
  bool readTypeSpecifier();
  bool detectBeforeReadingDeclarator(); // also used to correct ctor misinterpreted as typespecifier
  bool readDeclarator();
  bool detectFromDeclarator();

protected:
  bool readOptionalVirtual();
  bool readOptionalStatic();
  bool readOptionalExplicit();
  void readParams();
  void readArgsOrParams();
  bool readOptionalConst();
  bool readOptionalDeleteSpecifier();
  bool readOptionalDefaultSpecifier();
  bool readOptionalVirtualPureSpecifier();
  std::shared_ptr<ast::CompoundStatement> readFunctionBody();
  bool detectCtorDecl();
  bool detectDtorDecl();
  bool detectCastDecl();
  std::shared_ptr<ast::VariableDecl> parseVarDecl();
  std::shared_ptr<ast::FunctionDecl> parseFunctionDecl();
  std::shared_ptr<ast::FunctionDecl> parseConstructor();
  void readOptionalMemberInitializers();
  std::shared_ptr<ast::FunctionDecl> parseDestructor();

  bool isClassName(const std::shared_ptr<ast::Identifier> & name) const;

protected:
  std::shared_ptr<ast::Identifier> mClassName;
  Token mVirtualKw;
  Token mStaticKw;
  Token mExplicitKw;
  ast::QualifiedType mType;
  std::shared_ptr<ast::Identifier> mName;
  std::shared_ptr<ast::FunctionDecl> mFuncDecl;
  std::shared_ptr<ast::VariableDecl> mVarDecl;
  Decision mDecision;
  bool mParamsAlreadyRead;
  int mDeclaratorOptions;
};

class EnumValueParser : public ParserBase
{
public:
  EnumValueParser(std::shared_ptr<ParserContext> shared_context, const TokenReader& reader);
  ~EnumValueParser() = default;

  void parse();

  std::vector<ast::EnumValueDeclaration> values;
};

class EnumParser : public ParserBase
{
public:
  EnumParser(std::shared_ptr<ParserContext> shared_context, const TokenReader& reader);
  ~EnumParser() = default;

  std::shared_ptr<ast::EnumDeclaration> parse();
};

class ClassParser : public ParserBase
{
public:
  ClassParser(std::shared_ptr<ParserContext> shared_context, const TokenReader& reader);
  ~ClassParser();

  void setTemplateSpecialization(bool on);
  std::shared_ptr<ast::ClassDecl> parse();

protected:
  void parseAccessSpecifier();
  void parseFriend();
  void parseTemplate();
  void parseUsing();
  std::shared_ptr<ast::Identifier> readClassName();
  void readOptionalParent();
  void readDecl();
  void readNode();
  bool readClassEnd();
  using ParserBase::read;

protected:
  std::shared_ptr<ast::ClassDecl> mClass;
  bool mTemplateSpecialization;
};

class NamespaceParser : public ParserBase
{
public:
  NamespaceParser(std::shared_ptr<ParserContext> shared_context, const TokenReader& reader);
  ~NamespaceParser() = default;

  std::shared_ptr<ast::Declaration> parse();

protected:
  std::shared_ptr<ast::SimpleIdentifier> readNamespaceName();

protected:
  std::shared_ptr<ast::NamespaceDeclaration> mNamespaceDecl;
};

class FriendParser : public ParserBase
{
public:
  FriendParser(std::shared_ptr<ParserContext> shared_context, const TokenReader& reader);
  ~FriendParser() = default;

  std::shared_ptr<ast::FriendDeclaration> parse();
};

class UsingParser : public ParserBase
{
public:
  UsingParser(std::shared_ptr<ParserContext> shared_context, const TokenReader& reader);
  ~UsingParser() = default;

  std::shared_ptr<ast::Declaration> parse();

protected:
  std::shared_ptr<ast::Identifier> read_name();
};

class ImportParser : public ParserBase
{
public:
  ImportParser(std::shared_ptr<ParserContext> shared_context, const TokenReader& reader);
  ~ImportParser() = default;

  std::shared_ptr<ast::ImportDirective> parse();
};

class TemplateParser : public ParserBase
{
public:
  TemplateParser(std::shared_ptr<ParserContext> shared_context, const TokenReader& reader);
  ~TemplateParser() = default;

  std::shared_ptr<ast::TemplateDeclaration> parse();

protected:
  std::shared_ptr<ast::Declaration> parse_decl();
};

class TemplateParameterParser : public ParserBase
{
public:
  TemplateParameterParser(std::shared_ptr<ParserContext> shared_context, const TokenReader& reader);
  ~TemplateParameterParser() = default;

  ast::TemplateParameter parse();
};

class Parser : public ProgramParser
{
public:
  Parser();
  explicit Parser(const std::string& str);
  explicit Parser(const char* str);
  ~Parser() = default;

protected:
  std::shared_ptr<ast::ClassDecl> parseClassDeclaration() override;
};

LIBSCRIPT_API std::shared_ptr<ast::AST> parse(const SourceFile& source);

LIBSCRIPT_API std::shared_ptr<ast::Expression> parseExpression(const std::string& src);

} // namespace parser

} // namespace script

#endif // LIBSCRIPT_PARSER_H
