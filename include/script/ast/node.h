// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_AST_NODE_H
#define LIBSCRIPT_AST_NODE_H

#include <vector>

#include "libscriptdefs.h"
#include "script/parser/token.h"
#include "script/parser/lexer.h"

#include "script/operators.h"

namespace script
{

namespace ast
{

class AST;

class NodeVisitor;

enum class LIBSCRIPT_API NodeType {
  BoolLiteral,
  IntegerLiteral,
  FloatingPointLiteral,
  StringLiteral,
  UserDefinedLiteral,
  SimpleIdentifier,
  TemplateIdentifier,
  QualifiedIdentifier,
  OperatorName,
  LiteralOperatorName,
  QualifiedType,
  FunctionCall,
  BraceConstruction,
  ArraySubscript,
  Operation,
  ConditionalExpression,
  ArrayExpression,
  ListExpression,
  LambdaExpression,
  NullStatement,
  ExpressionStatement,
  CompoundStatement,
  IfStatement,
  WhileLoop,
  ForLoop,
  ReturnStatement,
  ContinueStatement,
  BreakStatement,
  EnumDeclaration,
  VariableDeclaration,
  ClassDeclaration,
  FunctionDeclaration,
  ConstructorDeclaration,
  DestructorDeclaration,
  OperatorOverloadDeclaration,
  CastDeclaration,
  AccessSpecifier,
  ConstructorInitialization,
  BraceInitialization,
  AssignmentInitialization,
  Typedef,
  NamespaceDecl,
  ClassFriendDecl,
  UsingDeclaration,
  UsingDirective,
  NamespaceAliasDef,
  TypeAliasDecl,
  ImportDirective,
  TemplateDecl,
  ScriptRoot,
};

class LIBSCRIPT_API Node
{
public:
  Node() = default;
  Node(const Node & other) = delete;
  virtual ~Node() = default;

  virtual parser::Token base_token() const = 0;

  virtual NodeType type() const = 0;

  template<typename T> 
  bool is() const { return dynamic_cast<const T*>(this) != nullptr; }

  template<typename T>
  const T & as() const { return dynamic_cast<const T&>(*this); }

  template<typename T>
  T & as() { return dynamic_cast<T&>(*this); }
};

typedef std::shared_ptr<Node> NodeRef;


class LIBSCRIPT_API Expression : public Node
{

};

class LIBSCRIPT_API Literal : public Expression
{
public:
  parser::Token token;

public:
  explicit Literal(const parser::Token& tok)
    : token(tok) { }

  inline parser::Token base_token() const override { return this->token; }

  std::string toString() const;
};

class LIBSCRIPT_API IntegralLiteral : public Literal
{
public:
  explicit IntegralLiteral(const parser::Token& tok)
    : Literal(tok) { }

};

class LIBSCRIPT_API BoolLiteral : public IntegralLiteral
{
public:
  explicit BoolLiteral(const parser::Token& tok)
    : IntegralLiteral(tok) { }

  inline static std::shared_ptr<BoolLiteral> New(const parser::Token & tok)
  {
    return std::make_shared<BoolLiteral>(tok);
  }

  static const NodeType type_code = NodeType::BoolLiteral;
  inline NodeType type() const override { return type_code; }
};

class LIBSCRIPT_API IntegerLiteral : public IntegralLiteral
{
public:
  explicit IntegerLiteral(const parser::Token& tok)
    : IntegralLiteral(tok) { }

  inline static std::shared_ptr<IntegerLiteral> New(const parser::Token & tok)
  {
    return std::make_shared<IntegerLiteral>(tok);
  }

  static const NodeType type_code = NodeType::IntegerLiteral;
  inline NodeType type() const override { return type_code; }
};

class LIBSCRIPT_API FloatingPointLiteral : public Literal
{
public:
  explicit FloatingPointLiteral(const parser::Token& tok)
    : Literal(tok) { }

  inline static std::shared_ptr<FloatingPointLiteral> New(const parser::Token & tok)
  {
    return std::make_shared<FloatingPointLiteral>(tok);
  }

  static const NodeType type_code = NodeType::FloatingPointLiteral;
  inline NodeType type() const override { return type_code; }
};

class LIBSCRIPT_API StringLiteral : public Literal
{
public:
  explicit StringLiteral(const parser::Token& tok)
    : Literal(tok) { }

  inline static std::shared_ptr<StringLiteral> New(const parser::Token & tok)
  {
    return std::make_shared<StringLiteral>(tok);
  }

  static const NodeType type_code = NodeType::StringLiteral;
  inline NodeType type() const override { return type_code; }
};

class LIBSCRIPT_API UserDefinedLiteral : public Literal
{
public:
  explicit UserDefinedLiteral(const parser::Token& tok)
    : Literal(tok) { }

  inline static std::shared_ptr<UserDefinedLiteral> New(const parser::Token & tok)
  {
    return std::make_shared<UserDefinedLiteral>(tok);
  }

  static const NodeType type_code = NodeType::UserDefinedLiteral;
  inline NodeType type() const override { return type_code; }
};


class LIBSCRIPT_API Identifier : public Expression
{
public:
  Identifier() = default;
};

class LIBSCRIPT_API SimpleIdentifier : public Identifier
{
public:
  parser::Token name;

public:
  SimpleIdentifier(const parser::Token & n) : name(n) {}

  inline static std::shared_ptr<SimpleIdentifier> New(const parser::Token & name)
  {
    return std::make_shared<SimpleIdentifier>(name);
  }

  std::string getName() const;

  static const NodeType type_code = NodeType::SimpleIdentifier;
  inline NodeType type() const override { return type_code; }
  inline parser::Token base_token() const override { return this->name; }
};

class LIBSCRIPT_API TemplateIdentifier : public Identifier
{
public:
  parser::Token name;
  parser::Token leftAngle;
  std::vector<NodeRef> arguments;
  parser::Token rightAngle;

public:
  TemplateIdentifier(const parser::Token & name, const std::vector<NodeRef> & args, const parser::Token & la, const parser::Token & ra)
    : name(name)
    , arguments(args)
    , leftAngle(la)
    , rightAngle(ra)
  {
  }

  inline static std::shared_ptr<TemplateIdentifier> New(const parser::Token & name, const std::vector<NodeRef> & args, const parser::Token & la, const parser::Token & ra)
  {
    return std::make_shared<TemplateIdentifier>(name, args, la, ra);
  }

  std::string getName() const;

  static const NodeType type_code = NodeType::TemplateIdentifier;
  inline NodeType type() const override { return type_code; }
  inline parser::Token base_token() const override { return this->name; }

};

class LIBSCRIPT_API OperatorName : public Identifier
{
public:
  parser::Token keyword;
  parser::Token symbol;

public:
  OperatorName(const parser::Token & opkeyword, const parser::Token & opSymbol)
    : keyword(opkeyword)
    , symbol(opSymbol)
  {}

  inline static std::shared_ptr<OperatorName> New(const parser::Token & opkeyword, const parser::Token & opSymbol)
  {
    return std::make_shared<OperatorName>(opkeyword, opSymbol);
  }

  enum BuiltInOpResol {
    PrefixOp = 1,
    PostFixOp = 2,
    InfixOp = 4,
    All = PrefixOp | PostFixOp | InfixOp,
    BinaryOp = InfixOp,
    UnaryOp = PrefixOp | PostFixOp,
  };

  static script::OperatorName getOperatorId(const parser::Token & tok, OperatorName::BuiltInOpResol options);

  static const NodeType type_code = NodeType::OperatorName;
  inline NodeType type() const override { return type_code; }
  inline parser::Token base_token() const override { return this->keyword; }
};

class LIBSCRIPT_API LiteralOperatorName : public Identifier
{
public:
  parser::Token keyword;
  parser::Token doubleQuotes;
  parser::Token suffix;

public:
  LiteralOperatorName(const parser::Token & opkeyword, const parser::Token & dquotes, const parser::Token & suffixName)
    : keyword(opkeyword)
    , doubleQuotes(dquotes)
    , suffix(suffixName)
  {}

  inline static std::shared_ptr<LiteralOperatorName> New(const parser::Token & opkeyword, const parser::Token & dquotes, const parser::Token & suffixName)
  {
    return std::make_shared<LiteralOperatorName>(opkeyword, dquotes, suffixName);
  }

  inline const parser::Token & suffixName() const { return this->suffix; }
  std::string suffix_string() const;

  static const NodeType type_code = NodeType::LiteralOperatorName;
  inline NodeType type() const override { return type_code; }
  inline parser::Token base_token() const override { return this->keyword; }

};

// qualified identifier
class LIBSCRIPT_API ScopedIdentifier : public Identifier
{
public:
  std::shared_ptr<Identifier> lhs;
  parser::Token scopeResolution;
  std::shared_ptr<Identifier> rhs;

public:
  ScopedIdentifier(const std::shared_ptr<Identifier> & l, const parser::Token & scopeRes, const std::shared_ptr<Identifier> & r)
    : lhs(l)
    , scopeResolution(scopeRes)
    , rhs(r)
  {}

  inline static std::shared_ptr<ScopedIdentifier> New(const std::shared_ptr<Identifier> & l, const parser::Token & scopeRes, const std::shared_ptr<Identifier> & r)
  {
    return std::make_shared<ScopedIdentifier>(l, scopeRes, r);
  }

  static std::shared_ptr<ScopedIdentifier> New(const std::vector<std::shared_ptr<Identifier>>::const_iterator & begin, const std::vector<std::shared_ptr<Identifier>>::const_iterator & end);

  static const NodeType type_code = NodeType::QualifiedIdentifier;
  inline NodeType type() const override { return type_code; }
  inline parser::Token base_token() const override { return rhs->base_token(); }

};


class LIBSCRIPT_API FunctionCall : public Expression
{
public:
  std::shared_ptr<Expression> callee;
  parser::Token leftPar;
  std::vector<std::shared_ptr<Expression>> arguments;
  parser::Token rightPar;

public:
  FunctionCall(const std::shared_ptr<Expression> & f,
    const parser::Token & lp,
    std::vector<std::shared_ptr<Expression>> && args,
    const parser::Token & rp);
  ~FunctionCall();

  static const NodeType type_code = NodeType::FunctionCall;
  inline NodeType type() const override { return type_code; }
  parser::Token base_token() const override { return this->leftPar; }

  static std::shared_ptr<FunctionCall> New(const std::shared_ptr<Expression> & callee,
    const parser::Token & leftPar,
    std::vector<std::shared_ptr<Expression>> && arguments,
    const parser::Token & rightPar);
};

class LIBSCRIPT_API BraceConstruction : public Expression
{
public:
  std::shared_ptr<Identifier> temporary_type;
  parser::Token left_brace;
  std::vector<std::shared_ptr<Expression>> arguments;
  parser::Token right_brace;

public:
  BraceConstruction(const std::shared_ptr<Identifier> & t,
    const parser::Token & lb,
    std::vector<std::shared_ptr<Expression>> && args,
    const parser::Token & rb);
  ~BraceConstruction() = default;

  static const NodeType type_code = NodeType::BraceConstruction;
  inline NodeType type() const override { return type_code; }
  inline parser::Token base_token() const override { return this->left_brace; }

  static std::shared_ptr<BraceConstruction> New(const std::shared_ptr<Identifier> & t,
    const parser::Token & lb,
    std::vector<std::shared_ptr<Expression>> && args,
    const parser::Token & rb);
};

class LIBSCRIPT_API ArraySubscript : public Expression
{
public:
  std::shared_ptr<Expression> array;
  parser::Token leftBracket;
  std::shared_ptr<Expression> index;
  parser::Token rightBracket;

public:
  ArraySubscript(const std::shared_ptr<Expression> & a,
    const parser::Token & lb,
    const std::shared_ptr<Expression> & i,
    const parser::Token & rb);
  ~ArraySubscript();

  static const NodeType type_code = NodeType::ArraySubscript;
  inline NodeType type() const override { return type_code; }
  inline parser::Token base_token() const override { return this->leftBracket; }

  static std::shared_ptr<ArraySubscript> New(const std::shared_ptr<Expression> & a,
    const parser::Token & lb,
    const std::shared_ptr<Expression> & i,
    const parser::Token & rb);
};


class LIBSCRIPT_API Operation : public Expression
{
public:
  parser::Token operatorToken;
  std::shared_ptr<Expression> arg1;
  std::shared_ptr<Expression> arg2;

public:
  Operation(const parser::Token & opTok, const std::shared_ptr<Expression> & arg);
  Operation(const parser::Token & opTok, const std::shared_ptr<Expression> & a1, const std::shared_ptr<Expression> & a2);

  static std::shared_ptr<Operation> New(const parser::Token & opTok, const std::shared_ptr<Expression> & arg);
  static std::shared_ptr<Operation> New(const parser::Token & opTok, const std::shared_ptr<Expression> & a1, const std::shared_ptr<Expression> & a2);

  parser::Token base_token() const override;

  static const NodeType type_code = NodeType::Operation;
  inline NodeType type() const override { return type_code; }
};

class LIBSCRIPT_API ConditionalExpression : public Expression
{
public:
  std::shared_ptr<Expression> condition;
  parser::Token questionMark;
  std::shared_ptr<Expression> onTrue;
  parser::Token colon;
  std::shared_ptr<Expression> onFalse;

public:
  ConditionalExpression(const std::shared_ptr<Expression> & cond, const parser::Token & question, 
                        const std::shared_ptr<Expression> & ifTrue, const parser::Token & colon,
                        const std::shared_ptr<Expression> & ifFalse);
  ~ConditionalExpression();

  static std::shared_ptr<ConditionalExpression> New(const std::shared_ptr<Expression> & cond, const parser::Token & question,
    const std::shared_ptr<Expression> & ifTrue, const parser::Token & colon,
    const std::shared_ptr<Expression> & ifFalse);

  parser::Token base_token() const override;

  static const NodeType type_code = NodeType::ConditionalExpression;
  inline NodeType type() const override { return type_code; }
};

class LIBSCRIPT_API ArrayExpression : public Expression
{
public:
  parser::Token leftBracket;
  std::vector<std::shared_ptr<Expression>> elements;
  parser::Token rightBracket;

public:
  ArrayExpression(const parser::Token & lb);
  ~ArrayExpression() = default;

  static std::shared_ptr<ArrayExpression> New(const parser::Token & lb);

  static const NodeType type_code = NodeType::ArrayExpression;
  inline NodeType type() const override { return type_code; }
  inline parser::Token base_token() const override { return this->leftBracket; }
};

class LIBSCRIPT_API ListExpression : public Expression
{
public:
  parser::Token left_brace;
  std::vector<std::shared_ptr<Expression>> elements;
  parser::Token right_brace;

public:
  ListExpression(const parser::Token & lb);
  ~ListExpression() = default;

  static std::shared_ptr<ListExpression> New(const parser::Token & lb);

  static const NodeType type_code = NodeType::ListExpression;
  inline NodeType type() const override { return type_code; }
  inline parser::Token base_token() const override { return this->left_brace; }

};



class LIBSCRIPT_API Statement : public Node
{
public:

  virtual bool isDeclaration() const;
};

class LIBSCRIPT_API NullStatement : public Statement
{
public:
  parser::Token semicolon;

public:
  NullStatement(const parser::Token & semicolon);
  ~NullStatement() = default;

  static std::shared_ptr<NullStatement> New(const parser::Token & semicolon);

  parser::Token base_token() const override;

  static const NodeType type_code = NodeType::NullStatement;
  inline NodeType type() const override { return type_code; }
};

class LIBSCRIPT_API ExpressionStatement : public Statement
{
public:
  std::shared_ptr<Expression> expression;
  parser::Token semicolon;

public:
  ExpressionStatement(const std::shared_ptr<Expression> & expr, const parser::Token & semicolon);
  ~ExpressionStatement();

  static std::shared_ptr<ExpressionStatement> New(const std::shared_ptr<Expression> & expr, const parser::Token & semicolon);

  parser::Token base_token() const override;

  static const NodeType type_code = NodeType::ExpressionStatement;
  inline NodeType type() const override { return type_code; }
};

class LIBSCRIPT_API CompoundStatement : public Statement
{
public:
  parser::Token openingBrace;
  std::vector<std::shared_ptr<Statement>> statements;
  parser::Token closingBrace;

public:
  CompoundStatement(const parser::Token & leftBrace, const parser::Token & rightBrace);
  ~CompoundStatement();

  static std::shared_ptr<CompoundStatement> New(const parser::Token & leftBrace, const parser::Token & rightBrace);

  parser::Token base_token() const override;

  static const NodeType type_code = NodeType::CompoundStatement;
  inline NodeType type() const override { return type_code; }
};

struct LIBSCRIPT_API SelectionStatement : public Statement
{
  parser::Token keyword;

public:
  SelectionStatement(const parser::Token & kw);
  ~SelectionStatement() = default;

  parser::Token base_token() const override { return this->keyword; }
};

class LIBSCRIPT_API IfStatement : public SelectionStatement
{
public:
  std::shared_ptr<Statement> initStatement; // currently unused
  std::shared_ptr<Expression> condition;
  std::shared_ptr<Statement> body;
  parser::Token elseKeyword;
  std::shared_ptr<Statement> elseClause;

public:
  IfStatement(const parser::Token & keyword);
  ~IfStatement();

  static std::shared_ptr<IfStatement> New(const parser::Token & keyword);

  parser::Token base_token() const override;

  static const NodeType type_code = NodeType::IfStatement;
  inline NodeType type() const override { return type_code; }
};


struct LIBSCRIPT_API IterationStatement : public Statement
{
  parser::Token keyword;

public:
  IterationStatement(const parser::Token & k);

  parser::Token base_token() const override;


};

class LIBSCRIPT_API WhileLoop : public IterationStatement
{
public:
  std::shared_ptr<Expression> condition;
  std::shared_ptr<Statement> body;

public:
  WhileLoop(const parser::Token & whileKw);
  ~WhileLoop() = default;

  static std::shared_ptr<WhileLoop> New(const parser::Token & keyword);

  static const NodeType type_code = NodeType::WhileLoop;
  inline NodeType type() const override { return type_code; }

};

class LIBSCRIPT_API ForLoop : public IterationStatement
{
public:
  std::shared_ptr<Statement> initStatement;
  std::shared_ptr<Expression> condition;
  std::shared_ptr<Expression> loopIncrement;
  std::shared_ptr<Statement> body;

public:
  ForLoop(const parser::Token & forKw);
  ~ForLoop() = default;

  static std::shared_ptr<ForLoop> New(const parser::Token & keyword);

  static const NodeType type_code = NodeType::ForLoop;
  inline NodeType type() const override { return type_code; }
};

class LIBSCRIPT_API JumpStatement : public Statement
{
public:
  parser::Token keyword;

public:
  JumpStatement(const parser::Token & k);

  parser::Token base_token() const override;
};

class LIBSCRIPT_API BreakStatement : public JumpStatement
{

public:
  BreakStatement(const parser::Token & kw);
  ~BreakStatement() = default;

  static std::shared_ptr<BreakStatement> New(const parser::Token & keyword);

  static const NodeType type_code = NodeType::BreakStatement;
  inline NodeType type() const override { return type_code; }
};

class LIBSCRIPT_API ContinueStatement : public JumpStatement
{
public:
  ContinueStatement(const parser::Token & kw);
  ~ContinueStatement() = default;

  static std::shared_ptr<ContinueStatement> New(const parser::Token & keyword);

  static const NodeType type_code = NodeType::ContinueStatement;
  inline NodeType type() const override { return type_code; }
};

class LIBSCRIPT_API ReturnStatement : public JumpStatement
{
public:
  std::shared_ptr<Expression> expression;

public:
  ReturnStatement(const parser::Token & kw);
  ~ReturnStatement() = default;

  static std::shared_ptr<ReturnStatement> New(const parser::Token & keyword);
  static std::shared_ptr<ReturnStatement> New(const parser::Token & keyword, const std::shared_ptr<Expression> & value);

  static const NodeType type_code = NodeType::ReturnStatement;
  inline NodeType type() const override { return type_code; }

};

class LIBSCRIPT_API Declaration : public Statement
{
public:

  bool isDeclaration() const override;
};

struct LIBSCRIPT_API EnumValueDeclaration
{
  std::shared_ptr<SimpleIdentifier> name;
  std::shared_ptr<Expression> value;
};

class LIBSCRIPT_API EnumDeclaration : public Declaration
{
public:
  parser::Token enumKeyword;
  parser::Token classKeyword;
  std::shared_ptr<SimpleIdentifier> name;
  std::vector<EnumValueDeclaration> values;

public:
  EnumDeclaration() = default;
  EnumDeclaration(const parser::Token & ek, const parser::Token & ck, const std::shared_ptr<SimpleIdentifier> & n, const std::vector<EnumValueDeclaration> && vals);
  ~EnumDeclaration() = default;

  static std::shared_ptr<EnumDeclaration> New(const parser::Token & ek, const parser::Token & ck, const std::shared_ptr<SimpleIdentifier> & n, const std::vector<EnumValueDeclaration> && vals);

  static const NodeType type_code = NodeType::EnumDeclaration;
  inline NodeType type() const override { return type_code; }
  parser::Token base_token() const override { return enumKeyword; }

};

class LIBSCRIPT_API Initialization : public Node
{

};

class LIBSCRIPT_API ConstructorInitialization : public Initialization
{
public:
  parser::Token left_par;
  std::vector<std::shared_ptr<Expression>> args;
  parser::Token right_par;

public:
  ConstructorInitialization() = default;
  ConstructorInitialization(const parser::Token &lp, std::vector<std::shared_ptr<Expression>> && args, const parser::Token &rp);
  ~ConstructorInitialization() = default;

  static std::shared_ptr<ConstructorInitialization> New(const parser::Token &lp, std::vector<std::shared_ptr<Expression>> && args, const parser::Token &rp);

  static const NodeType type_code = NodeType::ConstructorInitialization;
  inline NodeType type() const override { return type_code; }
  parser::Token base_token() const override { return left_par;  }
};

class LIBSCRIPT_API BraceInitialization : public Initialization
{
public:
  parser::Token left_brace;
  std::vector<std::shared_ptr<Expression>> args;
  parser::Token right_brace;

public:
  BraceInitialization() = default;
  BraceInitialization(const parser::Token & lb, std::vector<std::shared_ptr<Expression>> && args, const parser::Token & rb);
  ~BraceInitialization() = default;

  static std::shared_ptr<BraceInitialization> New(const parser::Token & lb, std::vector<std::shared_ptr<Expression>> && args, const parser::Token & rb);

  static const NodeType type_code = NodeType::BraceInitialization;
  inline NodeType type() const override { return type_code; }
  parser::Token base_token() const override { return left_brace; }

};

class LIBSCRIPT_API AssignmentInitialization : public Initialization
{
public:
  parser::Token equalSign;
  std::shared_ptr<Expression> value;

public:
  AssignmentInitialization(const parser::Token & eq, const std::shared_ptr<Expression> & val);
  ~AssignmentInitialization() = default;

  static std::shared_ptr<AssignmentInitialization> New(const parser::Token & eq, const std::shared_ptr<Expression> & val);

  static const NodeType type_code = NodeType::AssignmentInitialization;
  inline NodeType type() const override { return type_code; }
  parser::Token base_token() const override { return this->equalSign; }

};



struct FunctionType;

class LIBSCRIPT_API QualifiedType
{
public:
  std::shared_ptr<Identifier> type;
  parser::Token constQualifier;
  parser::Token reference;
  std::shared_ptr<FunctionType> functionType;

  inline bool isConst() const { return constQualifier.isValid(); }
  inline bool isRef() const { return reference == parser::Token::Ref; }
  inline bool isRefRef() const { return reference == parser::Token::RefRef; }
  inline bool isSimple() const { return !constQualifier.isValid() && !reference.isValid(); }

  // returns true if this type object might be interpreted as a variable name
  // e.g. 'const a &' and 'b &' are not ambiguous but 'a' is 
  bool isAmbiguous() const;

  bool isFunctionType() const;
};

struct LIBSCRIPT_API FunctionType
{
  QualifiedType returnType;
  std::vector<QualifiedType> params;
};

class LIBSCRIPT_API TypeNode : public Node
{
public:
  QualifiedType value;

  TypeNode(const QualifiedType & t)
    : value(t)
  {

  }

  ~TypeNode() = default;

  inline static std::shared_ptr<TypeNode> New(const QualifiedType &t)
  {
    return std::make_shared<TypeNode>(t);
  }

  parser::Token base_token() const override;

  static const NodeType type_code = NodeType::QualifiedType;
  inline NodeType type() const override { return type_code; }
};


class LIBSCRIPT_API VariableDecl : public Declaration
{
public:
  QualifiedType variable_type;
  parser::Token staticSpecifier;
  std::shared_ptr<SimpleIdentifier> name;
  std::shared_ptr<Initialization> init;
  parser::Token semicolon;

public:
  VariableDecl(const QualifiedType & t, const std::shared_ptr<SimpleIdentifier> & name);
  ~VariableDecl() = default;

  static std::shared_ptr<VariableDecl> New(const QualifiedType & t, const std::shared_ptr<SimpleIdentifier> & name);

  static const NodeType type_code = NodeType::VariableDeclaration;
  inline NodeType type() const override { return type_code; }
  inline parser::Token base_token() const override { return name->base_token(); }
};

class LIBSCRIPT_API ClassDecl : public Declaration
{
public:
  parser::Token classKeyword;
  std::shared_ptr<Identifier> name;
  parser::Token colon;
  std::shared_ptr<Identifier> parent;
  parser::Token openingBrace;
  std::vector<NodeRef> content;
  parser::Token closingBrace;
  parser::Token endingSemicolon;


public:
  ClassDecl(const parser::Token& classK, const std::shared_ptr<Identifier> & cname)
    : classKeyword(classK)
    , name(cname) 
  { }

  inline static std::shared_ptr<ClassDecl> New(const parser::Token& classK, const std::shared_ptr<Identifier> & cname)
  {
    return std::make_shared<ClassDecl>(classK, cname);
  }

  parser::Token base_token() const override
  {
    return name->base_token();
  }

  static const NodeType type_code = NodeType::ClassDeclaration;
  inline NodeType type() const override { return type_code; }
};

class LIBSCRIPT_API AccessSpecifier : public Node
{
public:
  parser::Token visibility;
  parser::Token colon;

public:
  AccessSpecifier(const parser::Token& v, const parser::Token& col)
    : visibility(v)
    , colon(col)
  { }

  inline static std::shared_ptr<AccessSpecifier> New(const parser::Token& visibility, const parser::Token& colon)
  {
    return std::make_shared<AccessSpecifier>(visibility, colon);
  }

  parser::Token base_token() const override
  {
    return visibility;
  }

  static const NodeType type_code = NodeType::AccessSpecifier;
  inline NodeType type() const override { return type_code; }
};

struct LIBSCRIPT_API FunctionParameter
{
  QualifiedType type;
  parser::Token name;
  std::shared_ptr<Expression> defaultValue;
};



class LIBSCRIPT_API FunctionDecl : public Declaration
{
public:
  QualifiedType returnType;
  std::shared_ptr<Identifier> name;
  std::vector<FunctionParameter> params;
  std::shared_ptr<CompoundStatement> body;
  parser::Token constQualifier;
  parser::Token explicitKeyword;
  parser::Token staticKeyword;
  parser::Token virtualKeyword;
  parser::Token equalSign;
  parser::Token deleteKeyword;
  parser::Token defaultKeyword;
  parser::Token virtualPure;

  inline bool isExplicit() const { return explicitKeyword.isValid(); }
  inline bool isStatic() const { return staticKeyword.isValid(); }
  inline bool isVirtual() const { return virtualKeyword.isValid(); }
  inline bool isDeleted() const { return deleteKeyword.isValid(); }
  inline bool isVirtualPure() const { return virtualKeyword.isValid() && virtualPure.isValid(); }

public:
  FunctionDecl();
  FunctionDecl(const std::shared_ptr<Identifier> & name);
  ~FunctionDecl() = default;

  std::string parameterName(int index) const;

  static std::shared_ptr<FunctionDecl> New(const std::shared_ptr<Identifier> & name);
  static std::shared_ptr<FunctionDecl> New();

  parser::Token base_token() const override;
  
  static const NodeType type_code = NodeType::FunctionDeclaration;
  inline NodeType type() const override { return type_code; }
};

struct LIBSCRIPT_API MemberInitialization
{
  std::shared_ptr<Identifier> name;
  std::shared_ptr<Initialization> init;

  MemberInitialization(const std::shared_ptr<Identifier> & n, const std::shared_ptr<Initialization> & init);
};

class LIBSCRIPT_API ConstructorDecl : public FunctionDecl
{
public:
  std::vector<MemberInitialization> memberInitializationList;

public:
  ConstructorDecl(const std::shared_ptr<Identifier> & name);
  ~ConstructorDecl() = default;

  static std::shared_ptr<ConstructorDecl> New(const std::shared_ptr<Identifier> & name);

  static const NodeType type_code = NodeType::ConstructorDeclaration;
  inline NodeType type() const override { return type_code; }
};

class LIBSCRIPT_API DestructorDecl : public FunctionDecl
{
public:
  parser::Token tilde;

public:
  DestructorDecl(const std::shared_ptr<Identifier> & name);
  ~DestructorDecl() = default;

  static std::shared_ptr<DestructorDecl> New(const std::shared_ptr<Identifier> & name);

  static const NodeType type_code = NodeType::DestructorDeclaration;
  inline NodeType type() const override { return type_code; }
};

class LIBSCRIPT_API OperatorOverloadDecl : public FunctionDecl
{
public:
  OperatorOverloadDecl(const std::shared_ptr<Identifier> & name);
  ~OperatorOverloadDecl() = default;

  static std::shared_ptr<OperatorOverloadDecl> New(const std::shared_ptr<Identifier> & name);

  static const NodeType type_code = NodeType::OperatorOverloadDeclaration;
  inline NodeType type() const override { return type_code; }
};

class LIBSCRIPT_API CastDecl : public FunctionDecl
{
public:
  parser::Token operatorKw;

public:
  CastDecl(const QualifiedType & rt);
  ~CastDecl() = default;

  static std::shared_ptr<CastDecl> New(const QualifiedType & rt);

  static const NodeType type_code = NodeType::CastDeclaration;
  inline NodeType type() const override { return type_code; }
};

struct LIBSCRIPT_API LambdaCapture
{
  parser::Token reference;
  parser::Token byValueSign;
  parser::Token name;
  parser::Token assignmentSign;
  std::shared_ptr<Expression> value;
};

class LIBSCRIPT_API LambdaExpression : public Expression
{
public:
  parser::Token leftBracket;
  std::vector<LambdaCapture> captures;
  parser::Token rightBracket;
  parser::Token leftPar;
  std::vector<FunctionParameter> params;
  parser::Token rightPar;
  std::shared_ptr<CompoundStatement> body;

public:
  LambdaExpression(const parser::Token & lb);
  ~LambdaExpression() = default;

  std::string parameterName(int index) const;


  static std::shared_ptr<LambdaExpression> New(const parser::Token & lb);

  static const NodeType type_code = NodeType::LambdaExpression;
  inline NodeType type() const override { return type_code; }

  parser::Token base_token() const override { return this->leftBracket; }
};

class LIBSCRIPT_API Typedef : public Declaration
{
public:
  parser::Token typedef_token;
  QualifiedType qualified_type;
  std::shared_ptr<ast::SimpleIdentifier> name;

public:
  Typedef(const parser::Token & typedef_tok, const QualifiedType & qtype, const std::shared_ptr<ast::SimpleIdentifier> & n);
  ~Typedef() = default;

  static std::shared_ptr<Typedef> New(const parser::Token & typedef_tok, const QualifiedType & qtype, const std::shared_ptr<ast::SimpleIdentifier> & n);

  static const NodeType type_code = NodeType::Typedef;
  inline NodeType type() const override { return type_code; }

  parser::Token base_token() const override { return name->base_token(); }
};

class LIBSCRIPT_API NamespaceDeclaration : public Declaration
{
public:
  parser::Token namespace_token;
  std::shared_ptr<SimpleIdentifier> namespace_name;
  parser::Token left_brace;
  std::vector<std::shared_ptr<Statement>> statements;
  parser::Token right_brace;

public:
  NamespaceDeclaration(const parser::Token & ns_tok, const std::shared_ptr<ast::SimpleIdentifier> & n, const parser::Token & lb, std::vector<std::shared_ptr<Statement>> && stats, const parser::Token & rb);
  ~NamespaceDeclaration() = default;

  static std::shared_ptr<NamespaceDeclaration> New(const parser::Token & ns_tok, const std::shared_ptr<ast::SimpleIdentifier> & n, const parser::Token & lb, std::vector<std::shared_ptr<Statement>> && stats, const parser::Token & rb);

  static const NodeType type_code = NodeType::NamespaceDecl;
  inline NodeType type() const override { return type_code; }

  parser::Token base_token() const override { return namespace_name->base_token(); }
};


class LIBSCRIPT_API FriendDeclaration : public Declaration
{
public:
  parser::Token friend_token;

public:
  FriendDeclaration(const parser::Token & friend_token);
  ~FriendDeclaration() = default;

  parser::Token base_token() const override { return friend_token; }
};

class LIBSCRIPT_API ClassFriendDeclaration : public FriendDeclaration
{
public:
  parser::Token class_token;
  std::shared_ptr<Identifier> class_name;

public:
  ClassFriendDeclaration(const parser::Token & friend_tok, const parser::Token & class_tok, const std::shared_ptr<Identifier> & cname);
  ~ClassFriendDeclaration() = default;

  static std::shared_ptr<ClassFriendDeclaration> New(const parser::Token & friend_tok, const parser::Token & class_tok, const std::shared_ptr<Identifier> & cname);

  static const NodeType type_code = NodeType::ClassFriendDecl;
  inline NodeType type() const override { return type_code; }
};

class LIBSCRIPT_API UsingDeclaration : public Declaration
{
public:
  parser::Token using_keyword;
  std::shared_ptr<ScopedIdentifier> used_name;

public:
  UsingDeclaration(const parser::Token & using_tok, const std::shared_ptr<ScopedIdentifier> & name);
  ~UsingDeclaration() = default;

  static std::shared_ptr<UsingDeclaration> New(const parser::Token & using_tok, const std::shared_ptr<ScopedIdentifier> & name);

  static const NodeType type_code = NodeType::UsingDeclaration;
  inline NodeType type() const override { return type_code; }

  parser::Token base_token() const override { return used_name->base_token(); }
};

class LIBSCRIPT_API UsingDirective : public Declaration
{
public:
  parser::Token using_keyword;
  parser::Token namespace_keyword;
  std::shared_ptr<Identifier> namespace_name;

public:
  UsingDirective(const parser::Token & using_tok, const parser::Token & namespace_tok, const std::shared_ptr<Identifier> & name);
  ~UsingDirective() = default;

  static std::shared_ptr<UsingDirective> New(const parser::Token & using_tok, const parser::Token & namespace_tok, const std::shared_ptr<Identifier> & name);

  static const NodeType type_code = NodeType::UsingDirective;
  inline NodeType type() const override { return type_code; }

  parser::Token base_token() const override { return namespace_name->base_token(); }
};

class LIBSCRIPT_API NamespaceAliasDefinition : public Declaration
{
public:
  parser::Token namespace_keyword;
  std::shared_ptr<SimpleIdentifier> alias_name;
  parser::Token equal_token;
  std::shared_ptr<Identifier> aliased_namespace;

public:
  NamespaceAliasDefinition(const parser::Token & namespace_tok, const std::shared_ptr<SimpleIdentifier> & a, const parser::Token & equal_tok, const std::shared_ptr<Identifier> & b);
  ~NamespaceAliasDefinition() = default;

  static std::shared_ptr<NamespaceAliasDefinition> New(const parser::Token & namespace_tok, const std::shared_ptr<SimpleIdentifier> & a, const parser::Token & equal_tok, const std::shared_ptr<Identifier> & b);

  static const NodeType type_code = NodeType::NamespaceAliasDef;
  inline NodeType type() const override { return type_code; }

  parser::Token base_token() const override { return alias_name->base_token(); }
};

class LIBSCRIPT_API TypeAliasDeclaration : public Declaration
{
public:
  parser::Token using_keyword;
  std::shared_ptr<SimpleIdentifier> alias_name;
  parser::Token equal_token;
  std::shared_ptr<Identifier> aliased_type;

public:
  TypeAliasDeclaration(const parser::Token & using_tok, const std::shared_ptr<SimpleIdentifier> & a, const parser::Token & equal_tok, const std::shared_ptr<Identifier> & b);
  ~TypeAliasDeclaration() = default;

  static std::shared_ptr<TypeAliasDeclaration> New(const parser::Token & using_tok, const std::shared_ptr<SimpleIdentifier> & a, const parser::Token & equal_tok, const std::shared_ptr<Identifier> & b);

  static const NodeType type_code = NodeType::TypeAliasDecl;
  inline NodeType type() const override { return type_code; }

  parser::Token base_token() const override { return alias_name->base_token(); }
};

class LIBSCRIPT_API ImportDirective : public Declaration
{
public:
  parser::Token export_keyword;
  parser::Token import_keyword;
  std::vector<parser::Token> names;

public:
  ImportDirective(const parser::Token & exprt, const parser::Token & imprt, std::vector<parser::Token> && nms);
  ~ImportDirective() = default;

  inline size_t size() const { return names.size(); }
  std::string at(size_t i) const;

  static std::shared_ptr<ImportDirective> New(const parser::Token & exprt, const parser::Token & imprt, std::vector<parser::Token> && nms);

  static const NodeType type_code = NodeType::ImportDirective;
  inline NodeType type() const override { return type_code; }

  parser::Token base_token() const override { return names.front(); }
};

class LIBSCRIPT_API TemplateParameter
{
public:
  parser::Token kind;
  parser::Token name;
  parser::Token eq;
  std::shared_ptr<Node> default_value;
};

class LIBSCRIPT_API TemplateDeclaration : public Declaration
{
public:
  parser::Token template_keyword;
  parser::Token left_angle_bracket;
  std::vector<TemplateParameter> parameters;
  parser::Token right_angle_bracket;
  std::shared_ptr<Declaration> declaration;

public:
  TemplateDeclaration(const parser::Token & tmplt_k, const parser::Token & left_angle_b, std::vector<TemplateParameter> && params, const parser::Token & right_angle_b, const std::shared_ptr<Declaration> & decl);
  ~TemplateDeclaration() = default;

  inline size_t size() const { return parameters.size(); }
  std::string parameter_name(size_t i) const;
  const TemplateParameter & at(size_t i) const;

  bool is_class_template() const;

  bool is_full_specialization() const;
  bool is_partial_specialization() const;

  static std::shared_ptr<TemplateDeclaration> New(const parser::Token & tmplt_k, const parser::Token & left_angle_b, std::vector<TemplateParameter> && params, const parser::Token & right_angle_b, const std::shared_ptr<Declaration> & decl);

  static const NodeType type_code = NodeType::TemplateDecl;
  inline NodeType type() const override { return type_code; }

  parser::Token base_token() const override { return template_keyword; }
};

class LIBSCRIPT_API ScriptRootNode : public Node
{
public:
  std::vector<std::shared_ptr<Statement>> statements;
  std::vector<std::shared_ptr<Declaration>> declarations;
  std::weak_ptr<AST> ast;

public:
  ScriptRootNode(const std::shared_ptr<AST> & st);
  ~ScriptRootNode() = default;

  static std::shared_ptr<ScriptRootNode> New(const std::shared_ptr<AST> & syntaxtree);

  static const NodeType type_code = NodeType::ScriptRoot;
  inline NodeType type() const override { return type_code; }

  inline parser::Token base_token() const override { return parser::Token(); }
};

} // namespace ast

} // namespace script

#endif // LIBSCRIPT_AST_NODE_H
