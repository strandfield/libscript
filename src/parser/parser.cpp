// Copyright (C) 2018-2020 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/parser/parser.h"

#include <map>
#include <stdexcept>

#include "script/operator.h"

#include "script/parser/parsererrors.h"

namespace script
{

namespace parser
{

void DelimitersCounter::reset()
{
  par_depth = 0;
  brace_depth = 0;
  bracket_depth = 0;
}

void DelimitersCounter::relaxed_feed(const Token& tok) noexcept
{
  switch (tok.id)
  {
  case Token::LeftPar:
    ++par_depth;
    break;
  case Token::RightPar:
    --par_depth;
    break;
  case Token::LeftBrace:
    ++brace_depth;
    break;
  case Token::RightBrace:
    --brace_depth;
    break;
  case Token::LeftBracket:
    ++bracket_depth;
    break;
  case Token::RightBracket:
    --bracket_depth;
    break;
  default:
    break;
  }
}

void DelimitersCounter::feed(const Token& tok)
{
  relaxed_feed(tok);

  if(invalid())
    throw SyntaxError{ ParserError::UnexpectedFragmentEnd }; // @TODO: create better error enum
}

bool DelimitersCounter::balanced() const
{
  return par_depth == 0 && brace_depth == 0 && bracket_depth == 0;
}

bool DelimitersCounter::invalid() const
{
  return par_depth < 0 || brace_depth < 0 || bracket_depth < 0;
}

Fragment::Fragment(const ParserContext& context)
  : m_begin(context.tokens().begin()),
    m_end(context.tokens().end())
{

}

Fragment::Fragment(iterator begin, iterator end)
  : m_begin(begin),
    m_end(end)
{

}

Fragment::Fragment(iterator begin, iterator end, Type<DelimiterPair>)
  : m_begin(begin),
    m_end(end)
{
  DelimitersCounter counter;
  counter.feed(*m_begin);

  if (counter.balanced())
    throw std::runtime_error{ "bad call to Fragment ctor" };

  ++m_begin;

  auto it = m_begin;

  while (it != m_end)
  {
    counter.feed(*it);

    if (counter.balanced())
    {
      m_end = it;
      return;
    }
    else
    {
      ++it;
    }
  }

  throw SyntaxError{ ParserError::UnexpectedFragmentEnd }; // @TODO: create better error enum
}

Fragment::Fragment(iterator begin, iterator end, Type<Statement>)
  : m_begin(begin),
    m_end(end)
{
  DelimitersCounter counter;

  auto it = m_begin;

  while (it != m_end)
  {
    counter.feed(*it);

    if (it->id == Token::Semicolon)
    {
      if (counter.balanced())
      {
        m_end = it;
        return;
      }
      else
      {
        // @TODO: we could check that we are inside brackets
        ++it;
      }
    }
    else
    {
      ++it;
    }
  }

  throw SyntaxError{ ParserError::UnexpectedFragmentEnd }; // @TODO: create better error enum
}

Fragment::Fragment(iterator begin, iterator end, Type<ListElement>)
  : m_begin(begin),
    m_end(end)
{
  DelimitersCounter counter;

  auto it = m_begin;

  while (it != m_end)
  {
    counter.feed(*it);

    if (it->id == Token::Comma)
    {
      if (counter.balanced())
      {
        m_end = it;
        return;
      }
      else
      {
        // @TODO: we could check that we are inside brackets
        ++it;
      }
    }
    else
    {
      ++it;
    }
  }

  if(!counter.balanced())
    throw SyntaxError{ ParserError::UnexpectedFragmentEnd }; // @TODO: create better error enum
}

std::vector<Token>::const_iterator Fragment::begin() const
{
  return m_begin;
}

std::vector<Token>::const_iterator Fragment::end() const
{
  return m_end;
}

size_t Fragment::size() const
{
  return std::distance(begin(), end());
}

Fragment Fragment::mid(iterator pos) const
{
  return Fragment(pos, end());
}

bool Fragment::tryBuildTemplateFragment(iterator begin, iterator end, iterator& o_begin, iterator& o_end, bool& o_half_consumed_right_right)
{
  if (begin->id != Token::LeftAngle)
    return false;

  DelimitersCounter counter;
  int angle_counter = 0;

  for (auto it = begin; it != end; ++it)
  {
    counter.relaxed_feed(*it);

    if (counter.invalid())
      return false;

    if (it->id == Token::RightAngle)
    {
      if (counter.balanced())
      {
        --angle_counter;

        if (angle_counter == 0)
        {
          o_begin = begin + 1;
          o_end = it;
          o_half_consumed_right_right = false;
          return true;
        }
      }
    }
    else if (it->id == Token::RightRightAngle)
    {
      if (counter.balanced())
      {
        if (angle_counter == 1 || angle_counter == 2)
        {
          angle_counter = 0;
          o_half_consumed_right_right = true;
          o_begin = begin + 1;
          o_end = it;
          return true;
        }
        else
        {
          angle_counter -= 2;
        }
      }
    }
    else if (it->id == Token::LeftAngle)
    {
      if (counter.balanced())
        ++angle_counter;
    }
  }

  return false;
}

ParserContext::ParserContext(const SourceFile & src)
  : mSource(src)
{
  Lexer lexer;
  lexer.setSource(mSource);

  while (!lexer.atEnd())
  {
    const Token t = lexer.read();
    if (isDiscardable(t))
      continue;
    m_tokens.push_back(t);
  }
}

ParserContext::ParserContext(std::vector<Token> tokens)
  : m_tokens(std::move(tokens))
{

}

ParserContext::~ParserContext()
{
  mSource = SourceFile{};
  m_tokens.clear();
}

bool ParserContext::isDiscardable(const Token & t) const
{
  return t == Token::MultiLineComment || t == Token::SingleLineComment;
}


ParserBase::ParserBase(std::shared_ptr<ParserContext> shared_context, const Fragment& frag)
  : m_context(shared_context),
    m_fragment(frag),
    m_iterator(m_fragment.begin())
{

}

ParserBase::~ParserBase()
{

}

void ParserBase::reset(std::shared_ptr<ParserContext> shared_context, const Fragment& fragment)
{
  m_context = shared_context;
  m_fragment = fragment;
  m_iterator = m_fragment.begin();
}

std::shared_ptr<ast::AST> ParserBase::ast() const
{
  return context()->mAst;
}

SourceLocation ParserBase::location() const
{
  SourceLocation loc;
  loc.m_source = ast()->source;
  
  if (!atEnd())
  {
    const Token tok = unsafe_peek();
    loc.m_pos = ast()->position(tok);
  }
  else
  {
    loc.m_pos.pos = ast()->source.content().length();
  }

  return loc;
}

bool ParserBase::atEnd() const
{
  return m_iterator == m_fragment.end();
}

bool ParserBase::eof() const
{
  return m_iterator == context()->tokens().end();
}

Token ParserBase::read()
{
  if (atEnd())
    throw SyntaxError{ ParserError::UnexpectedEndOfInput };

  return *(m_iterator++);
}

Token ParserBase::unsafe_read()
{
  return *(m_iterator++);
}

Token ParserBase::read(const Token::Id & type)
{
  Token ret = read();

  if (ret != type)
    throw SyntaxError{ ParserError::UnexpectedToken, errors::UnexpectedToken{ret, type} };

  return ret;
}

Token ParserBase::peek() const
{
  return *m_iterator;
}

Token ParserBase::peek(size_t n) const
{
  return *(std::next(m_iterator, n));
}

Token ParserBase::unsafe_peek() const
{
  return *(m_iterator);
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

Fragment ParserBase::midfragment() const
{
  return fragment().mid(iterator());
}

void ParserBase::seek(Fragment::iterator it)
{
  m_iterator = it;
}


LiteralParser::LiteralParser(std::shared_ptr<ParserContext> shared_context, const Fragment& fragment)
  : ParserBase(shared_context, fragment)
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
    return ast::BoolLiteral::New(lit, ast());
  case Token::IntegerLiteral:
  case Token::BinaryLiteral:
  case Token::OctalLiteral:
  case Token::HexadecimalLiteral:
    return ast::IntegerLiteral::New(lit, ast());
  case Token::DecimalLiteral:
    return ast::FloatingPointLiteral::New(lit, ast());
  case Token::StringLiteral:
    return ast::StringLiteral::New(lit, ast());
  case Token::UserDefinedLiteral:
    return ast::UserDefinedLiteral::New(lit, ast());
  default:
    break;
  }

  throw SyntaxError{ ParserError::ExpectedLiteral, errors::ActualToken{lit} };
}

ExpressionParser::ExpressionParser(std::shared_ptr<ParserContext> shared_context, const Fragment& fragment)
  : ParserBase(shared_context, fragment)
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
  return op != Operator::Null;
}

bool ExpressionParser::isInfixOperator(const Token & tok)
{
  auto op = ast::OperatorName::getOperatorId(tok, ast::OperatorName::InfixOp);
  return op != Operator::Null;
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

    Fragment subexpr_fragment{ iterator(), fragment().end(), Fragment::Type<Fragment::DelimiterPair>() };
    read(Token::LeftPar); // @TODO: is it necessary ?

    ExpressionParser exprParser{ context(), subexpr_fragment };
    operand = exprParser.parse();
    seek(subexpr_fragment.end());
    read(Token::RightPar);
  }
  else if (t == Token::LeftBracket) // array
  {
    LambdaParser lambda_parser{ context(), midfragment() };
    operand = lambda_parser.parse();
    seek(lambda_parser.iterator());
  }
  else if (t == Token::LeftBrace)
  {
    auto list = ast::ListExpression::New(unsafe_peek());
    Fragment list_fragment{ iterator(), fragment().end(), Fragment::Type<Fragment::DelimiterPair>() };
    read(Token::LeftBrace); // @TODO: is it necessary ?

    while (iterator() != list_fragment.end()) // @TODO: remove Fragment::atEnd()
    {
      Fragment list_element_fragment{ list_fragment.mid(iterator()), Fragment::Type<Fragment::ListElement>() };
      ExpressionParser exprparser{ context(), list_element_fragment };
      list->elements.push_back(exprparser.parse());

      seek(list_element_fragment.end());

      if (iterator() != list_fragment.end())
        read(Token::Comma);
    }

    list->right_brace = read(Token::RightBrace);

    operand = list;
  }
  else if (t.isLiteral())
  {
    LiteralParser p{ context(), midfragment() };
    operand = p.parse();
    seek(p.iterator());
  }
  else
  {
    IdentifierParser idParser{ context(), midfragment() };
    operand = idParser.parse();
    seek(idParser.iterator());

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
      IdentifierParser idParser{ context(), midfragment(), IdentifierParser::ParseSimpleId | IdentifierParser::ParseTemplateId };
      auto memberName = idParser.parse();
      operand = ast::Operation::New(t, operand, memberName);
      seek(idParser.iterator());
    }
    else if (t == Token::LeftPar)
    {
      Fragment argument_list_fragment{ midfragment(), Fragment::Type<Fragment::DelimiterPair>() };

      const Token leftpar = unsafe_read();

      ExpressionListParser argsParser{ context(), argument_list_fragment };
      std::vector<std::shared_ptr<ast::Expression>> args = parse_and_seek(argsParser);
      const Token rightpar = read(Token::RightPar);
      operand = ast::FunctionCall::New(operand, leftpar, std::move(args), rightpar);
    }
    else if (t == Token::LeftBracket) // subscript operator
    {
      Fragment subscript_fragment{ midfragment(), Fragment::Type<Fragment::DelimiterPair>() };

      auto leftBracket = read();

      Token next = peek();

      if (subscript_fragment.size() == 0)
        throw SyntaxError{ ParserError::InvalidEmptyBrackets };

      ExpressionParser exprParser{ context(), subscript_fragment };
      std::shared_ptr<ast::Expression> arg = parse_and_seek(exprParser);
      operand = ast::ArraySubscript::New(operand, leftBracket, arg, read());
    }
    else if (t == Token::LeftBrace && operand->is<ast::Identifier>())
    {
      Fragment bracelist_fragment{ midfragment(), Fragment::Type<Fragment::DelimiterPair>() };

      auto type_name = std::dynamic_pointer_cast<ast::Identifier>(operand);
      const Token & left_brace = unsafe_read();
      
      ExpressionListParser argsParser{ context(), bracelist_fragment };
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
        IdentifierParser idParser{ context(), midfragment() };
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
  const auto numOp = std::distance(opBegin, opEnd);
  auto getOp = [opBegin](int num) -> Token {
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
      return Operator::precedence(ConditionalOperator);
    else
      return Operator::precedence(ast::OperatorName::getOperatorId(tok, ast::OperatorName::InfixOp));
  };

  int index = 0;
  int preced = getPrecedence(getOp(index));
  for (int i(1); i < numOp; ++i)
  {
    int p = getPrecedence(getOp(i));
    if (p > preced)
    {
      index = i;
      preced = p;
    }
    else if (p == preced)
    {
      if (Operator::associativity(preced) == Operator::LeftToRight)
        index = i;
    }
  }

  if (getOp(index) == Token::QuestionMark)
  {
    auto cond = buildExpression(exprBegin, exprBegin + (index + 1), opBegin, opBegin + index);

    int colonIndex = -1;
    for (int j = numOp - 1; j > index; --j)
    {
      if (getOp(j) == Token::Colon)
      {
        colonIndex = j;
        break;
      }
    }

    if (colonIndex == -1)
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


LambdaParser::LambdaParser(std::shared_ptr<ParserContext> shared_context, const Fragment& fragment)
  : ParserBase(shared_context, fragment)
  , mDecision(Undecided)
{

}

std::shared_ptr<ast::Expression> LambdaParser::parse()
{
  mArray = ast::ArrayExpression::New(peek());
  mLambda = ast::LambdaExpression::New(unsafe_peek());

  readBracketContent();

  if (atEnd()) 
  {
    if (mDecision == ParsingLambda)
    {
      throw SyntaxError{ ParserError::UnexpectedFragmentEnd };
    }
    else 
    {
      setDecision(ParsingArray);
      return mArray;
    }
  }

  if (peek() != Token::LeftPar) 
  {
    if (mDecision == ParsingLambda)
    {
      throw SyntaxError{ ParserError::UnexpectedToken, errors::UnexpectedToken{unsafe_peek(), Token::LeftPar} };
    }
    else
    {
      setDecision(ParsingArray);
      return mArray;
    }
  }

  setDecision(ParsingLambda);

  readParams();

  mLambda->body = readBody();
  return mLambda;
}

void LambdaParser::readBracketContent()
{
  Fragment capture_list_fragment{ midfragment(), Fragment::Type<Fragment::DelimiterPair>() };

  read(Token::LeftBracket);

  while (iterator() != capture_list_fragment.end())
  {
    Fragment listelem_fragment{ iterator(), capture_list_fragment.end(), Fragment::Type<Fragment::ListElement>() };

    if(mDecision == Undecided || mDecision == ParsingArray)
    {
      ExpressionParser ep{ context(), listelem_fragment };
      try
      {
        auto elem = ep.parse();
        mArray->elements.push_back(elem);
      }
      catch (const SyntaxError &)
      {
        if (mDecision == ParsingArray)
          throw;
        mDecision = ParsingLambda;
        mArray = nullptr;
      }
    }


    if (mDecision == Undecided || mDecision == ParsingLambda)
    {
      LambdaCaptureParser capp{ context(), listelem_fragment };

      if (!capp.detect())
      {
        if (mDecision == ParsingLambda)
          throw SyntaxError{ ParserError::CouldNotParseLambdaCapture };

        setDecision(ParsingArray);

        seek(listelem_fragment.end());

        if (peek() == Token::Comma)
          unsafe_read();

        continue;
      }

      try
      {
        auto capture = capp.parse();
        mLambda->captures.push_back(capture);
      }
      catch (const SyntaxError&)
      {
        if (mDecision == ParsingLambda)
          throw;
        setDecision(ParsingArray);
      }
    }

    seek(listelem_fragment.end());

    if (peek() == Token::Comma)
      unsafe_read();
  }

  const Token rb = read(Token::RightBracket);

  if (mArray)
    mArray->rightBracket = rb;
  if (mLambda)
    mLambda->rightBracket = rb;
}

void LambdaParser::readParams()
{
  assert(mDecision == ParsingLambda);

  Fragment params_fragment{ midfragment(), Fragment::Type<Fragment::DelimiterPair>() };

  mLambda->leftPar = read(Token::LeftPar);

  while (iterator() != params_fragment.end())
  {
    Fragment listfrag{ params_fragment.mid(iterator()), Fragment::Type<Fragment::ListElement>() };

    FunctionParamParser pp{ context(), listfrag };
    auto param = pp.parse();
    mLambda->params.push_back(param);

    seek(listfrag.end());

    if (peek() == Token::Comma)
      unsafe_read();
  }

  mLambda->rightPar = read(Token::RightPar);
}


std::shared_ptr<ast::CompoundStatement> LambdaParser::readBody()
{
  if (atEnd())
    throw SyntaxError{ ParserError::UnexpectedEndOfInput };

  if (peek() != Token::LeftBrace)
    throw SyntaxError{ ParserError::UnexpectedToken, errors::UnexpectedToken{unsafe_peek(), Token::LeftBrace} };

  ProgramParser pParser{ context(), midfragment() };
  auto ret = std::dynamic_pointer_cast<ast::CompoundStatement>(pParser.parseStatement());
  seek(pParser.iterator());
  return ret;
}


LambdaCaptureParser::LambdaCaptureParser(std::shared_ptr<ParserContext> shared_context, const Fragment& fragment)
  : ParserBase(shared_context, fragment)
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

  IdentifierParser idpar{ context(), midfragment(), IdentifierParser::ParseOnlySimpleId };
  cap.name = parse_and_seek(idpar)->as<ast::SimpleIdentifier>().name;
  if (atEnd())
    return cap;
  cap.assignmentSign = read(Token::Eq);
  ExpressionParser ep{ context(), midfragment() };
  cap.value = parse_and_seek(ep);
  return cap;
}


LambdaParser::Decision LambdaParser::decision() const
{
  return mDecision;
}

void LambdaParser::setDecision(Decision d)
{
  mDecision = d;
  if (mDecision == ParsingArray)
    mLambda = nullptr;
  else if (mDecision == ParsingLambda)
    mArray = nullptr;
}


ProgramParser::ProgramParser(std::shared_ptr<ParserContext> shared_context)
  : ProgramParser(shared_context, Fragment{*shared_context})
{

}

ProgramParser::ProgramParser(std::shared_ptr<ParserContext> shared_context, const Fragment& frag)
  : ParserBase(shared_context, frag)
{

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
  DeclParser dp{ context(), midfragment() };
  if (dp.detectDecl())
    return parse_and_seek(dp);
  
  seek(savePoint);

  Fragment sentinel{ midfragment(), Fragment::Type<Fragment::Statement>() };
  ExpressionParser ep{ context(), sentinel };
  auto expr = parse_and_seek(ep);
  auto semicolon = read(Token::Semicolon);
  return ast::ExpressionStatement::New(expr, semicolon);
}

std::shared_ptr<ast::ClassDecl> ProgramParser::parseClassDeclaration()
{
  throw SyntaxError{ ParserError::UnexpectedToken, errors::UnexpectedToken{peek(), Token::Invalid} };
}

std::shared_ptr<ast::EnumDeclaration> ProgramParser::parseEnumDeclaration()
{
  /// TODO : shouldn't we also throw here but not in Parser (as in parseClassDeclaration()) 
  EnumParser ep{ context(), midfragment() };
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

  Fragment exprstatement_fragment{ midfragment(), Fragment::Type<Fragment::Statement>() };

  ExpressionParser exprParser{ context(), exprstatement_fragment };
  auto returnValue = parse_and_seek(exprParser);

  Token semicolon = read();
  assert(semicolon == Token::Semicolon);

  return ast::ReturnStatement::New(kw, returnValue);
}

std::shared_ptr<ast::CompoundStatement> ProgramParser::parseCompoundStatement()
{
  Fragment frag{ midfragment(), Fragment::Type<Fragment::DelimiterPair>() };

  Token leftBrace = read();
  assert(leftBrace == Token::LeftBrace);

  ProgramParser progParser{ context(), frag };

  std::vector<std::shared_ptr<ast::Statement>> statements = progParser.parseProgram();
  seek(progParser.iterator());
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
    Fragment condition{ midfragment(), Fragment::Type<Fragment::DelimiterPair>() };
    const Token leftpar = read(Token::LeftPar);

    ExpressionParser exprParser{ context(), condition };
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
    Fragment condition{ midfragment(), Fragment::Type<Fragment::DelimiterPair>() };
    const Token leftpar = read(Token::LeftPar);
    ExpressionParser exprParser{ context(), condition };
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

  Fragment init_loop_incr{ midfragment(), Fragment::Type<Fragment::DelimiterPair>() };

  const Token leftpar = read(Token::LeftPar);

  auto forLoop = ast::ForLoop::New(forkw);

  {
    DeclParser initParser{ context(), init_loop_incr };
    if (!initParser.detectDecl())
    {
      Fragment init{ init_loop_incr, Fragment::Type<Fragment::Statement>() };
      ExpressionParser exprParser{ context(), init };
      auto initExpr = parse_and_seek(exprParser);
      auto semicolon = read(Token::Semicolon);
      forLoop->initStatement = ast::ExpressionStatement::New(initExpr, semicolon);
    }
    else
    {
      initParser.setDecision(DeclParser::ParsingVariable);
      forLoop->initStatement = parse_and_seek(initParser);
    }
  }

  {
    Fragment condition{ init_loop_incr.mid(iterator()), Fragment::Type<Fragment::Statement>() };
    ExpressionParser exprParser{ context(), condition };
    forLoop->condition = parse_and_seek(exprParser);
    read(Token::Semicolon);
  }

  {

    Fragment loopIncr{ init_loop_incr.mid(iterator()) };
    ExpressionParser exprParser{ context(), loopIncr };
    forLoop->loopIncrement = parse_and_seek(exprParser);
    read(Token::RightPar);
  }

  forLoop->body = parseStatement();

  return forLoop;
}

std::shared_ptr<ast::Typedef> ProgramParser::parseTypedef()
{
  const parser::Token typedef_tok = unsafe_read();

  TypeParser tp{ context(), midfragment() };
  const ast::QualifiedType qtype = parse_and_seek(tp);

  IdentifierParser idp{ context(), midfragment(), IdentifierParser::ParseOnlySimpleId };
  /// TODO: add overload to IdentifierParser that only parses symple identifier.
  const auto name = std::static_pointer_cast<ast::SimpleIdentifier>(parse_and_seek(idp));

  const parser::Token semicolon = read(parser::Token::Semicolon);

  return ast::Typedef::New(typedef_tok, qtype, name);
}

std::shared_ptr<ast::Declaration> ProgramParser::parseNamespace()
{
  NamespaceParser np{ context(), midfragment() };
  return parse_and_seek(np);
}

std::shared_ptr<ast::Declaration> ProgramParser::parseUsing()
{
  UsingParser np{ context(), midfragment() };
  return parse_and_seek(np);
}

std::shared_ptr<ast::ImportDirective> ProgramParser::parseImport()
{
  ImportParser ip{ context(), midfragment() };
  return parse_and_seek(ip);
}

std::shared_ptr<ast::TemplateDeclaration> ProgramParser::parseTemplate()
{
  TemplateParser tp{ context(), midfragment() };
  return parse_and_seek(tp);
}



IdentifierParser::IdentifierParser(std::shared_ptr<ParserContext> shared_context, const Fragment& fragment, int opts)
  : ParserBase(shared_context, fragment)
  , mOptions(opts)
{

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
  if(op.isOperator())
    return ast::OperatorName::New(opkw, read());
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

    IdentifierParser idp{ context(), midfragment(), IdentifierParser::ParseOnlySimpleId };
    /// TODO: add overload to remove this cast
    auto suffixName = std::static_pointer_cast<ast::SimpleIdentifier>(parse_and_seek(idp));
    return ast::LiteralOperatorName::New(opkw, op, suffixName->name, ast());
  }
  else if (op == Token::UserDefinedLiteral)
  {
    op = unsafe_read();
    const auto& str = op.toString();

    if(str.find("\"\"") != 0)
      throw SyntaxError{ ParserError::ExpectedEmptyStringLiteral, errors::ActualToken{op} }; /// TODO ? should this have a different error than the previous

    Token quotes{ Token::StringLiteral, Token::Literal, StringView(op.text().data(), 2) };
    Token suffixName{ Token::UserDefinedName, Token::Identifier, StringView(op.text().data() + 2, op.text().size() - 2) };
    return ast::LiteralOperatorName::New(opkw, quotes, suffixName, ast());
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
    const auto savepoint = iterator();

    try 
    {
      Fragment::iterator frag_begin;
      Fragment::iterator frag_end;
      Fragment::iterator current_frag_end = context()->half_consumed_right_right_angle && fragment().end()->id == Token::RightRightAngle ?
        fragment().end() + 1 : fragment().end();
      bool half_consumed_right_right = false;

      bool ok = Fragment::tryBuildTemplateFragment(iterator(), current_frag_end,
        frag_begin, frag_end, half_consumed_right_right);

      if (ok)
      {
        RaiiRightRightAngleGuard guard{ context().get() };
        if(guard.value && half_consumed_right_right)
          context()->half_consumed_right_right_angle = false;
        else if(half_consumed_right_right)
          context()->half_consumed_right_right_angle = true;

        Fragment targlist_frag{ frag_begin, frag_end };
        ret = readTemplateArguments(base, targlist_frag);

        if (!guard.value)
          unsafe_read();
      }
      else
      {
        seek(savepoint);
      }
    }
    catch (const SyntaxError& )
    {
      seek(savepoint);
      return ret;
    }
    catch (const std::runtime_error &)
    {
      seek(savepoint);
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

      IdentifierParser idparser{ context(), midfragment(), IdentifierParser::ParseTemplateId };
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

std::shared_ptr<ast::Identifier> IdentifierParser::readTemplateArguments(const Token& base, Fragment targlist_frag)
{
  const Token leftangle = read();

  std::vector<ast::NodeRef> args;

  while (iterator() != targlist_frag.end())
  {
    Fragment frag{ targlist_frag.mid(iterator()), Fragment::Type<Fragment::ListElement>() };
    TemplateArgParser argparser{ context(), frag };
    args.push_back(parse_and_seek(argparser));
    
    if (iterator() != targlist_frag.end())
      read(Token::Comma);
  }

  Token right_angle = unsafe_peek();

  return ast::TemplateIdentifier::New(base, std::move(args), leftangle, right_angle);
}

TemplateArgParser::TemplateArgParser(std::shared_ptr<ParserContext> shared_context, const Fragment& fragment)
  : ParserBase(shared_context, fragment)
{

}

std::shared_ptr<ast::Node> TemplateArgParser::parse()
{
  auto p = iterator();

  {
    RaiiRightRightAngleGuard guard{ context().get() };

    TypeParser tp{ context(), midfragment() };
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

  ExpressionParser ep{ context(), midfragment() };
  return parse_and_seek(ep);
}



TypeParser::TypeParser(std::shared_ptr<ParserContext> shared_context, const Fragment& fragment)
  : ParserBase(shared_context, fragment)
  , mReadFunctionSignature(true)
{

}

ast::QualifiedType TypeParser::parse()
{
  ast::QualifiedType ret;

  if (peek() == Token::Const)
    ret.constQualifier = unsafe_read();

  IdentifierParser idparser{ context(), midfragment() };
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

  if (unsafe_peek() == Token::LeftPar && mReadFunctionSignature) {
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


bool TypeParser::detect()
{
  if (peek() == Token::Const)
    return true;
  return peek().isIdentifier();
}

ast::QualifiedType TypeParser::tryReadFunctionSignature(const ast::QualifiedType & rt)
{
  ast::QualifiedType ret;
  ret.functionType = std::make_shared<ast::FunctionType>();
  ret.functionType->returnType = rt;

  Fragment params_frag{ midfragment(), Fragment::Type<Fragment::DelimiterPair>() };
  const Token leftPar = unsafe_read();
  while (iterator() != params_frag.end())
  {
    Fragment listfrag{ params_frag.mid(iterator()), Fragment::Type<Fragment::ListElement>() };
    TypeParser tp{ context(), listfrag };
    auto param = parse_and_seek(tp);
    ret.functionType->params.push_back(param);

    if (iterator() != listfrag.end())
      throw SyntaxError{ ParserError::UnexpectedToken, errors::UnexpectedToken{ peek(), Token::Invalid} };

    if (peek() == Token::Comma)
      unsafe_read();
  }

  read(Token::RightPar);

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



FunctionParamParser::FunctionParamParser(std::shared_ptr<ParserContext> shared_context, const Fragment& fragment)
  : ParserBase(shared_context, fragment)
{

}

ast::FunctionParameter FunctionParamParser::parse()
{
  ast::FunctionParameter fp;

  TypeParser tp{ context(), midfragment() };
  fp.type = parse_and_seek(tp);

  if (atEnd())
    return fp;

  IdentifierParser ip{ context(), midfragment(), IdentifierParser::ParseOnlySimpleId };
  /// TODO: add overload that returns SimpleIdentifier
  auto name = std::static_pointer_cast<ast::SimpleIdentifier>(parse_and_seek(ip));
  fp.name = name->name;

  if (atEnd())
    return fp;

  const Token eqSign = read(Token::Eq);
  ExpressionParser ep{ context(), midfragment() };
  auto defaultVal = parse_and_seek(ep);
  fp.defaultValue = defaultVal;

  return fp;
}

ExpressionListParser::ExpressionListParser(std::shared_ptr<ParserContext> shared_context, const Fragment& fragment)
  : ParserBase(shared_context, fragment)
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
    Fragment f{ midfragment(), Fragment::Type<Fragment::ListElement>()  };

    ExpressionParser expr{ context(), f };
    auto e = parse_and_seek(expr);
    result.push_back(e);
    if (!atEnd())
      read(); // reads the comma
  }

  return result;
}


DeclParser::DeclParser(std::shared_ptr<ParserContext> shared_context, const Fragment& fragment, std::shared_ptr<ast::Identifier> cn)
  : ParserBase(shared_context, fragment)
  , mDecision(Undecided)
  , mClassName(cn)
  , mParamsAlreadyRead(false)
  , mDeclaratorOptions(IdentifierParser::ParseSimpleId | IdentifierParser::ParseOperatorName)
{

}

DeclParser::~DeclParser()
{

}


void DeclParser::readOptionalDeclSpecifiers()
{
  if (readOptionalVirtual())
  {
    if (!isParsingMember())
      throw SyntaxError{ ParserError::IllegalUseOfKeyword, errors::KeywordToken{mVirtualKw} };
  }

  readOptionalStatic();

  if (readOptionalExplicit())
  {
    if (!isParsingMember())
      throw SyntaxError{ ParserError::IllegalUseOfKeyword, errors::KeywordToken{mExplicitKw} };
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
  TypeParser tp{ context(), midfragment() };
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
      mType = ast::QualifiedType{};
      return true;
    }
  }

  return false;
}

bool DeclParser::readDeclarator()
{
  IdentifierParser ip{ context(), midfragment() };
  ip.setOptions(mDeclaratorOptions);

  try
  {
    mName = parse_and_seek(ip);
  }
  catch (const SyntaxError&)
  {
    if (mDecision != Undecided)
      throw;
    mDecision = NotADecl;
    return false;
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
    return true;
  }
  else if (mName->is<ast::LiteralOperatorName>())
  {
    mDecision = ParsingFunction;
    auto lon = ast::OperatorOverloadDecl::New(mName);
    lon->returnType = mType;
    mFuncDecl = lon;
    return true;
  }
  else if (mVirtualKw.isValid())
  {
    // We could detect that a little bit earlier but this avoids 
    // some code duplication
    mDecision = ParsingFunction;
    mFuncDecl = ast::FunctionDecl::New(mName);
    mFuncDecl->returnType = mType;
    mFuncDecl->virtualKeyword = mVirtualKw;
    return true;
  }

  return false;
}

// TODO : restructure this function as follow :
// - readOptionalDeclSpecifiers()
// - detectBeforeReadingTypeSpecifier()
// - readTypeSpecifier()
// - detectBeforeReadingDeclarator() // also used to correct ctor misinterpreted as typespecifier
// - readDeclarator()
// - detectFromDeclarator()
bool DeclParser::detectDecl()
{
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
    mFuncDecl->returnType = mType;
    mFuncDecl->staticKeyword = mStaticKw;
    mFuncDecl->virtualKeyword = mVirtualKw;

    assert(mName->is<ast::SimpleIdentifier>());
    mVarDecl = ast::VariableDecl::New(mType, std::static_pointer_cast<ast::SimpleIdentifier>(mName));
    mVarDecl->staticSpecifier = mStaticKw;
  }
  else
  {
    throw SyntaxError{ ParserError::UnexpectedToken, errors::UnexpectedToken{unsafe_peek(), Token::Invalid} };
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
      throw SyntaxError{ ParserError::UnexpectedToken, errors::UnexpectedToken{unsafe_peek(), Token::Invalid} };

    mDecision = ParsingFunction;
    mVarDecl = nullptr;
    mFuncDecl->body = readFunctionBody();
    return mFuncDecl;
  }
  else if (peek() == Token::Semicolon)
  {
    if (mDecision == ParsingFunction)
      throw SyntaxError{ ParserError::UnexpectedToken, errors::UnexpectedToken{unsafe_peek(), Token::LeftBrace} };

    mVarDecl->semicolon = read();
    return mVarDecl;
  }

  throw SyntaxError{ ParserError::UnexpectedToken, errors::UnexpectedToken{unsafe_peek(), Token::Invalid} };
}

std::shared_ptr<ast::VariableDecl> DeclParser::parseVarDecl()
{
  if (peek() == Token::Eq)
  {
    const Token eqsign = read();
    Fragment exprFrag{ midfragment(), Fragment::Type<Fragment::Statement>() };
    ExpressionParser ep{ context(), exprFrag };
    auto expr = parse_and_seek(ep);
    mVarDecl->init = ast::AssignmentInitialization::New(eqsign, expr);
  }
  else if (peek() == Token::LeftBrace)
  {
    Fragment sentinel{ midfragment(), Fragment::Type<Fragment::DelimiterPair>() };
    const Token leftBrace = read();
    ExpressionListParser argsParser{ context(), sentinel };
    auto args = parse_and_seek(argsParser);
    const Token rightbrace = read(Token::RightBrace);
    mVarDecl->init = ast::BraceInitialization::New(leftBrace, std::move(args), rightbrace);
  }
  else if (peek() == Token::LeftPar)
  {
    Fragment sentinel{ midfragment(), Fragment::Type<Fragment::DelimiterPair>() };
    const Token leftpar = read();
    ExpressionListParser argsParser{ context(), sentinel };
    auto args = parse_and_seek(argsParser);
    const Token rightpar = read(Token::RightPar);
    mVarDecl->init = ast::ConstructorInitialization::New(leftpar, std::move(args), rightpar);
  }
  else
  {
    /// TODO: should we assert here ?
    assert(peek() == Token::Semicolon);
  }

  const Token semicolon = read(Token::Semicolon);

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
  const Token colon = unsafe_read();

  for (;;)
  {
    IdentifierParser idp{ context(), midfragment(), IdentifierParser::ParseOnlySimpleId | IdentifierParser::ParseTemplateId };
    auto id = parse_and_seek(idp);
    if (peek() == Token::LeftBrace)
    {
      Fragment sentinel{ midfragment(), Fragment::Type<Fragment::DelimiterPair>() };
      const Token leftBrace = unsafe_read();
      ExpressionListParser argsParser{ context(), sentinel };
      auto args = parse_and_seek(argsParser);
      const Token rightBrace = read(Token::RightBrace);
      auto braceinit = ast::BraceInitialization::New(leftBrace, std::move(args), rightBrace);
      ctor->memberInitializationList.push_back(ast::MemberInitialization{ id, braceinit });
    }
    else if (peek() == Token::LeftPar)
    {
      Fragment sentinel{ midfragment(), Fragment::Type<Fragment::DelimiterPair>() };
      const Token leftpar = unsafe_read();
      ExpressionListParser argsParser{ context(), sentinel };
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
  const Token leftpar = read(Token::LeftPar);
  const Token rightpar = read(Token::RightPar);

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
  Fragment sentinel{ midfragment(), Fragment::Type<Fragment::DelimiterPair>() };

  const Token leftpar = read(Token::LeftPar);

  while (iterator() != sentinel.end())
  {
    Fragment listfrag{ sentinel.mid(iterator()), Fragment::Type<Fragment::ListElement>() };

    FunctionParamParser pp{ context(), listfrag };
    auto param = parse_and_seek(pp);
    mFuncDecl->params.push_back(param);

    if (peek() == Token::Comma)
      read(Token::Comma);
  }

  read(Token::RightPar);
}


void DeclParser::readArgsOrParams()
{
  Fragment sentinel{ midfragment(), Fragment::Type<Fragment::DelimiterPair>() };

  const Token leftPar = read();
  assert(leftPar == Token::LeftPar);

  if (mDecision == Undecided || mDecision == ParsingVariable)
    mVarDecl->init = ast::ConstructorInitialization::New(leftPar, {}, parser::Token{});

  while (iterator() != sentinel.end())
  {
    Fragment listfrag{ sentinel.mid(iterator()), Fragment::Type<Fragment::ListElement>() };

    if (mDecision == Undecided || mDecision == ParsingVariable)
    {
      ExpressionParser ep{ context(), listfrag };
      try
      {
        auto expr = parse_and_seek(ep);
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

    auto position = iterator();

    if (mDecision == Undecided || isParsingFunction())
    {
      seek(listfrag.begin());

      FunctionParamParser pp{ context(), listfrag };
      try
      {
        auto param = parse_and_seek(pp);
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

    if (iterator() != listfrag.end())
      seek(position);
    assert(iterator() == listfrag.end());

    if (peek() == Token::Comma)
      unsafe_read();
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
    throw SyntaxError{ ParserError::UnexpectedToken, errors::UnexpectedToken{unsafe_peek(), Token::Invalid} };

  mDecision = ParsingFunction;
  mVarDecl = nullptr;
  mFuncDecl->constQualifier = read();

  return true;
}

bool DeclParser::readOptionalDeleteSpecifier()
{
  if (mDecision == ParsingVariable)
    return false;

  if (peek() != Token::Eq)
    return false;

  auto p = iterator();
  const Token eqSign = read();

  if (atEnd())
    throw SyntaxError{ ParserError::UnexpectedEndOfInput };

  if (peek() != Token::Delete)
  {
    seek(p);
    return false;
  }

  const Token delSpec = read();
  mFuncDecl->deleteKeyword = delSpec;

  mDecision = ParsingFunction;
  mVarDecl = nullptr;

  if (atEnd())
    throw SyntaxError{ ParserError::UnexpectedEndOfInput };

  read(Token::Semicolon);

  return true;
}

bool DeclParser::readOptionalDefaultSpecifier()
{
  if (mDecision == ParsingVariable)
    return false;

  if (peek() != Token::Eq)
    return false;

  auto p = iterator();
  const Token eqSign = read();

  if (atEnd())
    throw SyntaxError{ ParserError::UnexpectedEndOfInput };

  if (peek() != Token::Default)
  {
    seek(p);
    return false;
  }

  const Token defspec = read();
  mFuncDecl->defaultKeyword = defspec;

  mDecision = ParsingFunction;
  mVarDecl = nullptr;

  read(Token::Semicolon);

  return true;
}

bool DeclParser::readOptionalVirtualPureSpecifier()
{
  if (mDecision == ParsingVariable)
    return false;

  if (peek() != Token::Eq)
    return false;

  auto p = iterator();
  const Token eqSign = unsafe_read();

  if (peek() != Token::OctalLiteral)
  {
    seek(p);
    return false;
  }

  mFuncDecl->virtualPure = read();

  if (!mFuncDecl->virtualPure.isZero())
    throw SyntaxError{ ParserError::UnexpectedToken, errors::UnexpectedToken{mFuncDecl->virtualPure, Token::OctalLiteral} };

  mDecision = ParsingFunction;
  mVarDecl = nullptr;

  read(Token::Semicolon);

  return true;
}


std::shared_ptr<ast::CompoundStatement> DeclParser::readFunctionBody()
{
  if (peek() != Token::LeftBrace)
    throw SyntaxError{ ParserError::UnexpectedToken, errors::UnexpectedToken{unsafe_peek(), Token::LeftBrace} };

  ProgramParser pParser{ context(), midfragment() };
  auto ret = std::dynamic_pointer_cast<ast::CompoundStatement>(pParser.parseStatement());
  seek(pParser.iterator());
  return ret;
}

bool DeclParser::detectCtorDecl()
{
  if (!mExplicitKw.isValid())
    return false;

  auto p = iterator();
  IdentifierParser ip{ context(), midfragment() };
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

  if (peek() != Token::LeftPar) {
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
    throw SyntaxError{ ParserError::UnexpectedEndOfInput };

  IdentifierParser ip{ context(), midfragment(), IdentifierParser::ParseSimpleId | IdentifierParser::ParseTemplateId };
  auto iden = parse_and_seek(ip);

  if (!isClassName(iden))
    throw SyntaxError{ ParserError::ExpectedCurrentClassName };

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
  TypeParser tp{ context(), midfragment() };
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
      throw SyntaxError{ ParserError::CouldNotReadType };
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



EnumValueParser::EnumValueParser(std::shared_ptr<ParserContext> shared_context, const Fragment& fragment)
  : ParserBase(shared_context, fragment)
{

}

void EnumValueParser::parse()
{
  while (!atEnd())
  {
    Fragment frag{ midfragment(), Fragment::Type<Fragment::ListElement>() };

    if (iterator() == frag.end())
    {
      if (!atEnd())
        read(Token::Comma);
      continue;
    }

    IdentifierParser idparser{ context(), frag, IdentifierParser::ParseOnlySimpleId };
    /// TODO: add overlaod that returns ast::SimpleIdentifier
    auto name = parse_and_seek(idparser);
    if (iterator() == frag.end())
    {
      values.push_back(ast::EnumValueDeclaration{ std::static_pointer_cast<ast::SimpleIdentifier>(name), nullptr });
      if (!atEnd())
        read(Token::Comma);
      continue;
    }

    const Token equalsign = read(Token::Eq);

    ExpressionParser valparser{ context(), frag.mid(iterator()) };
    auto expr = parse_and_seek(valparser);

    values.push_back(ast::EnumValueDeclaration{ std::static_pointer_cast<ast::SimpleIdentifier>(name), expr });
    if (!atEnd())
      read(Token::Comma);
  }
}


EnumParser::EnumParser(std::shared_ptr<ParserContext> shared_context, const Fragment& fragment)
  : ParserBase(shared_context, fragment)
{

}

std::shared_ptr<ast::EnumDeclaration> EnumParser::parse()
{
  const Token & etok = read();

  Token ctok;
  if (peek() == Token::Class)
    ctok = read();

  std::shared_ptr<ast::SimpleIdentifier> enum_name;
  {
    IdentifierParser idparser{ context(), midfragment(), IdentifierParser::ParseOnlySimpleId };
    /// TODO: add overload to avoid this cast
    enum_name = std::static_pointer_cast<ast::SimpleIdentifier>(parse_and_seek(idparser));
  }

  Fragment sentinel{ midfragment(), Fragment::Type<Fragment::DelimiterPair>() };

  read(Token::LeftBrace);

  EnumValueParser value_parser{ context(), sentinel };
  value_parser.parse();
  seek(value_parser.iterator());

  read(Token::RightBrace);
  read(Token::Semicolon);

  return ast::EnumDeclaration::New(etok, ctok, enum_name, std::move(value_parser.values));
}



ClassParser::ClassParser(std::shared_ptr<ParserContext> shared_context, const Fragment& fragment)
  : ParserBase(shared_context, fragment)
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
  auto name = readClassName();

  mClass = ast::ClassDecl::New(classKeyword, name);

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
  FriendParser fdp{ context(), midfragment() };
  auto friend_decl = parse_and_seek(fdp);
  mClass->content.push_back(friend_decl);
}

void ClassParser::parseTemplate()
{
  TemplateParser tp{ context(), midfragment() };
  auto template_decl = parse_and_seek(tp);
  mClass->content.push_back(template_decl);
}

void ClassParser::parseUsing()
{
  UsingParser np{ context(), midfragment() };
  auto decl = parse_and_seek(np);
  mClass->content.push_back(decl);
}

std::shared_ptr<ast::Identifier> ClassParser::readClassName()
{
  auto opts = mTemplateSpecialization ? IdentifierParser::ParseTemplateId : 0;
  IdentifierParser nameParser{ context(), midfragment(), opts | IdentifierParser::ParseSimpleId };
  return parse_and_seek(nameParser);
}

void ClassParser::readOptionalParent()
{
  if (atEnd())
    throw SyntaxError{ ParserError::UnexpectedEndOfInput };

  if (peek() != Token::Colon)
    return;

  mClass->colon = unsafe_read();

  if (atEnd())
    throw SyntaxError{ ParserError::UnexpectedEndOfInput };

  IdentifierParser nameParser{ context(), midfragment(), IdentifierParser::ParseTemplateId | IdentifierParser::ParseQualifiedId}; // TODO : forbid read operator name directly here
  auto parent = parse_and_seek(nameParser);
  //if(parent->is<ast::OperatorName>())
  //  throw Error{ "Unexpected operator name read after ':' " };

  mClass->parent = parent;
}

void ClassParser::readDecl()
{
  if (atEnd())
    throw SyntaxError{ ParserError::UnexpectedEndOfInput };

  DeclParser dp{ context(), midfragment(), mClass->name };
  
  if (!dp.detectDecl())
    throw SyntaxError{ ParserError::ExpectedDeclaration };

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

NamespaceParser::NamespaceParser(std::shared_ptr<ParserContext> shared_context, const Fragment& fragment)
  : ParserBase(shared_context, fragment)
{

}

std::shared_ptr<ast::Declaration> NamespaceParser::parse()
{
  const Token ns_tok = unsafe_read();

  auto name = readNamespaceName();

  if (peek() == Token::Eq)
  {
    const Token eq_sign = unsafe_read();

    IdentifierParser idp{ context(), midfragment() };
    std::shared_ptr<ast::Identifier> aliased_name = parse_and_seek(idp);

    read(Token::Semicolon);

    return ast::NamespaceAliasDefinition::New(ns_tok, name, eq_sign, aliased_name);
  }

  Fragment sentinel{ midfragment(), Fragment::Type<Fragment::DelimiterPair>() };

  const Token lb = read(Token::LeftBrace);

  Parser parser;
  parser.reset(context(), sentinel);

  auto statements = parser.parseProgram();
  seek(parser.iterator());

  const Token rb = read(Token::RightBrace);

  return ast::NamespaceDeclaration::New(ns_tok, name, lb, std::move(statements), rb);
}

std::shared_ptr<ast::SimpleIdentifier> NamespaceParser::readNamespaceName()
{
  IdentifierParser idp{ context(), midfragment(), IdentifierParser::ParseOnlySimpleId };
  return std::static_pointer_cast<ast::SimpleIdentifier>(parse_and_seek(idp));
}



FriendParser::FriendParser(std::shared_ptr<ParserContext> shared_context, const Fragment& fragment)
  : ParserBase(shared_context, fragment)
{

}

std::shared_ptr<ast::FriendDeclaration> FriendParser::parse()
{
  const Token friend_tok = unsafe_read();

  const Token class_tok = read(Token::Class);

  std::shared_ptr<ast::Identifier> class_name;
  {
    IdentifierParser idp{ context(), midfragment() };
    class_name = parse_and_seek(idp);
  }

  const Token semicolon = read(Token::Semicolon);

  return ast::ClassFriendDeclaration::New(friend_tok, class_tok, class_name);
}



UsingParser::UsingParser(std::shared_ptr<ParserContext> shared_context, const Fragment& fragment)
  : ParserBase(shared_context, fragment)
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
  IdentifierParser idp{ context(), midfragment() };
  return parse_and_seek(idp);
}



ImportParser::ImportParser(std::shared_ptr<ParserContext> shared_context, const Fragment& fragment)
  : ParserBase(shared_context, fragment) { }

std::shared_ptr<ast::ImportDirective> ImportParser::parse()
{
  const Token exprt = unsafe_peek() == Token::Export ? unsafe_read() : Token{};

  const Token imprt = read(Token::Import);

  std::vector<Token> names;

  Token tok = read();

  if (!tok.isIdentifier())
    throw SyntaxError{ ParserError::ExpectedIdentifier, errors::ActualToken{tok} };

  names.push_back(tok);

  while (peek() == Token::Dot)
  {
    unsafe_read();

    tok = read();

    if (!tok.isIdentifier())
      throw SyntaxError{ ParserError::ExpectedIdentifier, errors::ActualToken{tok} };

    names.push_back(tok);
  }

  read(Token::Semicolon);

  return ast::ImportDirective::New(exprt, imprt, std::move(names));
}



TemplateParser::TemplateParser(std::shared_ptr<ParserContext> shared_context, const Fragment& fragment)
  : ParserBase(shared_context, fragment)
{

}

std::shared_ptr<ast::TemplateDeclaration> TemplateParser::parse()
{
  const Token tmplt_k = unsafe_read();

  Fragment::iterator current_frag_end = context()->half_consumed_right_right_angle && fragment().end()->id == Token::RightRightAngle ?
    fragment().end() + 1 : fragment().end();
  Fragment::iterator frag_begin;
  Fragment::iterator frag_end;
  bool half_consumed_right_right = false;

  bool ok = Fragment::tryBuildTemplateFragment(iterator(), current_frag_end,
    frag_begin, frag_end, half_consumed_right_right);

  if (!ok)
    throw SyntaxError{ ParserError::UnexpectedFragmentEnd };

  RaiiRightRightAngleGuard guard{ context().get() };
  if (guard.value && half_consumed_right_right)
    context()->half_consumed_right_right_angle = false;
  else if (half_consumed_right_right)
    context()->half_consumed_right_right_angle = true;

  Fragment sentinel{ frag_begin, frag_end };

  const Token left_angle = read(Token::LeftAngle);

  std::vector<ast::TemplateParameter> params;

  while(iterator() != sentinel.end())
  {
    Fragment frag{ sentinel.mid(iterator()), Fragment::Type<Fragment::ListElement>() };
    TemplateParameterParser param_parser{ context(), frag };
    params.push_back(parse_and_seek(param_parser));

    if (iterator() != sentinel.end())
      read(Token::Comma);
  }

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
    ClassParser parser{ context(), midfragment() };
    parser.setTemplateSpecialization(true);
    return parse_and_seek(parser);
  }

  DeclParser funcparser{ context(), midfragment() };
  funcparser.setDeclaratorOptions(IdentifierParser::ParseSimpleId | IdentifierParser::ParseOperatorName | IdentifierParser::ParseTemplateId);

  if (!funcparser.detectDecl())
    throw SyntaxError{ ParserError::ExpectedDeclaration };

  funcparser.setDecision(DeclParser::ParsingFunction);
  return parse_and_seek(funcparser);
}



TemplateParameterParser::TemplateParameterParser(std::shared_ptr<ParserContext> shared_context, const Fragment& fragment)
  : ParserBase(shared_context, fragment)
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
    throw SyntaxError{ ParserError::UnexpectedToken, errors::UnexpectedToken{unsafe_peek(), Token::Invalid} };

  if (!peek().isIdentifier())
    throw SyntaxError{ ParserError::ExpectedIdentifier, errors::ActualToken{unsafe_peek()} };

  result.name = unsafe_read();

  if (atEnd())
    return result;

  result.eq = read(Token::Eq);

  TemplateArgParser argp{ context(), midfragment() };
  result.default_value = parse_and_seek(argp);

  return result;
}



Parser::Parser()
  : ProgramParser(std::make_shared<ParserContext>(std::vector<Token>()))
{

}

Parser::Parser(const SourceFile & source)
  : ProgramParser(std::make_shared<ParserContext>(source))
{
  m_context->mAst = std::make_shared<ast::AST>(source);
}

std::shared_ptr<ast::AST> Parser::parse(const SourceFile & source)
{
  auto c = std::make_shared<ParserContext>(source);
  reset(c, Fragment{ *c });

  std::shared_ptr<ast::AST> ret = std::make_shared<ast::AST>(source);
  ret->root = ast::ScriptRootNode::New(ret);
  c->mAst = ret;

  try
  {
    while (!atEnd())
    {
      ret->add(parseStatement());
    }
  }
  catch (SyntaxError& ex)
  {
    ex.location = location();
    throw;
  }

  return ret;
}

std::shared_ptr<ast::AST> Parser::parseExpression(const SourceFile & source)
{
  auto c = std::make_shared<ParserContext>(source);
  reset(c, Fragment{ *c });

  std::shared_ptr<ast::AST> ret = std::make_shared<ast::AST>(source);
  c->mAst = ret;

  try
  {
    ExpressionParser ep{ context(), fragment() };
    auto expr = parse_and_seek(ep);
    ret->root = expr;
  }
  catch (SyntaxError & ex)
  {
    ex.location = location();
    throw;
  }

  return ret;
}


std::shared_ptr<ast::ClassDecl> Parser::parseClassDeclaration()
{
  ClassParser cp{ context(), midfragment() };
  return parse_and_seek(cp);
}

} // parser

} // namespace script

