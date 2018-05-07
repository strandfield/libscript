// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_PARSER_H
#define LIBSCRIPT_PARSER_H

#include "script/parser/lexer.h"
#include "script/ast/ast.h"

namespace script
{

namespace parser
{

struct ParserData
{
private:
  SourceFile mSource;
  Lexer mLexer;
  std::vector<Token> mBuffer;
  size_t mIndex;
public:
  std::shared_ptr<ast::AST> mAst;
public:
  ParserData(const SourceFile & src);
  ParserData(const std::vector<Token> & tokens);
  ParserData(std::vector<Token> && tokens);
  ~ParserData();

  bool atEnd() const;
  Token read();
  Token unsafe_read();
  [[deprecated]] void unread();
  Token peek();
  inline Token unsafe_peek() const { assert(mIndex < (int) mBuffer.size()); return mBuffer[mIndex]; }
  std::string text(const Token & tok) const;

  struct Position {
    size_t index;
    Token token;
  };

  Position pos() const;
  void seek(const Position & p);

  SourceFile::Position sourcepos() const;

  void clearBuffer();

protected:
  void fetchNext();
  bool isDiscardable(const Token & t) const;
};


class AbstractFragment
{
public:
  AbstractFragment(const std::shared_ptr<ParserData> & pdata);
  AbstractFragment(AbstractFragment *parent);
  ~AbstractFragment();

  virtual bool atEnd() const = 0;
  virtual Token read();
  virtual Token peek() const;
  void seekBegin();

  AbstractFragment * parent() const;

public:
  std::shared_ptr<ParserData> data() const;
private:
  std::shared_ptr<ParserData> mData;
  ParserData::Position mBegin;
  AbstractFragment *mParent;
};

class ScriptFragment : public AbstractFragment
{
public:
  ScriptFragment(const std::shared_ptr<ParserData> & pdata);

  bool atEnd() const override;
};

// we might have a problem if parent is a SentinelFragment with the same 
// sentinel value, as the sentinel does not consume the ending token
// e.g. ((a+1)
class SentinelFragment : public AbstractFragment
{
public:
  SentinelFragment(const Token::Type & s, AbstractFragment *parent);
  ~SentinelFragment();

  bool atEnd() const override;

  Token consumeSentinel();

protected:
  Token::Type mSentinel;
};

class StatementFragment : public SentinelFragment
{
public:
  StatementFragment(AbstractFragment *parent);
  ~StatementFragment();
};

class CompoundStatementFragment : public SentinelFragment
{
public:
  CompoundStatementFragment(AbstractFragment *parent);
  ~CompoundStatementFragment();
};

class TemplateArgumentListFragment : public AbstractFragment
{
public: 
  TemplateArgumentListFragment(AbstractFragment *parent);
  ~TemplateArgumentListFragment();

  bool atEnd() const override;
  void consumeEnd();

  bool right_shift_flag;
  Token right_angle;
};

class TemplateArgumentFragment : public AbstractFragment
{
public:
  TemplateArgumentFragment(AbstractFragment *parent);
  ~TemplateArgumentFragment();

  bool atEnd() const override;
  void consumeComma();
};


class FunctionArgFragment : public AbstractFragment
{
public:
  FunctionArgFragment(AbstractFragment *parent);
  ~FunctionArgFragment();

  bool atEnd() const override;
};

class ListFragment : public AbstractFragment
{
public:
  ListFragment(AbstractFragment *parent);
  ~ListFragment();

  bool atEnd() const override;
  void consumeComma();
};

class SubFragment : public AbstractFragment
{
public:
  SubFragment(AbstractFragment *parent);
  ~SubFragment() = default;

  bool atEnd() const override;
};



class ParserBase
{

public:
  ParserBase(AbstractFragment *frag);
  virtual ~ParserBase();

  void reset(AbstractFragment *fragment);

  std::shared_ptr<ast::AST> ast() const;

protected:
  bool atEnd() const;
  bool eof() const;
  Token read();
  Token unsafe_read();
  Token read(const Token::Type & t);
  Token peek() const;
  Token unsafe_peek() const;
  AbstractFragment * fragment() const;
  ParserData::Position pos() const;
  void seek(const ParserData::Position & p);
  const std::string text(const Token & tok);

  template<typename ExceptionType>
  Token readToken(const Token::Type & type)
  {
    Token t = peek();
    if (t.type != type)
      throw ExceptionType{};
    return unsafe_read();
  }

  SourceFile::Position sourcepos() const;

protected:
  AbstractFragment *mFragment;
};

class LiteralParser : public ParserBase
{
public:
  LiteralParser(AbstractFragment *fragment);

  std::shared_ptr<ast::Literal> parse();
};

class ExpressionParser : public ParserBase
{
public:
  ExpressionParser(AbstractFragment *fragment);

  std::shared_ptr<ast::Expression> parse();
protected:
  std::shared_ptr<ast::Expression> readOperand();
  Token readBinaryOperator();
  std::shared_ptr<ast::Expression> buildExpression(const std::vector<std::shared_ptr<ast::Expression>> & operands, const std::vector<Token> & operators);
  std::shared_ptr<ast::Expression> buildExpression(std::vector<std::shared_ptr<ast::Expression>>::const_iterator exprBegin,
    std::vector<std::shared_ptr<ast::Expression>>::const_iterator exprEnd,
    std::vector<Token>::const_iterator opBegin, 
    std::vector<Token>::const_iterator opEnd);
  std::shared_ptr<ast::Expression> buildExpression(std::vector<std::shared_ptr<ast::Expression>>::const_iterator exprBegin,
    std::vector<std::shared_ptr<ast::Expression>>::const_iterator exprEnd,
    std::vector<Token>::const_iterator opBegin,
    std::vector<Token>::const_iterator opEnd, int opIndex);


  bool isPrefixOperator(const Token & tok) const;
  bool isPostfixOperator(const Token & tok) const;
  bool isInfixOperator(const Token & tok) const;
};

// parses a lambda or an array expression
class LambdaParser : public ParserBase
{
public:
  LambdaParser(AbstractFragment *fragment);

  std::shared_ptr<ast::Expression> parse();

  enum Decision {
    Undecided = 0,
    ParsingLambda = 1,
    ParsingArray = 2
  };

  Decision decision() const;
  void setDecision(Decision d);

protected:
  void readBracketContent();
  void readParams();
  std::shared_ptr<ast::CompoundStatement> readBody();

protected:
  Decision mDecision;
  std::shared_ptr<ast::LambdaExpression> mLambda;
  std::shared_ptr<ast::ArrayExpression> mArray;
};

class LambdaCaptureParser : public ParserBase
{
public:
  LambdaCaptureParser(AbstractFragment *fragment);

  bool detect() const;
  ast::LambdaCapture parse();

};

class ProgramParser : public ParserBase
{
public:
  ProgramParser(AbstractFragment *frag);

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

  IdentifierParser(AbstractFragment *fragment, int options = ParseAll);

  int options() const { return mOptions; }
  void setOptions(int opts) { mOptions = opts; }
  inline bool testOption(int opt) const { return (mOptions & opt) != 0; }

  /// TODO : add support for things like ::foo
  std::shared_ptr<ast::Identifier> parse();

protected:
  std::shared_ptr<ast::Identifier> readOperatorName();
  std::shared_ptr<ast::Identifier> readUserDefinedName();
  std::shared_ptr<ast::Identifier> readTemplateArguments(const Token & base);

protected:
  int mOptions;
};

class TemplateArgParser : public ParserBase
{
public:
  TemplateArgParser(AbstractFragment *fragment);

  std::shared_ptr<ast::Node> parse();
};

class TypeParser : public ParserBase
{
public:
  TypeParser(AbstractFragment *fragment);

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
  FunctionParamParser(AbstractFragment *fragment);

  ast::FunctionParameter parse();
};

class ExpressionListParser : public ParserBase
{
public:
  ExpressionListParser(AbstractFragment *fragment);
  ~ExpressionListParser();

  std::vector<std::shared_ptr<ast::Expression>> parse();

protected:
  class Fragment : public AbstractFragment
  {
  public:
    Fragment(AbstractFragment *parent);
    ~Fragment();

    bool atEnd() const override;
  };
};

// parses a variable or function declaration
class DeclParser : public ParserBase
{
public:
  DeclParser(AbstractFragment *fragment, std::shared_ptr<ast::Identifier> className = nullptr);
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
  void readArgs();
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
  EnumValueParser(AbstractFragment *fragment);
  ~EnumValueParser() = default;

  void parse();

  std::vector<ast::EnumValueDeclaration> values;
};

class EnumParser : public ParserBase
{
public:
  EnumParser(AbstractFragment *fragment);
  ~EnumParser() = default;

  std::shared_ptr<ast::EnumDeclaration> parse();
};

bool compareName(const std::shared_ptr<ast::Identifier> & a, const std::shared_ptr<ast::Identifier> & b, const std::shared_ptr<ParserData> & data);

class ClassParser : public ParserBase
{
public:
  ClassParser(AbstractFragment *fragment);
  ~ClassParser();

  void setTemplateSpecialization(bool on);
  std::shared_ptr<ast::ClassDecl> parse();

protected:
  void parseAccessSpecifier(); // public, private, protected
  void parseFriend();
  void parseTemplate();
  void parseUsing();
  std::shared_ptr<ast::Identifier> readClassName();
  void readOptionalParent();
  void readDecl();
  void readNode();
  bool readClassEnd();
  using ParserBase::read;
  Token read(Token::Type tt);

protected:
  std::shared_ptr<ast::ClassDecl> mClass;
  bool mTemplateSpecialization;
};

class NamespaceParser : public ParserBase
{
public:
  NamespaceParser(AbstractFragment *fragment);
  ~NamespaceParser() = default;

  std::shared_ptr<ast::Declaration> parse();

protected:
  std::shared_ptr<ast::Identifier> readNamespaceName();

protected:
  std::shared_ptr<ast::NamespaceDeclaration> mNamespaceDecl;
};

class FriendParser : public ParserBase
{
public:
  FriendParser(AbstractFragment *fragment);
  ~FriendParser() = default;

  std::shared_ptr<ast::FriendDeclaration> parse();
};

class UsingParser : public ParserBase
{
public:
  UsingParser(AbstractFragment *fragment);
  ~UsingParser() = default;

  std::shared_ptr<ast::Declaration> parse();

protected:
  std::shared_ptr<ast::Identifier> read_name();
  void read_semicolon();
};

class ImportParser : public ParserBase
{
public:
  ImportParser(AbstractFragment *fragment);
  ~ImportParser() = default;

  std::shared_ptr<ast::ImportDirective> parse();
};

class TemplateParser : public ParserBase
{
public:
  TemplateParser(AbstractFragment *fragment);
  ~TemplateParser() = default;

  std::shared_ptr<ast::TemplateDeclaration> parse();

protected:
  std::shared_ptr<ast::Declaration> parse_decl();
};

class TemplateParameterParser : public ParserBase
{
public:
  TemplateParameterParser(AbstractFragment *fragment);
  ~TemplateParameterParser() = default;

  ast::TemplateParameter parse();
};

class Parser : public ProgramParser
{
public:
  Parser();
  Parser(const SourceFile & source);
  ~Parser() = default;

  std::shared_ptr<ast::AST> parse(const SourceFile & source);
  std::shared_ptr<ast::AST> parseExpression(const SourceFile & source);

protected:
  std::shared_ptr<ast::ClassDecl> parseClassDeclaration() override;

private:
  std::unique_ptr<ScriptFragment> m_fragment;
};

} // namespace parser

} // namespace script




#endif // LIBSCRIPT_PARSER_H
