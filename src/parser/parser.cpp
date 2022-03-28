// Copyright (C) 2018-2022 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/parser/parser.h"
#include "script/parser/specific-parsers.h"

#include "script/parser/lexer.h"

#include "script/parser/delimiters-counter.h"

namespace script
{

namespace parser
{

ParserContext::ParserContext(const char* src)
  : ParserContext(src, std::strlen(src))
{

}

ParserContext::ParserContext(const std::string& str)
  : ParserContext(str.data(), str.size())
{

}

ParserContext::ParserContext(const char* src, size_t s)
  : m_source(src),
    m_size(0)
{
  m_tokens = parser::tokenize(src, s);
}

ParserContext::ParserContext(const char* src, std::vector<Token> tokens)
  : m_source(src),
    m_size(0),
    m_tokens(std::move(tokens))
{

}

ParserContext::~ParserContext()
{

}

ParserBase::ParserBase(std::shared_ptr<ParserContext> shared_context, const TokenReader& reader)
  : TokenReader(reader),
    m_context(shared_context)
{

}

ParserBase::~ParserBase()
{

}

void ParserBase::reset(std::shared_ptr<ParserContext> shared_context, const TokenReader& reader)
{
  m_context = shared_context;
  static_cast<TokenReader&>(*this) = reader;
}

size_t ParserBase::offset() const
{
  return atEnd() ? context()->source_length() : (unsafe_peek().text().data() - context()->source());
}

bool ParserBase::atEnd() const
{
  return m_iterator == m_fragment.end();
}

const Fragment& ParserBase::fragment() const
{
  return m_fragment;
}

const std::shared_ptr<ParserContext>& ParserBase::context() const
{
  return m_context;
}

Fragment::iterator ParserBase::iterator() const
{
  return m_iterator;
}

LiteralParser::LiteralParser(std::shared_ptr<ParserContext> shared_context, const TokenReader& reader)
  : ParserBase(shared_context, reader)
{

}

std::shared_ptr<ast::Literal> LiteralParser::parse()
{
  Token lit = read();
  assert(lit.isLiteral());
  switch (lit.id)
  {
  case Token::True:
  case Token::False:
    return ast::BoolLiteral::New(lit);
  case Token::IntegerLiteral:
  case Token::BinaryLiteral:
  case Token::OctalLiteral:
  case Token::HexadecimalLiteral:
    return ast::IntegerLiteral::New(lit);
  case Token::DecimalLiteral:
    return ast::FloatingPointLiteral::New(lit);
  case Token::StringLiteral:
    return ast::StringLiteral::New(lit);
  case Token::UserDefinedLiteral:
    return ast::UserDefinedLiteral::New(lit);
  default:
    break;
  }

  throw SyntaxError{ ParserError::ExpectedLiteral, errors::ActualToken{lit} };
}

ExpressionParser::ExpressionParser(std::shared_ptr<ParserContext> shared_context, const TokenReader& reader)
  : ParserBase(shared_context, reader)
{

}

std::shared_ptr<ast::Expression> ExpressionParser::parse()
{
  std::vector<Token> operators;
  std::vector<std::shared_ptr<ast::Expression>> operands;

  {
    auto operand = readOperand();
    operands.push_back(operand);
  }


  while (!atEnd())
  {
    Token operation = readBinaryOperator();
    operators.push_back(operation);

    {
      auto operand = readOperand();
      operands.push_back(operand);
    }
  }

  return buildExpression(operands, operators);
}


bool ExpressionParser::isPrefixOperator(const Token & tok)
{
  auto op = ast::OperatorName::getOperatorId(tok, ast::OperatorName::PrefixOp);
  return op != OperatorName::InvalidOperator;
}

bool ExpressionParser::isInfixOperator(const Token & tok)
{
  auto op = ast::OperatorName::getOperatorId(tok, ast::OperatorName::InfixOp);
  return op != OperatorName::InvalidOperator;
}

std::shared_ptr<ast::Expression> ExpressionParser::readOperand()
{
  if (atEnd())
    throw SyntaxError{ ParserError::UnexpectedFragmentEnd };

  auto pos_backup = iterator();
  Token t = unsafe_peek();

  std::shared_ptr<ast::Expression> operand = nullptr;

  if (t.isOperator())
  {
    if (!isPrefixOperator(t))
      throw SyntaxError{ ParserError::ExpectedPrefixOperator, errors::ActualToken{t} };

    read();
    operand = readOperand();
    operand = ast::Operation::New(t, operand);
  }
  else if (t == Token::LeftPar) 
  {
    if (peek(1) == Token::RightPar) // we just read '()'
      throw SyntaxError{ ParserError::InvalidEmptyOperand };

    ExpressionParser exprParser{ context(), next<Fragment::DelimiterPair>() };
    operand = exprParser.parse();
    read(Token::RightPar);
  }
  else if (t == Token::LeftBracket) // array
  {
    LambdaParser lambda_parser{ context(), subfragment() };
    operand = parse_and_seek(lambda_parser);
  }
  else if (t == Token::LeftBrace)
  {
    auto list = ast::ListExpression::New(unsafe_peek());
    TokenReader list_reader = next<Fragment::DelimiterPair>();

    while (!list_reader.atEnd())
    {
      ExpressionParser exprparser{ context(), list_reader.next<Fragment::ListElement>() };
      list->elements.push_back(exprparser.parse());

      if (!list_reader.atEnd())
        list_reader.read(Token::Comma);
    }

    list->right_brace = read(Token::RightBrace);

    operand = list;
  }
  else if (t.isLiteral())
  {
    LiteralParser p{ context(), subfragment() };
    operand = parse_and_seek(p);
  }
  else
  {
    IdentifierParser idParser{ context(), subfragment() };
    operand = parse_and_seek(idParser);

    /// TODO : handle static_cast and other built-int constructions here..
  }

  while (!atEnd())
  {
    t = peek();
    if (t == Token::PlusPlus || t == Token::MinusMinus)
    {
      operand = ast::Operation::New(t, operand, nullptr);
      read();
    }
    else if (t == Token::Dot)
    {
      unsafe_read();
      IdentifierParser idParser{ context(), subfragment(), IdentifierParser::ParseSimpleId | IdentifierParser::ParseTemplateId };
      auto memberName = parse_and_seek(idParser);
      operand = ast::Operation::New(t, operand, memberName);
    }
    else if (t == Token::LeftPar)
    {
      const Token leftpar = unsafe_peek();
      ExpressionListParser argsParser{ context(), next<Fragment::DelimiterPair>() };
      std::vector<std::shared_ptr<ast::Expression>> args = argsParser.parse();
      const Token rightpar = read(Token::RightPar);
      operand = ast::FunctionCall::New(operand, leftpar, std::move(args), rightpar);
    }
    else if (t == Token::LeftBracket) // subscript operator
    {
      TokenReader subscript_reader = subfragment<Fragment::DelimiterPair>();
      auto leftBracket = read();

      if (subscript_reader.begin() == subscript_reader.end())
        throw SyntaxError{ ParserError::InvalidEmptyBrackets };

      ExpressionParser exprParser{ context(), subscript_reader };
      std::shared_ptr<ast::Expression> arg = parse_and_seek(exprParser);
      operand = ast::ArraySubscript::New(operand, leftBracket, arg, unsafe_read());
    }
    else if (t == Token::LeftBrace && operand->is<ast::Identifier>())
    {
      TokenReader bracelist_reader = subfragment<Fragment::DelimiterPair>();

      auto type_name = std::dynamic_pointer_cast<ast::Identifier>(operand);
      const Token left_brace = unsafe_read();
      
      ExpressionListParser argsParser{ context(), bracelist_reader };
      std::vector<std::shared_ptr<ast::Expression>> args = parse_and_seek(argsParser);
      const Token right_brace = read(Token::RightBrace);
      operand = ast::BraceConstruction::New(type_name, left_brace, std::move(args), right_brace);
    }
    else if (t.isOperator() || t == Token::QuestionMark || t == Token::Colon)
    {
      break;
    }
    else
    {
      if (operand->is<ast::TemplateIdentifier>())
      {
        // template identifiers cannot be used as operand
        seek(pos_backup);
        IdentifierParser idParser{ context(), subfragment() };
        idParser.setOptions(IdentifierParser::ParseOperatorName | IdentifierParser::ParseQualifiedId);
        operand = parse_and_seek(idParser);
        continue;
      }

      throw SyntaxError{ ParserError::UnexpectedToken, errors::UnexpectedToken{t, Token::Invalid} };
    }
  }

  return operand;
}

Token ExpressionParser::readBinaryOperator()
{
  assert(!atEnd());
  Token t = peek();

  if (t == Token::QuestionMark || t == Token::Colon)
    return read();

  if (!t.isOperator())
    throw SyntaxError{ ParserError::ExpectedOperator, errors::ActualToken{t} };
  else if (!isInfixOperator(t))
    throw SyntaxError{ ParserError::ExpectedBinaryOperator, errors::ActualToken{t} };

  return read();
}

std::shared_ptr<ast::Expression> ExpressionParser::buildExpression(const std::vector<std::shared_ptr<ast::Expression>> & operands, const std::vector<Token> & operators)
{
  if (operands.size() == 1)
    return operands.front();

  return buildExpression(operands.begin(), operands.end(), operators.begin(), operators.end());
}

std::shared_ptr<ast::Expression> ExpressionParser::buildExpression(std::vector<std::shared_ptr<ast::Expression>>::const_iterator exprBegin,
  std::vector<std::shared_ptr<ast::Expression>>::const_iterator exprEnd,
  std::vector<Token>::const_iterator opBegin,
  std::vector<Token>::const_iterator opEnd)
{
  const size_t numOp = std::distance(opBegin, opEnd);
  auto getOp = [opBegin](size_t num) -> Token {
    return *(opBegin + num);
  };

  if (numOp == 0)
  {
    assert(std::distance(exprBegin, exprEnd) == 1);
    return *exprBegin;
  }

  auto getPrecedence = [opBegin](const Token & tok) -> int {
    if (tok == Token::Colon)
      return -66;
    else if (tok == Token::QuestionMark)
      return script::precedence(ConditionalOperator);
    else
      return script::precedence(ast::OperatorName::getOperatorId(tok, ast::OperatorName::InfixOp));
  };

  size_t index = 0;
  int preced = getPrecedence(getOp(index));
  for (size_t i(1); i < numOp; ++i)
  {
    int p = getPrecedence(getOp(i));
    if (p > preced)
    {
      index = i;
      preced = p;
    }
    else if (p == preced)
    {
      if (script::associativity(preced) == Associativity::LeftToRight)
        index = i;
    }
  }

  if (getOp(index) == Token::QuestionMark)
  {
    auto cond = buildExpression(exprBegin, exprBegin + (index + 1), opBegin, opBegin + index);

    size_t colonIndex = std::numeric_limits<size_t>::max();
    for (size_t j = numOp - 1; j > index; --j)
    {
      if (getOp(j) == Token::Colon)
      {
        colonIndex = j;
        break;
      }
    }

    if (colonIndex == std::numeric_limits<size_t>::max())
      throw SyntaxError{ ParserError::MissingConditionalColon };

    auto onTrue = buildExpression(exprBegin + (index + 1), exprBegin + (colonIndex + 1), opBegin + (index + 1), opBegin + colonIndex);
    auto onFalse = buildExpression(exprBegin + (colonIndex + 1), exprEnd, opBegin + (colonIndex + 1), opEnd);
    
    return ast::ConditionalExpression::New(cond, getOp(index), onTrue, getOp(colonIndex), onFalse);
  }
  else
  {
    auto lhs = buildExpression(exprBegin, exprBegin + (index + 1), opBegin, opBegin + index);
    auto rhs = buildExpression(exprBegin + (index + 1), exprEnd, opBegin + (index + 1), opEnd);
    return ast::Operation::New(getOp(index), lhs, rhs);
  }
}


LambdaParser::LambdaParser(std::shared_ptr<ParserContext> shared_context, const TokenReader& reader)
  : ParserBase(shared_context, reader)
{

}

std::shared_ptr<ast::Expression> LambdaParser::parse()
{
  TokenReader bracket_content = subfragment<Fragment::DelimiterPair>();

  if (detect(bracket_content.fragment()) == ParsingArray)
  {
    return parseArray(bracket_content);
  }
  else
  {
    mLambda = ast::LambdaExpression::New(unsafe_peek());
    readCaptures(bracket_content);
    readParams();
    mLambda->body = readBody();
    return mLambda;
  }
}

LambdaParser::Decision LambdaParser::detect(const Fragment& frag) const
{
  auto it = frag.end();
  assert(it->id == Token::RightBracket);
  ++it;
  
  if (it == context()->tokens().end() || it->id != Token::LeftPar)
    return Decision::ParsingArray;
  else
    return Decision::ParsingLambda;
}

std::shared_ptr<ast::Expression> LambdaParser::parseArray(TokenReader& bracket_content)
{
  std::shared_ptr<ast::ArrayExpression> result = ast::ArrayExpression::New(read(Token::LeftBracket));

  while (!bracket_content.atEnd())
  {
    TokenReader listelem_reader = bracket_content.next<Fragment::ListElement>();

    ExpressionParser ep{ context(), listelem_reader };
    result->elements.push_back(ep.parse());

    if (!bracket_content.atEnd())
      bracket_content.read(Token::Comma);
  }

  seek(bracket_content.end());
  result->rightBracket = unsafe_read();

  return result;
}

void LambdaParser::readCaptures(TokenReader& bracket_content)
{
  read(Token::LeftBracket);

  while (!bracket_content.atEnd())
  {
    TokenReader listelem_reader = bracket_content.next<Fragment::ListElement>();

    LambdaCaptureParser capp{ context(), listelem_reader };

    if (!capp.detect())
      throw SyntaxError{ ParserError::CouldNotParseLambdaCapture };

    auto capture = capp.parse();
    mLambda->captures.push_back(capture);

    if (!bracket_content.atEnd())
      bracket_content.read(Token::Comma);
  }

  seek(bracket_content.end());
  const Token rb = unsafe_read();

  mLambda->rightBracket = rb;
}

void LambdaParser::readParams()
{
  assert(mLambda != nullptr);

  TokenReader params_reader = next<Fragment::DelimiterPair>();

  mLambda->leftPar = *(params_reader.begin() - 1);

  while (!params_reader.atEnd())
  {
    FunctionParamParser pp{ context(), params_reader.next<Fragment::ListElement>() };
    auto param = pp.parse();
    mLambda->params.push_back(param);

    if (!params_reader.atEnd())
      params_reader.read(Token::Comma);
  }

  seek(params_reader.end());
  mLambda->rightPar = unsafe_read();
}


std::shared_ptr<ast::CompoundStatement> LambdaParser::readBody()
{
  if (atEnd())
    throw SyntaxError{ ParserError::UnexpectedEndOfInput };

  if (peek() != Token::LeftBrace)
    throw SyntaxError{ ParserError::UnexpectedToken, errors::UnexpectedToken{unsafe_peek(), Token::LeftBrace} };

  ProgramParser pParser{ context(), subfragment() };
  auto ret = std::dynamic_pointer_cast<ast::CompoundStatement>(pParser.parseStatement());
  seek(pParser.iterator());
  return ret;
}


LambdaCaptureParser::LambdaCaptureParser(std::shared_ptr<ParserContext> shared_context, const TokenReader& reader)
  : ParserBase(shared_context, reader)
{

}

bool LambdaCaptureParser::detect() const
{
  if (peek() == Token::Eq || unsafe_peek() == Token::Ref)
    return true;
  return unsafe_peek() == Token::UserDefinedName;
}

ast::LambdaCapture LambdaCaptureParser::parse()
{
  if (atEnd())
    throw SyntaxError{ ParserError::UnexpectedFragmentEnd };

  ast::LambdaCapture cap;

  if (peek() == Token::Eq) {
    cap.byValueSign = read();
    if (!atEnd())
      throw SyntaxError{ ParserError::UnexpectedToken, errors::UnexpectedToken{cap.byValueSign, Token::RightBracket} };
    return cap;
  }
  else if (peek() == Token::Ref) {
    cap.reference = read();
    if (atEnd())
      return cap;
  }

  IdentifierParser idpar{ context(), subfragment(), IdentifierParser::ParseOnlySimpleId };
  cap.name = parse_and_seek(idpar)->as<ast::SimpleIdentifier>().name;
  if (atEnd())
    return cap;
  cap.assignmentSign = read(Token::Eq);
  ExpressionParser ep{ context(), subfragment() };
  cap.value = parse_and_seek(ep);
  return cap;
}



ProgramParser::ProgramParser(std::shared_ptr<ParserContext> shared_context)
  : ProgramParser(shared_context, TokenReader{shared_context->source(), Fragment(shared_context->tokens())})
{

}

ProgramParser::ProgramParser(std::shared_ptr<ParserContext> shared_context, const TokenReader& reader)
  : ParserBase(shared_context, reader)
{

}

std::shared_ptr<ast::Statement> ProgramParser::parse()
{
  return parseStatement();
}

std::vector<std::shared_ptr<ast::Statement>> ProgramParser::parseProgram()
{
  std::vector<std::shared_ptr<ast::Statement>> ret;

  while (!atEnd())
  {
    auto statement = parseStatement();
    ret.push_back(statement);
  }

  return ret;
}

std::shared_ptr<ast::Statement> ProgramParser::parseStatement()
{
  Token t = peek();
  switch (t.id)
  {
  case Token::Semicolon:
    return ast::NullStatement::New(read());
  case Token::Break:
    return parseBreakStatement();
  case Token::Class:
  case Token::Struct:
    return parseClassDeclaration();
  case Token::Continue:
    return parseContinueStatement();
  case Token::Enum:
    return parseEnumDeclaration();
  case Token::If:
    return parseIfStatement();
  case Token::Return:
    return parseReturnStatement();
  case Token::Using:
    return parseUsing();
  case Token::While:
    return parseWhileLoop();
  case Token::For:
    return parseForLoop();
  case Token::LeftBrace:
    return parseCompoundStatement();
  case Token::Template:
    return parseTemplate();
  case Token::Typedef:
    return parseTypedef();
  case Token::Namespace:
    return parseNamespace();
  case Token::Friend:
    throw SyntaxError{ ParserError::IllegalUseOfKeyword, errors::KeywordToken{t} };
  case Token::Export:
  case Token::Import:
    return parseImport();
  default:
    break;
  }

  return parseAmbiguous();
}

std::shared_ptr<ast::Statement> ProgramParser::parseAmbiguous()
{
  auto savePoint = iterator();
  DeclParser dp{ context(), subfragment() };
  if (dp.detectDecl())
    return parse_and_seek(dp);
  
  seek(savePoint);

  ExpressionParser ep{ context(), subfragment<Fragment::Statement>() };
  auto expr = parse_and_seek(ep);
  auto semicolon = read(Token::Semicolon);
  return ast::ExpressionStatement::New(expr, semicolon);
}

std::shared_ptr<ast::ClassDecl> ProgramParser::parseClassDeclaration()
{
  ClassParser cp{ context(), subfragment() };
  return parse_and_seek(cp);
}

std::shared_ptr<ast::EnumDeclaration> ProgramParser::parseEnumDeclaration()
{
  EnumParser ep{ context(), subfragment() };
  return parse_and_seek(ep);
}

std::shared_ptr<ast::BreakStatement> ProgramParser::parseBreakStatement()
{
  Token kw = read();
  assert(kw == Token::Break);
  read(Token::Semicolon);
  return ast::BreakStatement::New(kw);
}

std::shared_ptr<ast::ContinueStatement> ProgramParser::parseContinueStatement()
{
  Token kw = read();
  assert(kw == Token::Continue);
  read(Token::Semicolon);
  return ast::ContinueStatement::New(kw);
}

std::shared_ptr<ast::ReturnStatement> ProgramParser::parseReturnStatement()
{
  Token kw = read();
  assert(kw == Token::Return);
  Token next = peek();
  if (next == Token::Semicolon)
  {
    unsafe_read();
    return ast::ReturnStatement::New(kw);
  }

  ExpressionParser exprParser{ context(), subfragment<Fragment::Statement>() };
  auto returnValue = parse_and_seek(exprParser);

  Token semicolon = read();
  assert(semicolon == Token::Semicolon);

  return ast::ReturnStatement::New(kw, returnValue);
}

std::shared_ptr<ast::CompoundStatement> ProgramParser::parseCompoundStatement()
{
  TokenReader compound_reader = next<Fragment::DelimiterPair>();

  Token leftBrace = *(compound_reader.begin() - 1);
  assert(leftBrace == Token::LeftBrace);

  ProgramParser progParser{ context(), compound_reader };

  std::vector<std::shared_ptr<ast::Statement>> statements = progParser.parseProgram();
  seek(compound_reader.end());
  Token rightBrace = read(Token::RightBrace);
  assert(rightBrace == Token::RightBrace);

  auto ret = ast::CompoundStatement::New(leftBrace, rightBrace);
  ret->statements = std::move(statements);
  return ret;
}

std::shared_ptr<ast::IfStatement> ProgramParser::parseIfStatement()
{
  Token ifkw = read();
  assert(ifkw == Token::If);

  auto ifStatement = ast::IfStatement::New(ifkw);

  {
    ExpressionParser exprParser{ context(), subfragment<Fragment::DelimiterPair>() };
    ifStatement->condition = parse_and_seek(exprParser);
    read(Token::RightPar);
  }

  ifStatement->body = parseStatement();

  if (atEnd() || unsafe_peek() != Token::Else)
    return ifStatement;

  ifStatement->elseKeyword = read();
  ifStatement->elseClause = parseStatement();

  return ifStatement;
}

std::shared_ptr<ast::WhileLoop> ProgramParser::parseWhileLoop()
{
  Token whilekw = read();
  assert(whilekw == Token::While);

  auto whileLoop = ast::WhileLoop::New(whilekw);

  {
    ExpressionParser exprParser{ context(), subfragment<Fragment::DelimiterPair>() };
    whileLoop->condition = parse_and_seek(exprParser);
    read(Token::RightPar);
  }

  whileLoop->body = parseStatement();

  return whileLoop;
}

std::shared_ptr<ast::ForLoop> ProgramParser::parseForLoop()
{
  Token forkw = read();
  assert(forkw == Token::For);

  TokenReader init_loop_incr_reader = subfragment<Fragment::DelimiterPair>();

  auto forLoop = ast::ForLoop::New(forkw);

  // @TODO: handle "for(;;)"

  {
    DeclParser initParser{ context(), init_loop_incr_reader };
    if (!initParser.detectDecl())
    {
      TokenReader reader = init_loop_incr_reader.next<Fragment::Statement>();
      
      if (!reader.atEnd())
      {
        ExpressionParser exprParser{ context(), reader };
        auto initExpr = exprParser.parse();
        const Token semicolon = init_loop_incr_reader.read(Token::Semicolon);
        forLoop->initStatement = ast::ExpressionStatement::New(initExpr, semicolon);
      }
      else
      {
        init_loop_incr_reader.read(Token::Semicolon);
      }
    }
    else
    {
      initParser.setDecision(DeclParser::ParsingVariable);
      forLoop->initStatement = initParser.parse();
      init_loop_incr_reader.seek(initParser.iterator());
    }
  }

  {
    TokenReader reader = init_loop_incr_reader.next<Fragment::Statement>();

    if (!reader.atEnd())
    {
      ExpressionParser exprParser{ context(), reader };
      forLoop->condition = exprParser.parse();
    }

    init_loop_incr_reader.read(Token::Semicolon);
  }

  {
    TokenReader reader = init_loop_incr_reader.subfragment();

    if (!reader.atEnd())
    {
      ExpressionParser exprParser{ context(), init_loop_incr_reader.subfragment() };
      forLoop->loopIncrement = parse_and_seek(exprParser);
    }
  }

  seek(init_loop_incr_reader.end() + 1);

  forLoop->body = parseStatement();

  return forLoop;
}

std::shared_ptr<ast::Typedef> ProgramParser::parseTypedef()
{
  TypedefParser tp{ context(), subfragment() };
  auto ret = parse_and_seek(tp);
  return ret;
}

std::shared_ptr<ast::Declaration> ProgramParser::parseNamespace()
{
  NamespaceParser np{ context(), subfragment() };
  return parse_and_seek(np);
}

std::shared_ptr<ast::Declaration> ProgramParser::parseUsing()
{
  UsingParser np{ context(), subfragment() };
  return parse_and_seek(np);
}

std::shared_ptr<ast::ImportDirective> ProgramParser::parseImport()
{
  ImportParser ip{ context(), subfragment() };
  return parse_and_seek(ip);
}

std::shared_ptr<ast::TemplateDeclaration> ProgramParser::parseTemplate()
{
  TemplateParser tp{ context(), subfragment() };
  return parse_and_seek(tp);
}



IdentifierParser::IdentifierParser(std::shared_ptr<ParserContext> shared_context, const TokenReader& reader, int opts)
  : ParserBase(shared_context, reader)
  , mOptions(opts)
{

}

bool IdentifierParser::lookAhead() const
{
  Token t = peek();

  switch (t.id)
  {
  case Token::Void:
  case Token::Bool:
  case Token::Char:
  case Token::Int:
  case Token::Float:
  case Token::Double:
  case Token::Auto:
  case Token::This:
  case Token::Operator:
  case Token::UserDefinedName:
    return true;
  default:
    return false;
  }
}

std::shared_ptr<ast::Identifier> IdentifierParser::parse()
{
  Token t = peek();
  switch (t.id)
  {
  case Token::Void:
  case Token::Bool:
  case Token::Char:
  case Token::Int:
  case Token::Float:
  case Token::Double:
  case Token::Auto:
  case Token::This:
    return ast::SimpleIdentifier::New(unsafe_read());
  case Token::Operator:
    return readOperatorName();
  case Token::UserDefinedName:
    return readUserDefinedName();
  default:
    break;
  }

  throw SyntaxError{ ParserError::ExpectedIdentifier, errors::ActualToken{t} };
}

std::shared_ptr<ast::Identifier> IdentifierParser::readOperatorName()
{
  if (!testOption(ParseOperatorName))
    throw SyntaxError{ ParserError::UnexpectedToken, errors::UnexpectedToken{peek(), Token::Invalid} };

  Token opkw = read();
  if (atEnd())
    throw SyntaxError{ ParserError::UnexpectedEndOfInput };

  Token op = peek();
  if (op.isOperator())
  {
    return ast::OperatorName::New(opkw, read());
  }
  else if (op == Token::LeftPar)
  {
    const Token lp = read();
    if (atEnd())
      throw SyntaxError{ ParserError::UnexpectedEndOfInput };
    const Token rp = read(Token::RightPar);
    if (lp.text().data() + 1 != rp.text().data())
      throw SyntaxError{ ParserError::UnexpectedToken, errors::UnexpectedToken{lp, Token::LeftRightPar} };
    return ast::OperatorName::New(opkw, Token{ Token::LeftRightPar, 0, StringView(lp.text().data(), 2) });
  }
  else if (op == Token::LeftBracket)
  {
    const Token lb = read();
    const Token rb = read(Token::RightBracket);
    if (lb.text().data() + 1 != rb.text().data())
      throw SyntaxError{ ParserError::UnexpectedToken, errors::UnexpectedToken{lb, Token::LeftRightBracket} };
    return ast::OperatorName::New(opkw, Token{ Token::LeftRightBracket, 0, StringView(lb.text().data(), 2) });
  }
  else if (op == Token::StringLiteral)
  {
    if (op.text().size() != 2)
      throw SyntaxError{ ParserError::ExpectedEmptyStringLiteral, errors::ActualToken{op} };

    op = read();
    IdentifierParser idp{ context(), subfragment(), IdentifierParser::ParseOnlySimpleId };
    /// TODO: add overload to remove this cast
    auto suffixName = std::static_pointer_cast<ast::SimpleIdentifier>(parse_and_seek(idp));
    return ast::LiteralOperatorName::New(opkw, op, suffixName->name);
  }
  else if (op == Token::UserDefinedLiteral)
  {
    op = unsafe_read();

    if(!op.text().starts_with("\"\""))
      throw SyntaxError{ ParserError::ExpectedEmptyStringLiteral, errors::ActualToken{op} }; /// TODO ? should this have a different error than the previous

    Token quotes{ Token::StringLiteral, Token::Literal, StringView(op.text().data(), 2) };
    Token suffixName{ Token::UserDefinedName, Token::Identifier, StringView(op.text().data() + 2, op.text().size() - 2) };
    return ast::LiteralOperatorName::New(opkw, quotes, suffixName);
  }

  throw SyntaxError{ ParserError::ExpectedOperatorSymbol, errors::ActualToken{op} };
}

std::shared_ptr<ast::Identifier> IdentifierParser::readUserDefinedName()
{
  const Token base = read();

  if (base != Token::UserDefinedName)
    throw SyntaxError{ ParserError::ExpectedUserDefinedName, errors::ActualToken{base} };

  if(atEnd())
    return ast::SimpleIdentifier::New(base);

  std::shared_ptr<ast::Identifier> ret = ast::SimpleIdentifier::New(base);

  Token t = peek();
  if ((options() & ParseTemplateId) && t == Token::LeftAngle)
  {
    try 
    {
      TokenReader template_arg_reader = subfragment<Fragment::Template>();

      if (template_arg_reader.valid())
      {
        ret = readTemplateArguments(base, template_arg_reader);

        if (template_arg_reader.end() != fragment().end())
          seek(template_arg_reader.end() + 1);
        else
          seek(template_arg_reader.end());
      }
    }
    catch (const std::exception &)
    {
      return ret;
    }
  }

  if (atEnd())
    return ret;

  t = peek();
  if ((options() & ParseQualifiedId) && t == Token::ScopeResolution)
  {
    std::vector<std::shared_ptr<ast::Identifier>> identifiers;
    identifiers.push_back(ret);

    while (t == Token::ScopeResolution)
    {
      read();

      IdentifierParser idparser{ context(), subfragment(), IdentifierParser::ParseTemplateId };
      identifiers.push_back(parse_and_seek(idparser));

      if (atEnd())
        break;
      else
        t = peek();
    }

    ret = ast::ScopedIdentifier::New(identifiers.begin(), identifiers.end());
  }

  return ret;
}

std::shared_ptr<ast::Identifier> IdentifierParser::readTemplateArguments(const Token& base, TokenReader& reader)
{
  std::vector<ast::NodeRef> args;

  while (!reader.atEnd())
  {
    TemplateArgParser argparser{ context(), reader.next<Fragment::ListElement>() };
    args.push_back(argparser.parse());
    
    if (!reader.atEnd())
      reader.read(Token::Comma);
  }

  return ast::TemplateIdentifier::New(base, std::move(args), *(reader.begin() - 1), *reader.end());
}

TemplateArgParser::TemplateArgParser(std::shared_ptr<ParserContext> shared_context, const TokenReader& reader)
  : ParserBase(shared_context, reader)
{

}

std::shared_ptr<ast::Node> TemplateArgParser::parse()
{
  auto p = iterator();

  {
    TypeParser tp{ context(), subfragment() };
    if (tp.detect())
    {
      try
      {
        auto type = parse_and_seek(tp);
        if (atEnd())
          return ast::TypeNode::New(type);
      }
      catch (const SyntaxError&)
      {

      }
    }
  }

  seek(p);

  ExpressionParser ep{ context(), subfragment() };
  return parse_and_seek(ep);
}



TypeParser::TypeParser(std::shared_ptr<ParserContext> shared_context, const TokenReader& reader)
  : ParserBase(shared_context, reader)
  , mReadFunctionSignature(true)
{

}

ast::QualifiedType TypeParser::parse()
{
  ast::QualifiedType ret;

  if (peek() == Token::Const)
    ret.constQualifier = unsafe_read();

  IdentifierParser idparser{ context(), subfragment() };
  ret.type = parse_and_seek(idparser);

  if (atEnd())
    return ret;

  if (unsafe_peek() == Token::Const)
  {
    ret.constQualifier = unsafe_read();

    if (atEnd())
      return ret;

    if (unsafe_peek() == Token::Ref || unsafe_peek() == Token::RefRef)
      ret.reference = unsafe_read();
  }
  else if (unsafe_peek() == Token::Ref || unsafe_peek() == Token::RefRef)
  {
    ret.reference = unsafe_read();

    if (atEnd())
      return ret;

    if (unsafe_peek() == Token::Const)
      ret.constQualifier = unsafe_read();
  }

  if (atEnd())
    return ret;

  if (mReadFunctionSignature && lookAheadFunctionSignature()) {
    auto save_point = iterator();
    try
    {
      auto fsig = tryReadFunctionSignature(ret);
      return fsig;
    }
    catch (const SyntaxError&)
    {
      seek(save_point);
    }
  }
  return ret;
}


bool TypeParser::detect(Detection opt)
{
  if (opt == LookAheadDetection)
  {
    if (peek() == Token::Const)
      return true;
    return peek().isIdentifier();
  }
  else
  {
    if (!detect(LookAheadDetection))
      return false;

    size_t n = std::distance(iterator(), end());

    // 1. Check that there is not two consecutive identifier, 
    //    as in 'int v'.
    //    Note that 'const T' is valid so we need to take that into account.
    // 2. Check that after an identifier, there is either a '<' or a '::'
    //    or a reference sign.
    // 3. Check that '&' or '&&' are at the end.
    // 4. Check the proper nesting of '<' and '>'.

    bool prev_was_identifier = false;
    DelimitersCounter counter;
    int template_delimiters = 0;

    for (size_t i(0); i < n; ++i)
    {
      Token t = peek(i);

      if (t == Token::Const)
      {
        prev_was_identifier = false;
      }
      else
      {
        if (prev_was_identifier && t != Token::LeftAngle && t != Token::ScopeResolution && t != Token::Ampersand && t != Token::LogicalAnd)
          return false;

        if (t == Token::Ampersand || t == Token::LogicalAnd)
        {
          if (i != n - 1)
            return false;
        }
        
        if (counter.balanced() && (t == Token::LeftAngle || t == Token::RightAngle || t == Token::RightShift))
        {
          if (t == Token::LeftAngle)
            template_delimiters += 1;
          else if (t == Token::RightAngle)
            template_delimiters -= 1;
          else if (t == Token::RightShift)
            template_delimiters -= 2;
        }

        if (t.isIdentifier())
        {
          if (prev_was_identifier)
            return false;

          prev_was_identifier = true;
        }
        else
        {
          prev_was_identifier = false;
        }
      }
    }

    return template_delimiters == 0;
  }
}

bool TypeParser::lookAheadFunctionSignature()
{
  if (unsafe_peek() != Token::LeftPar)
    return false;

  TokenReader params_reader = subfragment<Fragment::DelimiterPair>();

  while (!params_reader.atEnd())
  {
    TypeParser tp{ context(), params_reader.next<Fragment::ListElement>() };

    if (!tp.detect(FullFragmentDetection))
      return false;
   
    params_reader.seek(tp.end());

    if (!params_reader.atEnd())
      params_reader.read(Token::Comma);
  }

  return true;
}

ast::QualifiedType TypeParser::tryReadFunctionSignature(const ast::QualifiedType & rt)
{
  ast::QualifiedType ret;
  ret.functionType = std::make_shared<ast::FunctionType>();
  ret.functionType->returnType = rt;
  
  TokenReader params_reader = subfragment<Fragment::DelimiterPair>();

  unsafe_read(); // @TODO: is this read() required as we do a seek() after the loop ?
  while (!params_reader.atEnd())
  {
    TypeParser tp{ context(), params_reader.next<Fragment::ListElement>() };
    auto param = tp.parse();
    ret.functionType->params.push_back(param);

    if (!tp.atEnd())
      throw tp.SyntaxErr(ParserError::UnexpectedToken, errors::UnexpectedToken{ *tp.iterator(), Token::Invalid});

    if (!params_reader.atEnd())
      params_reader.read(Token::Comma);
  }

  seek(params_reader.end() + 1);

  if (atEnd())
    return ret;

  if (unsafe_peek() == Token::Const)
    ret.constQualifier = unsafe_read();

  if (atEnd())
    return ret;

  if (unsafe_peek() == Token::Ref)
    ret.reference = unsafe_read();

  return ret;
}



FunctionParamParser::FunctionParamParser(std::shared_ptr<ParserContext> shared_context, const TokenReader& reader)
  : ParserBase(shared_context, reader)
{

}

ast::FunctionParameter FunctionParamParser::parse()
{
  ast::FunctionParameter fp;

  TypeParser tp{ context(), subfragment() };
  fp.type = parse_and_seek(tp);

  if (atEnd())
    return fp;

  IdentifierParser ip{ context(), subfragment(), IdentifierParser::ParseOnlySimpleId };
  /// TODO: add overload that returns SimpleIdentifier
  auto name = std::static_pointer_cast<ast::SimpleIdentifier>(parse_and_seek(ip));
  fp.name = name->name;

  if (atEnd())
    return fp;

  read(Token::Eq);
  ExpressionParser ep{ context(), subfragment() };
  auto defaultVal = parse_and_seek(ep);
  fp.defaultValue = defaultVal;

  return fp;
}

ExpressionListParser::ExpressionListParser(std::shared_ptr<ParserContext> shared_context, const TokenReader& reader)
  : ParserBase(shared_context, reader)
{

}

ExpressionListParser::~ExpressionListParser()
{

}

std::vector<std::shared_ptr<ast::Expression>> ExpressionListParser::parse()
{
  std::vector<std::shared_ptr<ast::Expression>> result;
  while (!atEnd())
  {
    ExpressionParser expr{ context(), subfragment<Fragment::ListElement>() };
    auto e = parse_and_seek(expr);
    result.push_back(e);
    if (!atEnd())
      read(); // reads the comma
  }

  return result;
}


DeclParser::DeclParser(std::shared_ptr<ParserContext> shared_context, const TokenReader& reader, std::shared_ptr<ast::Identifier> cn)
  : ParserBase(shared_context, reader)
  , mDecision(Undecided)
  , mClassName(cn)
  , mParamsAlreadyRead(false)
  , mDeclaratorOptions(IdentifierParser::ParseSimpleId | IdentifierParser::ParseOperatorName)
{

}

DeclParser::~DeclParser()
{

}

void DeclParser::readOptionalAttribute()
{
  AttributeParser parser{ context(), subfragment() };
 
  if (parser.ready())
    mAttribute = parse_and_seek(parser);
}

void DeclParser::readOptionalDeclSpecifiers()
{
  if (readOptionalVirtual())
  {
    if (!isParsingMember())
      throw SyntaxErr(ParserError::IllegalUseOfKeyword, errors::KeywordToken{mVirtualKw});
  }

  readOptionalStatic();

  if (readOptionalExplicit())
  {
    if (!isParsingMember())
      throw SyntaxErr(ParserError::IllegalUseOfKeyword, errors::KeywordToken{mExplicitKw});
  }
}

bool DeclParser::detectBeforeReadingTypeSpecifier()
{
  if (!isParsingMember())
    return false;
  return detectDtorDecl() || detectCastDecl() || detectCtorDecl();
}

bool DeclParser::readTypeSpecifier()
{
  TypeParser tp{ context(), subfragment() };
  try
  {
    mType = parse_and_seek(tp);
  }
  catch (const SyntaxError& )
  {
    if (mDecision != Undecided)
      throw;
    mDecision = NotADecl;
    return false;
  }

  return true;
}

bool DeclParser::detectBeforeReadingDeclarator()
{
  if (!isParsingMember())
    return false;

  if (mType.functionType != nullptr && (peek() == Token::Colon || peek() == Token::LeftBrace || peek() == Token::Eq))
  {
    // this could be a ctor decl which was misinterpreted
    // eg. A(int, int) : a(0) { } // where A(int, int) is interpreted as a type
    if (mType.functionType->returnType.isSimple() && isClassName(mType.functionType->returnType.type))
    {
      mDecision = ParsingConstructor;
      auto ctorDecl = ast::ConstructorDecl::New(mType.functionType->returnType.type);
      mFuncDecl = ctorDecl;
      mFuncDecl->attribute = mAttribute;
      for (const auto & p : mType.functionType->params)
      {
        ast::FunctionParameter param;
        param.type = p;
        mFuncDecl->params.push_back(param);
      }
      mParamsAlreadyRead = true;
      mType = ast::QualifiedType{};
      return true;
    }
  }
  else if (peek() == Token::LeftPar)
  {
    if (mType.functionType == nullptr && mType.reference == Token::Invalid && mType.constQualifier == Token::Invalid && isClassName(mType.type))
    {
      mDecision = ParsingConstructor;
      auto ctorDecl = ast::ConstructorDecl::New(mType.type);
      mFuncDecl = ctorDecl;
      mFuncDecl->attribute = mAttribute;
      mType = ast::QualifiedType{};
      return true;
    }
  }

  return false;
}

bool DeclParser::readDeclarator()
{
  IdentifierParser ip{ context(), subfragment() };
  ip.setOptions(mDeclaratorOptions);

  if (mDecision != Undecided)
  {
    mName = parse_and_seek(ip);
  }
  else
  {
    if (!ip.lookAhead())
    {
      mDecision = NotADecl;
      return false;
    }

    try
    {
      mName = parse_and_seek(ip);
    }
    catch (const SyntaxError&)
    {
      mDecision = NotADecl;
      return false;
    }
  }

  return true;
}


bool DeclParser::detectFromDeclarator()
{
  if (mName->is<ast::OperatorName>())
  {
    mDecision = ParsingFunction;
    auto overload = ast::OperatorOverloadDecl::New(mName);
    overload->returnType = mType;
    mFuncDecl = overload;
    mFuncDecl->attribute = mAttribute;
    return true;
  }
  else if (mName->is<ast::LiteralOperatorName>())
  {
    mDecision = ParsingFunction;
    auto lon = ast::OperatorOverloadDecl::New(mName);
    lon->returnType = mType;
    mFuncDecl = lon;
    mFuncDecl->attribute = mAttribute;
    return true;
  }
  else if (mVirtualKw.isValid())
  {
    // We could detect that a little bit earlier but this avoids 
    // some code duplication
    mDecision = ParsingFunction;
    mFuncDecl = ast::FunctionDecl::New(mName);
    mFuncDecl->attribute = mAttribute;
    mFuncDecl->returnType = mType;
    mFuncDecl->virtualKeyword = mVirtualKw;
    return true;
  }

  return false;
}

bool DeclParser::detectDecl()
{
  readOptionalAttribute();

  readOptionalDeclSpecifiers();

  if (detectBeforeReadingTypeSpecifier())
    return true;
  
  if (!readTypeSpecifier())
    return false;

  if (detectBeforeReadingDeclarator())
    return true;

  if (!readDeclarator())
    return false;
  
  detectFromDeclarator();

  if (peek() == Token::Semicolon)
    mDecision = ParsingVariable;

  return true;
}

std::shared_ptr<ast::Declaration> DeclParser::parse()
{
  assert(mDecision != NotADecl);

  if (mDecision == ParsingDestructor)
    return parseDestructor();
  else if (mDecision == ParsingConstructor)
    return parseConstructor();
  else if (mDecision == ParsingCastDecl || mDecision == ParsingFunction)
    return parseFunctionDecl();
  else if (mDecision == ParsingVariable)
  {
    if (mVarDecl == nullptr) /// TODO : is this always true ?
      mVarDecl = ast::VariableDecl::New(mType, std::static_pointer_cast<ast::SimpleIdentifier>(mName));
    mVarDecl->staticSpecifier = mStaticKw;
    return parseVarDecl();
  }

  assert(mDecision == Undecided);

  if (peek() == Token::LeftBrace || peek() == Token::Eq)
  {
    mDecision = ParsingVariable;

    mVarDecl = ast::VariableDecl::New(mType, std::static_pointer_cast<ast::SimpleIdentifier>(mName));
    mVarDecl->staticSpecifier = mStaticKw;
    return parseVarDecl();
  }
  else if (peek() == Token::LeftPar)
  {
    mFuncDecl = ast::FunctionDecl::New(mName);
    mFuncDecl->attribute = mAttribute;
    mFuncDecl->returnType = mType;
    mFuncDecl->staticKeyword = mStaticKw;
    mFuncDecl->virtualKeyword = mVirtualKw;

    assert(mName->is<ast::SimpleIdentifier>());
    mVarDecl = ast::VariableDecl::New(mType, std::static_pointer_cast<ast::SimpleIdentifier>(mName));
    mVarDecl->staticSpecifier = mStaticKw;
  }
  else
  {
    throw SyntaxErr(ParserError::UnexpectedToken, errors::UnexpectedToken{unsafe_peek(), Token::Invalid});
  }

  readArgsOrParams();

  readOptionalConst();

  if (readOptionalDeleteSpecifier() || readOptionalDefaultSpecifier()) {
    return mFuncDecl;
  } else if (isParsingMember() && readOptionalVirtualPureSpecifier()) {
    return mFuncDecl;
  }

  if (peek() == Token::LeftBrace)
  {
    if (mDecision == ParsingVariable)
      throw SyntaxErr(ParserError::UnexpectedToken, errors::UnexpectedToken{unsafe_peek(), Token::Invalid});

    mDecision = ParsingFunction;
    mVarDecl = nullptr;
    mFuncDecl->body = readFunctionBody();
    return mFuncDecl;
  }
  else if (peek() == Token::Semicolon)
  {
    if (mDecision == ParsingFunction)
      throw SyntaxErr(ParserError::UnexpectedToken, errors::UnexpectedToken{unsafe_peek(), Token::LeftBrace});

    mVarDecl->semicolon = read();
    return mVarDecl;
  }

  throw SyntaxErr(ParserError::UnexpectedToken, errors::UnexpectedToken{unsafe_peek(), Token::Invalid});
}

std::shared_ptr<ast::VariableDecl> DeclParser::parseVarDecl()
{
  if (peek() == Token::Eq)
  {
    const Token eqsign = read();
    ExpressionParser ep{ context(), subfragment<Fragment::Statement>() };
    auto expr = parse_and_seek(ep);
    mVarDecl->init = ast::AssignmentInitialization::New(eqsign, expr);
  }
  else if (peek() == Token::LeftBrace)
  {
    const Token leftBrace = unsafe_peek();
    ExpressionListParser argsParser{ context(), subfragment<Fragment::DelimiterPair>() };
    auto args = parse_and_seek(argsParser);
    const Token rightbrace = read(Token::RightBrace);
    mVarDecl->init = ast::BraceInitialization::New(leftBrace, std::move(args), rightbrace);
  }
  else if (peek() == Token::LeftPar)
  {
    const Token leftpar = unsafe_peek();
    ExpressionListParser argsParser{ context(), subfragment<Fragment::DelimiterPair>() };
    auto args = parse_and_seek(argsParser);
    const Token rightpar = read(Token::RightPar);
    mVarDecl->init = ast::ConstructorInitialization::New(leftpar, std::move(args), rightpar);
  }
  else
  {
    /// TODO: should we assert here ?
    assert(peek() == Token::Semicolon);
  }

  mVarDecl->semicolon = read(Token::Semicolon);

  return mVarDecl;
}

std::shared_ptr<ast::FunctionDecl> DeclParser::parseFunctionDecl()
{
  assert(isParsingFunction());
  
  readParams();

  readOptionalConst();

  if (readOptionalDeleteSpecifier() || readOptionalDefaultSpecifier()) {
    return mFuncDecl;
  }
  else if (isParsingMember() && readOptionalVirtualPureSpecifier()) {
    return mFuncDecl;
  }
 
  mFuncDecl->body = readFunctionBody();

  return mFuncDecl;
}

std::shared_ptr<ast::FunctionDecl> DeclParser::parseConstructor()
{
  if(!mParamsAlreadyRead)
    readParams();

  readOptionalMemberInitializers();

  if (readOptionalDeleteSpecifier() || readOptionalDefaultSpecifier()) {
    return mFuncDecl;
  }

  mFuncDecl->body = readFunctionBody();

  return mFuncDecl;
}

void DeclParser::readOptionalMemberInitializers()
{
  if (peek() != Token::Colon)
    return;

  auto ctor = std::dynamic_pointer_cast<ast::ConstructorDecl>(mFuncDecl);
  unsafe_read();

  for (;;)
  {
    IdentifierParser idp{ context(), subfragment(), IdentifierParser::ParseOnlySimpleId | IdentifierParser::ParseTemplateId };
    auto id = parse_and_seek(idp);
    if (peek() == Token::LeftBrace)
    {
      const Token leftBrace = unsafe_peek();
      ExpressionListParser argsParser{ context(), subfragment<Fragment::DelimiterPair>() };
      auto args = parse_and_seek(argsParser);
      const Token rightBrace = read(Token::RightBrace);
      auto braceinit = ast::BraceInitialization::New(leftBrace, std::move(args), rightBrace);
      ctor->memberInitializationList.push_back(ast::MemberInitialization{ id, braceinit });
    }
    else if (peek() == Token::LeftPar)
    {
      const Token leftpar = unsafe_peek();
      ExpressionListParser argsParser{ context(), subfragment<Fragment::DelimiterPair>() };
      auto args = parse_and_seek(argsParser);
      const Token rightpar = read(Token::RightPar);
      auto ctorinit = ast::ConstructorInitialization::New(leftpar, std::move(args), rightpar);
      ctor->memberInitializationList.push_back(ast::MemberInitialization{ id, ctorinit });
    }

    if (peek() == Token::LeftBrace)
      break;

    read(Token::Comma);
  }
}

std::shared_ptr<ast::FunctionDecl> DeclParser::parseDestructor()
{
  read(Token::LeftPar);
  read(Token::RightPar);

  if (readOptionalDeleteSpecifier() || readOptionalDefaultSpecifier()) {
    return mFuncDecl;
  }

  mFuncDecl->body = readFunctionBody();
  
  return mFuncDecl;
}

DeclParser::Decision DeclParser::decision() const
{
  return mDecision;
}

void DeclParser::setDecision(Decision d)
{
  assert(mDecision == Undecided);

  mDecision = d;
  if (mDecision == ParsingVariable)
  {
    mFuncDecl = nullptr;
  }
  else if (isParsingFunction())
  {
    mVarDecl = nullptr;

    if (mFuncDecl == nullptr)
    {
      mFuncDecl = ast::FunctionDecl::New(mName);
      mFuncDecl->returnType = mType;
      mFuncDecl->staticKeyword = mStaticKw;
      mFuncDecl->virtualKeyword = mVirtualKw;
    }
  }
}

bool DeclParser::isParsingFunction() const
{
  return static_cast<int>(mDecision) >= static_cast<int>(ParsingFunction);
}

bool DeclParser::isParsingMember() const
{
  return mClassName != nullptr;
}


bool DeclParser::readOptionalVirtual()
{
  if (peek() != Token::Virtual)
    return false;

  mVirtualKw = read();

  return true;
}

bool DeclParser::readOptionalStatic()
{
  if (peek() != Token::Static)
    return false;

  mStaticKw = read();

  return true;
}

bool DeclParser::readOptionalExplicit()
{
  if (peek() != Token::Explicit)
    return false;

  mExplicitKw = read();

  return true;
}

void DeclParser::readParams()
{
  TokenReader parameters = next<Fragment::DelimiterPair>();

  while (!parameters.atEnd())
  {
    FunctionParamParser pp{ context(), parameters.next<Fragment::ListElement>() };
    auto param = pp.parse();
    mFuncDecl->params.push_back(param);

    if (!parameters.atEnd())
      parameters.read(Token::Comma);
  }

  read(Token::RightPar);
}


void DeclParser::readArgsOrParams()
{
  const Token leftPar = unsafe_peek();
  assert(leftPar == Token::LeftPar);

  TokenReader args_or_params = next<Fragment::DelimiterPair>();

  if (mDecision == Undecided || mDecision == ParsingVariable)
    mVarDecl->init = ast::ConstructorInitialization::New(leftPar, {}, parser::Token{});

  while (!args_or_params.atEnd())
  {
    TokenReader listelem_reader = args_or_params.next<Fragment::ListElement>();

    if (mDecision == Undecided || mDecision == ParsingVariable)
    {
      try
      {
        ExpressionParser ep{ context(), listelem_reader };
        auto expr = ep.parse();
        mVarDecl->init->as<ast::ConstructorInitialization>().args.push_back(expr);
      }
      catch (const SyntaxError&)
      {
        if (mDecision == ParsingVariable)
          throw;
        else
          mDecision = ParsingFunction;

        mVarDecl = nullptr;
      }
    }

    if (mDecision == Undecided || isParsingFunction())
    {
      try
      {
        FunctionParamParser pp{ context(), listelem_reader };
        auto param = pp.parse();
        mFuncDecl->params.push_back(param);
      }
      catch (const SyntaxError&)
      {
        if (isParsingFunction())
          throw;
        else
          mDecision = ParsingVariable;

        mFuncDecl = nullptr;
      }
    }

    if (!args_or_params.atEnd())
      args_or_params.read(Token::Comma);
  }

  const Token rightpar = read(Token::RightPar);

  if (mVarDecl != nullptr)
    mVarDecl->init->as<ast::ConstructorInitialization>().right_par = rightpar;
}

bool DeclParser::readOptionalConst()
{
  if (peek() != Token::Const)
    return false;
  
  if (mDecision == ParsingVariable)
    throw SyntaxErr(ParserError::UnexpectedToken, errors::UnexpectedToken{unsafe_peek(), Token::Invalid});

  mDecision = ParsingFunction;
  mVarDecl = nullptr;
  mFuncDecl->constQualifier = read();

  return true;
}

bool DeclParser::readOptionalSpecifier(Token::Id id, Token& tok)
{
  if (mDecision == ParsingVariable)
    return false;

  if (peek() != Token::Eq)
    return false;

  auto p = iterator();
  Token eq_token = unsafe_read();

  if (atEnd())
    throw SyntaxErr(ParserError::UnexpectedEndOfInput);

  if (peek() != id)
  {
    seek(p);
    return false;
  }

  const Token spec = read();
  mFuncDecl->equalSign = eq_token;
  tok = spec;

  mDecision = ParsingFunction;
  mVarDecl = nullptr;

  if (atEnd())
    throw SyntaxErr(ParserError::UnexpectedEndOfInput);

  read(Token::Semicolon);

  return true;
}

bool DeclParser::readOptionalDeleteSpecifier()
{
  return mFuncDecl && readOptionalSpecifier(Token::Delete, mFuncDecl->deleteKeyword);
}

bool DeclParser::readOptionalDefaultSpecifier()
{
  return mFuncDecl && readOptionalSpecifier(Token::Default, mFuncDecl->defaultKeyword);
}

bool DeclParser::readOptionalVirtualPureSpecifier()
{
  bool success = mFuncDecl && readOptionalSpecifier(Token::OctalLiteral, mFuncDecl->virtualPure);

  if(success)
  {
    if (!mFuncDecl->virtualPure.isZero())
      throw SyntaxErr(ParserError::UnexpectedToken, errors::UnexpectedToken{ mFuncDecl->virtualPure, Token::OctalLiteral });
  }

  return success;
}


std::shared_ptr<ast::CompoundStatement> DeclParser::readFunctionBody()
{
  if (peek() != Token::LeftBrace)
    throw SyntaxErr(ParserError::UnexpectedToken, errors::UnexpectedToken{unsafe_peek(), Token::LeftBrace});

  ProgramParser pParser{ context(), subfragment() };
  auto ret = std::dynamic_pointer_cast<ast::CompoundStatement>(pParser.parseStatement());
  seek(pParser.iterator());
  return ret;
}

bool DeclParser::detectCtorDecl()
{
  if (!mExplicitKw.isValid())
    return false;

  auto p = iterator();
  IdentifierParser ip{ context(), subfragment() };
  std::shared_ptr<ast::Identifier> iden;
  try
  {
    iden = parse_and_seek(ip);
    if (!isClassName(iden))
    {
      seek(p);
      return false;
    }
  }
  catch (const SyntaxError&)
  {
    seek(p);
    return false;
  }

  if (peek() != Token::LeftPar) 
  {
    seek(p);
    return false;
  }

  mDecision = ParsingConstructor;
  auto ctorDecl = ast::ConstructorDecl::New(iden);
  mFuncDecl = ctorDecl;
  mFuncDecl->explicitKeyword = mExplicitKw;

  return true;
}

bool DeclParser::detectDtorDecl()
{
  if (peek() != Token::Tilde)
    return false;

  const Token tilde = unsafe_read();
  if (atEnd())
    throw SyntaxErr(ParserError::UnexpectedEndOfInput);

  IdentifierParser ip{ context(), subfragment(), IdentifierParser::ParseSimpleId | IdentifierParser::ParseTemplateId };
  auto iden = parse_and_seek(ip);

  if (!isClassName(iden))
    throw SyntaxErr(ParserError::ExpectedCurrentClassName);

  mDecision = ParsingDestructor;
  auto dtor = ast::DestructorDecl::New(iden);
  dtor->tilde = tilde;
  dtor->virtualKeyword = mVirtualKw;
  mFuncDecl = dtor;
  return true;
}

bool DeclParser::detectCastDecl()
{
  if (peek() != Token::Operator)
    return false;

  const auto p = iterator();
  
  const Token opKw = read();
  TypeParser tp{ context(), subfragment() };
  tp.setReadFunctionSignature(false); // people should use a typedef in this situtation
  ast::QualifiedType type;
  try
  {
    type = parse_and_seek(tp);
  }
  catch (const SyntaxError&)
  {
    if (mExplicitKw.isValid())
    {
      throw SyntaxErr(ParserError::CouldNotReadType);
    }
    else 
    {
      seek(p);
      return false;
    }
  }

  mDecision = ParsingCastDecl;
  auto castDecl = ast::CastDecl::New(type);
  castDecl->operatorKw = opKw;
  mFuncDecl = castDecl;
  mFuncDecl->explicitKeyword = mExplicitKw;

  return true;
}

bool DeclParser::isClassName(const std::shared_ptr<ast::Identifier> & name) const
{
  if (!name->is<ast::SimpleIdentifier>())
    return false;

  if (mClassName->is<ast::SimpleIdentifier>())
    return mClassName->as<ast::SimpleIdentifier>().getName() == name->as<ast::SimpleIdentifier>().getName();
  else if(mClassName->is<ast::TemplateIdentifier>())
    return mClassName->as<ast::TemplateIdentifier>().getName() == name->as<ast::SimpleIdentifier>().getName();
 
  return false;
}


AttributeParser::AttributeParser(std::shared_ptr<ParserContext> shared_context, const TokenReader& reader)
  : ParserBase(shared_context, reader)
{

}

bool AttributeParser::ready() const
{
  return peek() == Token::LeftBracket && peek(1) == Token::LeftBracket;
}

std::shared_ptr<ast::AttributeDeclaration> AttributeParser::parse()
{
  TokenReader subreader = subfragment<Fragment::DelimiterPair>();
  subreader = subreader.subfragment<Fragment::DelimiterPair>();

  Token lb1 = read(Token::LeftBracket);
  Token lb2 = read(Token::LeftBracket);
  Token dblb{ Token::DblLeftBracket, 0, StringView(lb1.text().data(), 2) };

  ExpressionParser subparser{ context(), subreader };
  std::shared_ptr<ast::Node> attr = parse_and_seek(subparser);

  Token rb1 = read(Token::RightBracket);
  Token rb2 = read(Token::RightBracket);
  Token dbrb{ Token::DblRightBracket, 0, StringView(rb1.text().data(), 2) };

  return ast::AttributeDeclaration::New(dblb, attr, dbrb);
}


EnumValueParser::EnumValueParser(std::shared_ptr<ParserContext> shared_context, const TokenReader& reader)
  : ParserBase(shared_context, reader)
{

}

void EnumValueParser::parse()
{
  while (!atEnd())
  {
    TokenReader value_reader = next<Fragment::ListElement>();

    IdentifierParser idparser{ context(), value_reader, IdentifierParser::ParseOnlySimpleId };
    /// TODO: add overlaod that returns ast::SimpleIdentifier
    auto name = idparser.parse();

    value_reader.seek(idparser.iterator());

    if (value_reader.atEnd())
    {
      values.push_back(ast::EnumValueDeclaration{ std::static_pointer_cast<ast::SimpleIdentifier>(name), nullptr });
    }
    else
    {
      value_reader.read(Token::Eq);

      ExpressionParser valparser{ context(), value_reader };
      auto expr = valparser.parse();

      values.push_back(ast::EnumValueDeclaration{ std::static_pointer_cast<ast::SimpleIdentifier>(name), expr });
    }

    if (!atEnd())
      read(Token::Comma);
  }
}


EnumParser::EnumParser(std::shared_ptr<ParserContext> shared_context, const TokenReader& reader)
  : ParserBase(shared_context, reader)
{

}

std::shared_ptr<ast::EnumDeclaration> EnumParser::parse()
{
  const Token & etok = read();

  Token ctok;
  if (peek() == Token::Class)
    ctok = read();

  std::shared_ptr<ast::AttributeDeclaration> attr = nullptr;
  {
    AttributeParser attrparser{ context(), subfragment() };

    if (attrparser.ready())
      attr = parse_and_seek(attrparser);
  }

  std::shared_ptr<ast::SimpleIdentifier> enum_name;
  {
    IdentifierParser idparser{ context(), subfragment(), IdentifierParser::ParseOnlySimpleId };
    /// TODO: add overload to avoid this cast
    enum_name = std::static_pointer_cast<ast::SimpleIdentifier>(parse_and_seek(idparser));
  }

  parser::Token lb = read(Token::LeftBrace);
  seek(iterator() - 1);

  EnumValueParser value_parser{ context(), subfragment<Fragment::DelimiterPair>() };
  value_parser.parse();
  seek(value_parser.iterator());

  parser::Token rb = read(Token::RightBrace);
  read(Token::Semicolon);

  auto result = ast::EnumDeclaration::New(etok, ctok, lb, enum_name, std::move(value_parser.values), rb);
  result->attribute = attr;
  return result;
}



ClassParser::ClassParser(std::shared_ptr<ParserContext> shared_context, const TokenReader& reader)
  : ParserBase(shared_context, reader)
  , mTemplateSpecialization(false)
{

}

ClassParser::~ClassParser()
{

}

void ClassParser::setTemplateSpecialization(bool on)
{
  mTemplateSpecialization = on;
}

std::shared_ptr<ast::ClassDecl> ClassParser::parse()
{
  const Token classKeyword = read();

  std::shared_ptr<ast::AttributeDeclaration> attr = readOptionalAttribute();

  auto name = readClassName();

  mClass = ast::ClassDecl::New(classKeyword, name);

  mClass->attribute = attr;

  readOptionalParent();

  mClass->openingBrace = read(Token::LeftBrace);

  while (!readClassEnd())
    readNode();

  return mClass;
}

void ClassParser::parseAccessSpecifier()
{
  const Token visibility = unsafe_read();

  const Token colon = read(Token::Colon);

  mClass->content.push_back(ast::AccessSpecifier::New(visibility, colon));
}

void ClassParser::parseFriend()
{
  FriendParser fdp{ context(), subfragment() };
  auto friend_decl = parse_and_seek(fdp);
  mClass->content.push_back(friend_decl);
}

void ClassParser::parseTemplate()
{
  TemplateParser tp{ context(), subfragment() };
  auto template_decl = parse_and_seek(tp);
  mClass->content.push_back(template_decl);
}

void ClassParser::parseUsing()
{
  UsingParser np{ context(), subfragment() };
  auto decl = parse_and_seek(np);
  mClass->content.push_back(decl);
}

void ClassParser::parseTypedef()
{
  TypedefParser tp{ context(), subfragment() };
  auto decl = parse_and_seek(tp);
  mClass->content.push_back(decl);
}

std::shared_ptr<ast::AttributeDeclaration> ClassParser::readOptionalAttribute()
{
  AttributeParser parser{ context(), subfragment() };
  return parser.ready() ? parse_and_seek(parser) : nullptr;
}

std::shared_ptr<ast::Identifier> ClassParser::readClassName()
{
  auto opts = mTemplateSpecialization ? IdentifierParser::ParseTemplateId : 0;
  IdentifierParser nameParser{ context(), subfragment(), opts | IdentifierParser::ParseSimpleId };
  return parse_and_seek(nameParser);
}

void ClassParser::readOptionalParent()
{
  if (atEnd())
    throw SyntaxErr(ParserError::UnexpectedEndOfInput);

  if (peek() != Token::Colon)
    return;

  mClass->colon = unsafe_read();

  if (atEnd())
    throw SyntaxErr(ParserError::UnexpectedEndOfInput);

  IdentifierParser nameParser{ context(), subfragment(), IdentifierParser::ParseTemplateId | IdentifierParser::ParseQualifiedId}; // TODO : forbid read operator name directly here
  auto parent = parse_and_seek(nameParser);
  //if(parent->is<ast::OperatorName>())
  //  throw Error{ "Unexpected operator name read after ':' " };

  mClass->parent = parent;
}

void ClassParser::readDecl()
{
  if (atEnd())
    throw SyntaxErr(ParserError::UnexpectedEndOfInput);

  DeclParser dp{ context(), subfragment(), mClass->name };
  
  if (!dp.detectDecl())
    throw SyntaxErr(ParserError::ExpectedDeclaration);

  mClass->content.push_back(parse_and_seek(dp));
}

void ClassParser::readNode()
{
  switch (peek().id)
  {
  case Token::Public:
  case Token::Protected:
  case Token::Private:
    parseAccessSpecifier();
    return;
  case Token::Friend:
    parseFriend();
    return;
  case Token::Using:
    parseUsing();
    return;
  case Token::Template:
    parseTemplate();
    return;
  case Token::Typedef:
    parseTypedef();
    return;
  default:
    break;
  }

  readDecl();
}

bool ClassParser::readClassEnd()
{
  if (peek() != Token::RightBrace)
    return false;

  mClass->closingBrace = unsafe_read();
  mClass->endingSemicolon = read(Token::Semicolon);

  return true;
}

NamespaceParser::NamespaceParser(std::shared_ptr<ParserContext> shared_context, const TokenReader& reader)
  : ParserBase(shared_context, reader)
{

}

std::shared_ptr<ast::Declaration> NamespaceParser::parse()
{
  const Token ns_tok = unsafe_read();

  auto name = readNamespaceName();

  if (peek() == Token::Eq)
  {
    const Token eq_sign = unsafe_read();

    IdentifierParser idp{ context(), subfragment() };
    std::shared_ptr<ast::Identifier> aliased_name = parse_and_seek(idp);

    read(Token::Semicolon);

    return ast::NamespaceAliasDefinition::New(ns_tok, name, eq_sign, aliased_name);
  }

  const Token lb = read(Token::LeftBrace);
  seek(iterator() - 1);

  Parser parser;
  parser.reset(context(), next<Fragment::DelimiterPair>());
  auto statements = parser.parseProgram();

  const Token rb = read(Token::RightBrace);

  return ast::NamespaceDeclaration::New(ns_tok, name, lb, std::move(statements), rb);
}

std::shared_ptr<ast::SimpleIdentifier> NamespaceParser::readNamespaceName()
{
  IdentifierParser idp{ context(), subfragment(), IdentifierParser::ParseOnlySimpleId };
  return std::static_pointer_cast<ast::SimpleIdentifier>(parse_and_seek(idp));
}



FriendParser::FriendParser(std::shared_ptr<ParserContext> shared_context, const TokenReader& reader)
  : ParserBase(shared_context, reader)
{

}

std::shared_ptr<ast::FriendDeclaration> FriendParser::parse()
{
  const Token friend_tok = unsafe_read();

  const Token class_tok = read(Token::Class);

  std::shared_ptr<ast::Identifier> class_name;
  {
    IdentifierParser idp{ context(), subfragment() };
    class_name = parse_and_seek(idp);
  }

  const Token semicolon = read(Token::Semicolon);

  return ast::ClassFriendDeclaration::New(friend_tok, class_tok, class_name);
}



UsingParser::UsingParser(std::shared_ptr<ParserContext> shared_context, const TokenReader& reader)
  : ParserBase(shared_context, reader)
{

}

std::shared_ptr<ast::Declaration> UsingParser::parse()
{
  assert(peek() == Token::Using);

  const Token using_tok = unsafe_read();

  if (peek() == Token::Namespace)
  {
    const Token namespace_tok = unsafe_read();
    std::shared_ptr<ast::Identifier> name = read_name();
    read(Token::Semicolon);
    return ast::UsingDirective::New(using_tok, namespace_tok, name);
  }

  std::shared_ptr<ast::Identifier> name = read_name();

  if (name->is<ast::ScopedIdentifier>())
  {
    read(Token::Semicolon);
    return ast::UsingDeclaration::New(using_tok, std::static_pointer_cast<ast::ScopedIdentifier>(name));
  }

  /// TODO: throw exception instead
  assert(name->is<ast::SimpleIdentifier>());

  const Token eq_sign = read(Token::Eq);

  std::shared_ptr<ast::Identifier> aliased_type = read_name();
  read(Token::Semicolon);
  return ast::TypeAliasDeclaration::New(using_tok, std::static_pointer_cast<ast::SimpleIdentifier>(name), eq_sign, aliased_type);
}

std::shared_ptr<ast::Identifier> UsingParser::read_name()
{
  IdentifierParser idp{ context(), subfragment() };
  return parse_and_seek(idp);
}



TypedefParser::TypedefParser(std::shared_ptr<ParserContext> shared_context, const TokenReader& reader)
  : ParserBase(shared_context, reader)
{

}

std::shared_ptr<ast::Typedef> TypedefParser::parse()
{
  assert(peek() == Token::Typedef);

  const parser::Token typedef_tok = unsafe_read();

  TypeParser tp{ context(), subfragment() };
  const ast::QualifiedType qtype = parse_and_seek(tp);

  IdentifierParser idp{ context(), subfragment(), IdentifierParser::ParseOnlySimpleId };
  /// TODO: add overload to IdentifierParser that only parses symple identifier.
  const auto name = std::static_pointer_cast<ast::SimpleIdentifier>(parse_and_seek(idp));

  read(parser::Token::Semicolon);

  return ast::Typedef::New(typedef_tok, qtype, name);
}



ImportParser::ImportParser(std::shared_ptr<ParserContext> shared_context, const TokenReader& reader)
  : ParserBase(shared_context, reader) { }

std::shared_ptr<ast::ImportDirective> ImportParser::parse()
{
  const Token exprt = unsafe_peek() == Token::Export ? unsafe_read() : Token{};

  const Token imprt = read(Token::Import);

  std::vector<Token> names;

  Token tok = read();

  if (!tok.isIdentifier())
    throw SyntaxErr(ParserError::ExpectedIdentifier, errors::ActualToken{tok});

  names.push_back(tok);

  while (peek() == Token::Dot)
  {
    unsafe_read();

    tok = read();

    if (!tok.isIdentifier())
      throw SyntaxErr(ParserError::ExpectedIdentifier, errors::ActualToken{tok});

    names.push_back(tok);
  }

  read(Token::Semicolon);

  return ast::ImportDirective::New(exprt, imprt, std::move(names));
}



TemplateParser::TemplateParser(std::shared_ptr<ParserContext> shared_context, const TokenReader& reader)
  : ParserBase(shared_context, reader)
{

}

std::shared_ptr<ast::TemplateDeclaration> TemplateParser::parse()
{
  const Token tmplt_k = unsafe_read();

  TokenReader tparams_reader = subfragment<Fragment::Template>();

  if (!tparams_reader.valid())
    throw SyntaxErr(ParserError::UnexpectedFragmentEnd);

  const Token left_angle = read(Token::LeftAngle);

  std::vector<ast::TemplateParameter> params;

  while(!tparams_reader.atEnd())
  {
    TemplateParameterParser param_parser{ context(), tparams_reader.next<Fragment::ListElement>() };
    params.push_back(param_parser.parse());

    if (!tparams_reader.atEnd())
      tparams_reader.read(Token::Comma);
  }

  seek(tparams_reader.end());

  //const Token right_angle = sentinel.consumeSentinel();
  const Token right_angle = read();
  assert(right_angle == Token::RightAngle || right_angle == Token::RightRightAngle);

  const auto decl = parse_decl();

  return ast::TemplateDeclaration::New(tmplt_k, left_angle, std::move(params), right_angle, decl);
}

std::shared_ptr<ast::Declaration> TemplateParser::parse_decl()
{
  if (peek() == Token::Class || peek() == Token::Struct)
  {
    ClassParser parser{ context(), subfragment() };
    parser.setTemplateSpecialization(true);
    return parse_and_seek(parser);
  }

  DeclParser funcparser{ context(), subfragment() };
  funcparser.setDeclaratorOptions(IdentifierParser::ParseSimpleId | IdentifierParser::ParseOperatorName | IdentifierParser::ParseTemplateId);

  if (!funcparser.detectDecl())
    throw SyntaxErr(ParserError::ExpectedDeclaration);

  funcparser.setDecision(DeclParser::ParsingFunction);
  return parse_and_seek(funcparser);
}



TemplateParameterParser::TemplateParameterParser(std::shared_ptr<ParserContext> shared_context, const TokenReader& reader)
  : ParserBase(shared_context, reader)
{

}

ast::TemplateParameter TemplateParameterParser::parse()
{
  ast::TemplateParameter result;

  if (peek() == Token::Typename)
    result.kind = unsafe_read();
  else if (unsafe_peek() == Token::Int)
    result.kind = unsafe_read();
  else if (unsafe_peek() == Token::Bool)
    result.kind = unsafe_read();
  else
    throw SyntaxErr(ParserError::UnexpectedToken, errors::UnexpectedToken{ unsafe_peek(), Token::Invalid });

  if (!peek().isIdentifier())
    throw SyntaxErr(ParserError::ExpectedIdentifier, errors::ActualToken{unsafe_peek()});

  result.name = unsafe_read();

  if (atEnd())
    return result;

  result.eq = read(Token::Eq);

  TemplateArgParser argp{ context(), subfragment() };
  result.default_value = parse_and_seek(argp);

  return result;
}

static SourceFile loaded_source_file(SourceFile src)
{
  if (!src.isLoaded())
    src.load();
  return src;
}

Parser::Parser()
  : ProgramParser(std::make_shared<ParserContext>(nullptr, std::vector<Token>()))
{

}

Parser::Parser(const std::string& str)
  : ProgramParser(std::make_shared<ParserContext>(str))
{

}

Parser::Parser(const char* str)
  : ProgramParser(std::make_shared<ParserContext>(str))
{

}

const std::vector<Token>& Parser::tokens() const
{
  return context()->tokens();
}

std::shared_ptr<ast::AST> parse(const SourceFile& source)
{
  Parser p{ loaded_source_file(source).content() };

  std::shared_ptr<ast::AST> ret = std::make_shared<ast::AST>(source);
  ret->root = ast::ScriptRootNode::New(ret);

  while (!p.atEnd())
  {
    ret->add(p.parseStatement());
  }

  return ret;
}

std::shared_ptr<ast::Expression> parseExpression(const std::string& source)
{
  auto c = std::make_shared<ParserContext>(source);
  ExpressionParser ep{ c, TokenReader(c->source(), c->tokens()) };
  return ep.parse();
}

std::shared_ptr<ast::Expression> parseExpression(const char* src)
{
  auto c = std::make_shared<ParserContext>(src);
  ExpressionParser ep{ c, TokenReader(src, c->tokens()) };
  return ep.parse();
}

std::shared_ptr<ast::Identifier> parseIdentifier(const std::string& src)
{
  auto c = std::make_shared<parser::ParserContext>(src);
  parser::TokenReader reader{ c->source(), c->tokens() };
  parser::IdentifierParser idp{ c, reader };
  auto result = idp.parse();

  if (!idp.atEnd())
    throw SyntaxError(ParserError::ExpectedIdentifier);

  return result;
}

} // parser

} // namespace script

