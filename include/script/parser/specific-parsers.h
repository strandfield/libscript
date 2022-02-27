// Copyright (C) 2020 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_PARSER_SPECIFIC_PARSERS_H
#define LIBSCRIPT_PARSER_SPECIFIC_PARSERS_H

#include "script/parser/parser-base.h"

namespace script
{

namespace parser
{

class LIBSCRIPT_API LiteralParser : public ParserBase
{
public:
  LiteralParser(std::shared_ptr<ParserContext> shared_context, const TokenReader& reader);

  std::shared_ptr<ast::Literal> parse();
};

class LIBSCRIPT_API ExpressionParser : public ParserBase
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
class LIBSCRIPT_API LambdaParser : public ParserBase
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

class LIBSCRIPT_API LambdaCaptureParser : public ParserBase
{
public:
  LambdaCaptureParser(std::shared_ptr<ParserContext> shared_context, const TokenReader& reader);

  bool detect() const;
  ast::LambdaCapture parse();

};

class LIBSCRIPT_API IdentifierParser : public ParserBase
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

class LIBSCRIPT_API TemplateArgParser : public ParserBase
{
public:
  TemplateArgParser(std::shared_ptr<ParserContext> shared_context, const TokenReader& reader);

  std::shared_ptr<ast::Node> parse();
};

class LIBSCRIPT_API TypeParser : public ParserBase
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

class LIBSCRIPT_API FunctionParamParser : public ParserBase
{
public:
  FunctionParamParser(std::shared_ptr<ParserContext> shared_context, const TokenReader& reader);

  ast::FunctionParameter parse();
};

class LIBSCRIPT_API ExpressionListParser : public ParserBase
{
public:
  ExpressionListParser(std::shared_ptr<ParserContext> shared_context, const TokenReader& reader);
  ~ExpressionListParser();

  std::vector<std::shared_ptr<ast::Expression>> parse();
};

// parses a variable or function declaration
class LIBSCRIPT_API DeclParser : public ParserBase
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
  void readOptionalAttribute();
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
  std::shared_ptr<ast::AttributeDeclaration> mAttribute;
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

class LIBSCRIPT_API AttributeParser : public ParserBase
{
public:
  AttributeParser(std::shared_ptr<ParserContext> shared_context, const TokenReader& reader);

  bool ready() const;

  std::shared_ptr<ast::AttributeDeclaration> parse();
};

class LIBSCRIPT_API EnumValueParser : public ParserBase
{
public:
  EnumValueParser(std::shared_ptr<ParserContext> shared_context, const TokenReader& reader);
  ~EnumValueParser() = default;

  void parse();

  std::vector<ast::EnumValueDeclaration> values;
};

class LIBSCRIPT_API EnumParser : public ParserBase
{
public:
  EnumParser(std::shared_ptr<ParserContext> shared_context, const TokenReader& reader);
  ~EnumParser() = default;

  std::shared_ptr<ast::EnumDeclaration> parse();
};

class LIBSCRIPT_API ClassParser : public ParserBase
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
  void parseTypedef();
  std::shared_ptr<ast::AttributeDeclaration> readOptionalAttribute();
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

class LIBSCRIPT_API NamespaceParser : public ParserBase
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

class LIBSCRIPT_API FriendParser : public ParserBase
{
public:
  FriendParser(std::shared_ptr<ParserContext> shared_context, const TokenReader& reader);
  ~FriendParser() = default;

  std::shared_ptr<ast::FriendDeclaration> parse();
};

class LIBSCRIPT_API UsingParser : public ParserBase
{
public:
  UsingParser(std::shared_ptr<ParserContext> shared_context, const TokenReader& reader);
  ~UsingParser() = default;

  std::shared_ptr<ast::Declaration> parse();

protected:
  std::shared_ptr<ast::Identifier> read_name();
};

class LIBSCRIPT_API TypedefParser : public ParserBase
{
public:
  TypedefParser(std::shared_ptr<ParserContext> shared_context, const TokenReader& reader);
  ~TypedefParser() = default;

  std::shared_ptr<ast::Typedef> parse();
};

class LIBSCRIPT_API ImportParser : public ParserBase
{
public:
  ImportParser(std::shared_ptr<ParserContext> shared_context, const TokenReader& reader);
  ~ImportParser() = default;

  std::shared_ptr<ast::ImportDirective> parse();
};

class LIBSCRIPT_API TemplateParser : public ParserBase
{
public:
  TemplateParser(std::shared_ptr<ParserContext> shared_context, const TokenReader& reader);
  ~TemplateParser() = default;

  std::shared_ptr<ast::TemplateDeclaration> parse();

protected:
  std::shared_ptr<ast::Declaration> parse_decl();
};

class LIBSCRIPT_API TemplateParameterParser : public ParserBase
{
public:
  TemplateParameterParser(std::shared_ptr<ParserContext> shared_context, const TokenReader& reader);
  ~TemplateParameterParser() = default;

  ast::TemplateParameter parse();
};

} // namespace parser

} // namespace script

#endif // LIBSCRIPT_PARSER_SPECIFIC_PARSERS_H
