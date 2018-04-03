// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/parser/parser.h"

#include <map>

#include "script/operator.h"

#include "script/parser/parsererrors.h"

namespace script
{

namespace parser
{



AbstractFragment::AbstractFragment(const std::shared_ptr<ParserData> & pdata)
  : mData(pdata)
  , mBegin(mData->pos())
  , mParent(nullptr)
{

}

AbstractFragment::AbstractFragment(AbstractFragment *parent)
  : mData(parent->data())
  , mBegin(mData->pos())
  , mParent(parent)
{

}

AbstractFragment::~AbstractFragment()
{
  mParent = nullptr;
  mData = nullptr;
}

Token AbstractFragment::read()
{
  return data()->read();
}

Token AbstractFragment::peek() const
{
  if (atEnd())
    throw UnexpectedFragmentEnd{};
  return mData->unsafe_peek();
}

void AbstractFragment::seekBegin()
{
  return data()->seek(mBegin);
}

AbstractFragment * AbstractFragment::parent() const
{
  return mParent;
}

std::shared_ptr<ParserData> AbstractFragment::data() const
{
  return mData;
}

ScriptFragment::ScriptFragment(const std::shared_ptr<ParserData> & pdata)
  : AbstractFragment(pdata)
{

}

bool ScriptFragment::atEnd() const
{
  return data()->atEnd();
}

SentinelFragment::SentinelFragment(const Token::Type & s, AbstractFragment *parent)
  : AbstractFragment(parent)
  , mSentinel(s)
{

}

SentinelFragment::~SentinelFragment()
{

}

StatementFragment::StatementFragment(AbstractFragment *parent)
  : SentinelFragment(Token::Semicolon, parent)
{

}

StatementFragment::~StatementFragment()
{

}

CompoundStatementFragment::CompoundStatementFragment(AbstractFragment *parent)
  : SentinelFragment(Token::RightBrace, parent)
{

}
CompoundStatementFragment::~CompoundStatementFragment()
{}

bool SentinelFragment::atEnd() const
{
  return data()->atEnd() || data()->unsafe_peek().type == mSentinel;
}

Token SentinelFragment::consumeSentinel()
{
  return data()->read();
}

TemplateArgumentListFragment::TemplateArgumentListFragment(AbstractFragment *parent)
  : AbstractFragment(parent)
  , right_shift_flag(false)
{

}

TemplateArgumentListFragment::~TemplateArgumentListFragment()
{

}


bool TemplateArgumentListFragment::atEnd() const
{
  if (parent()->atEnd() && dynamic_cast<TemplateArgumentFragment*>(parent()) == nullptr)
    throw std::runtime_error{ "Not a template argument list" };

  const Token tok = data()->peek();
  return tok.type == Token::RightAngle || tok.type == Token::RightRightAngle;
}

void TemplateArgumentListFragment::consumeEnd()
{
  assert(atEnd());

  const Token tok = data()->peek();
  if (tok.type == Token::RightRightAngle)
  {
    if (this->right_shift_flag)
    {
      this->right_angle = data()->unsafe_read();
      this->right_angle.type = Token::RightAngle;
      this->right_angle.length = 1;
      this->right_angle.pos += 1;
    }
    else
    {
      TemplateArgumentListFragment *p = dynamic_cast<TemplateArgumentListFragment*>(parent()->parent());
      if(p == nullptr)
        throw std::runtime_error{ "Not implemented" };
      p->right_shift_flag = true;
      this->right_angle = tok;
      this->right_angle.type = Token::RightAngle;
      this->right_angle.length = 1;
    }
  }
  else
    this->right_angle = data()->unsafe_read();
}

TemplateArgumentFragment::TemplateArgumentFragment(AbstractFragment *parent)
  : AbstractFragment(parent)
{
  if (data()->peek().type == Token::Comma)
    throw std::runtime_error{ "TemplateArgumentFragment constructor : Implementation error" };
}

TemplateArgumentFragment::~TemplateArgumentFragment()
{

}


bool TemplateArgumentFragment::atEnd() const
{
  return parent()->atEnd() || data()->peek().type == Token::Comma;
}

void TemplateArgumentFragment::consumeComma()
{
  assert(atEnd());
  if (!parent()->atEnd())
    read();
}



FunctionArgFragment::FunctionArgFragment(AbstractFragment *parent)
  : AbstractFragment(parent)
{
}

FunctionArgFragment::~FunctionArgFragment()
{

}

bool FunctionArgFragment::atEnd() const
{
  if (data()->atEnd())
    return true;
  Token tok = data()->peek();
  return tok.type == Token::Comma || tok.type == Token::RightPar;
}

ListFragment::ListFragment(AbstractFragment *parent)
  : AbstractFragment(parent)
{

}

ListFragment::~ListFragment()
{

}

bool ListFragment::atEnd() const
{
  return parent()->atEnd() || data()->unsafe_peek() == Token::Comma;
}

void ListFragment::consumeComma()
{
  assert(atEnd());
  if (!parent()->atEnd())
    read();
}

SubFragment::SubFragment(AbstractFragment *parent)
  : AbstractFragment(parent)
{

}

bool SubFragment::atEnd() const
{
  return parent()->atEnd();
}

ParserData::ParserData(const SourceFile & src)
  : mSource(src)
  , mIndex(0)
{
  mLexer.setSource(mSource);
  fetchNext();
}

ParserData::ParserData(const std::vector<Token> & tokens)
  : mBuffer(tokens)
{

}

ParserData::ParserData(std::vector<Token> && tokens)
  : mBuffer(std::move(tokens))
{

}

ParserData::~ParserData()
{
  mSource = SourceFile{};
  mLexer.reset();
  mBuffer.clear();
  mIndex = 0;
}

bool ParserData::atEnd() const
{
  if (mIndex < mBuffer.size())
    return false;

  return true;
}

Token ParserData::read()
{
  if (mIndex == mBuffer.size())
    throw UnexpectedEndOfInput{};

  const Token ret = mBuffer[mIndex++];
  fetchNext();
  return ret;
}

Token ParserData::unsafe_read()
{
  assert(mIndex < mBuffer.size());
  fetchNext();
  return mBuffer[mIndex++];
}

void ParserData::unread()
{
  if (mIndex == 0)
    throw std::runtime_error{ "Cannot unread" };

  mIndex--;
}

Token ParserData::peek()
{
  if (atEnd())
    throw UnexpectedEndOfInput{};
  return mBuffer[mIndex];
}

std::string ParserData::text(const Token & tok) const
{
  return mLexer.text(tok);
}

ParserData::Position ParserData::pos() const
{
  if (mIndex < mBuffer.size())
    return Position{ mIndex, mBuffer[mIndex] };
  return Position{ mIndex, Token{} };
}

void ParserData::seek(const Position & p)
{
  // TODO : use p.token to ensure pos is correct
  mIndex = p.index;
}

SourceFile::Position ParserData::sourcepos() const
{
  if (mIndex < mBuffer.size())
    return SourceFile::Position{ mBuffer[mIndex].pos, mBuffer[mIndex].line, mBuffer[mIndex].column };
  return SourceFile::Position{ (SourceFile::Offset)-1, (uint16) -1, (uint16) -1 };
}

void ParserData::clearBuffer()
{
  if (mIndex == mBuffer.size())
  {
    mBuffer.clear();
    mIndex = 0;
  }
  else
  {
    for (size_t i(mIndex); i < mBuffer.size(); ++i)
      mBuffer[i - mIndex] = mBuffer[i];
    mBuffer.resize(mBuffer.size() - mIndex);
    mIndex = 0;
  }
}


void ParserData::fetchNext()
{
  while (!mLexer.atEnd())
  {
    const Token t = mLexer.read();
    if (isDiscardable(t))
      continue;
    mBuffer.push_back(t);
    return;
  }
}

bool ParserData::isDiscardable(const Token & t) const
{
  return t == Token::MultiLineComment || t == Token::SingleLineComment;
}


ParserBase::ParserBase(AbstractFragment *frag)
  : mFragment(frag)
{

}

ParserBase::~ParserBase()
{
  mFragment = nullptr;
}

void ParserBase::reset(AbstractFragment *fragment)
{
  mFragment = fragment;
}

std::shared_ptr<ast::AST> ParserBase::ast() const
{
  return mFragment->data()->mAst;
}

bool ParserBase::atEnd() const
{
  return mFragment->atEnd();
}

bool ParserBase::eof() const
{
  auto p = mFragment->data();
  return p->atEnd();
}

Token ParserBase::read()
{
  return mFragment->read();
}

Token ParserBase::unsafe_read()
{
  return mFragment->data()->unsafe_read();
}

Token ParserBase::read(const Token::Type & type)
{
  Token ret = read();
  if (ret.type != type)
  {
    switch (type)
    {
    case Token::LeftBrace:
      throw ExpectedLeftBrace{};
    case Token::LeftPar:
      throw ExpectedLeftPar{};
    default:
      throw UnexpectedToken{};
    }
  }
  return ret;
}

Token ParserBase::peek() const
{
  return mFragment->peek();
}

Token ParserBase::unsafe_peek() const
{
  return mFragment->data()->unsafe_peek();
}

AbstractFragment * ParserBase::fragment() const
{
  return mFragment;
}

ParserData::Position ParserBase::pos() const
{
  return mFragment->data()->pos();
}

void ParserBase::seek(const ParserData::Position & p)
{
  return mFragment->data()->seek(p);
}

const std::string ParserBase::text(const Token & tok)
{
  return mFragment->data()->text(tok);
}

SourceFile::Position ParserBase::sourcepos() const
{
  return mFragment->data()->sourcepos();
}

LiteralParser::LiteralParser(AbstractFragment *fragment)
  : ParserBase(fragment)
{

}

std::shared_ptr<ast::Literal> LiteralParser::parse()
{
  Token lit = read();
  assert(lit.type & Token::Literal);
  switch (lit.type)
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

  throw CouldNotReadLiteral{};
}

ExpressionParser::ExpressionParser(AbstractFragment *fragment)
  : ParserBase(fragment)
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


bool ExpressionParser::isPrefixOperator(const Token & tok) const
{
  auto op = ast::OperatorName::getOperatorId(tok, ast::OperatorName::PrefixOp);
  return op != Operator::Null;
}

bool ExpressionParser::isPostfixOperator(const Token & tok) const
{
  auto op = ast::OperatorName::getOperatorId(tok, ast::OperatorName::PostFixOp);
  return op != Operator::Null;
}
bool ExpressionParser::isInfixOperator(const Token & tok) const
{
  auto op = ast::OperatorName::getOperatorId(tok, ast::OperatorName::InfixOp);
  return op != Operator::Null;
}

std::shared_ptr<ast::Expression> ExpressionParser::readOperand()
{
  assert(!atEnd());
  auto p = pos();
  Token t = peek();

  std::shared_ptr<ast::Expression> operand = nullptr;

  if (t.isOperator())
  {
    if (!isPrefixOperator(t))
      throw NotPrefixOperator{};

    assert(isPrefixOperator(t));
    read();
    operand = readOperand();
    operand = ast::Operation::New(t, operand);
  }
  else if (t.type == Token::LeftPar) 
  {
    unsafe_read();
    if (peek().type == Token::RightPar) // we just read '()'
      throw InvalidEmptyOperand{};
    SentinelFragment sentinel{ Token::RightPar, fragment() };
    ExpressionParser exprParser{ &sentinel };
    operand = exprParser.parse();
    sentinel.consumeSentinel();
  }
  else if (t.type == Token::LeftBracket) // array
  {
    LambdaParser lambda_parser{ fragment() };
    operand = lambda_parser.parse();
  }
  else if (t.type == Token::LeftBrace)
  {
    const Token left_brace = unsafe_read();
    auto list = ast::ListExpression::New(left_brace);
    SentinelFragment sentinel{ Token::RightBrace, fragment() };
    ListFragment listfrag{ &sentinel };
    while (!sentinel.atEnd())
    {
      ExpressionParser exprparser{ &listfrag };
      list->elements.push_back(exprparser.parse());
      listfrag.consumeComma();
    }

    list->right_brace = sentinel.consumeSentinel();

    operand = list;
  }
  else if (t.isLiteral())
  {
    LiteralParser p{ fragment() };
    operand = p.parse();
  }
  else
  {
    IdentifierParser idParser{ fragment() };
    operand = idParser.parse();

    /// TODO : handle static_cast and other built-int constructions here..
  }

  while (!atEnd())
  {
    t = peek();
    if (t.type == Token::PlusPlus || t.type == Token::MinusMinus)
    {
      operand = ast::Operation::New(t, operand, nullptr);
      read();
    }
    else if (t.type == Token::Dot)
    {
      unsafe_read();
      IdentifierParser idParser{ fragment(), IdentifierParser::ParseSimpleId | IdentifierParser::ParseTemplateId };
      auto memberName = idParser.parse();
      operand = ast::Operation::New(t, operand, memberName);
    }
    else if (t.type == Token::LeftPar)
    {
      auto leftPar = unsafe_read();
      Token next = peek();
      std::vector<std::shared_ptr<ast::Expression>> args;
      while (next != Token::RightPar)
      {
        FunctionArgFragment fargFrag{ fragment() };
        ExpressionParser exprParser{ &fargFrag };
        args.push_back(exprParser.parse());
        next = peek();
        if (next.type == Token::Comma)
          read();
      }

      operand = ast::FunctionCall::New(operand, leftPar, std::move(args), next);
      read(); // reads the rightPar
    }
    else if (t.type == Token::LeftBracket) // subscript operator
    {
      auto leftBracket = read();
      if (atEnd())
        throw UnexpectedEndOfInput{};
      Token next = peek();
      if (next == Token::RightBracket)
        throw InvalidEmptyBrackets{};
      SentinelFragment sentinel{ Token::RightBracket, fragment() };
      ExpressionParser exprParser{ &sentinel };
      auto arg = exprParser.parse();
      operand = ast::ArraySubscript::New(operand, leftBracket, arg, read());
    }
    else if (t.type == Token::LeftBrace && operand->is<ast::Identifier>())
    {
      auto type_name = std::dynamic_pointer_cast<ast::Identifier>(operand);
      const Token & left_brace = unsafe_read();
      
      SentinelFragment sentinel{ Token::RightBrace, fragment() };
      ExpressionListParser argsParser{ &sentinel };
      auto args = argsParser.parse();
      const Token & right_brace = sentinel.consumeSentinel();
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
        seek(p);
        IdentifierParser idParser{ fragment() };
        idParser.setOptions(IdentifierParser::ParseOperatorName | IdentifierParser::ParseQualifiedId);
        operand = idParser.parse();
        continue;
      }

      throw UnexpectedToken{};
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
    throw CouldNotReadOperator{};
  else if (!isInfixOperator(t))
    throw ExpectedBinaryOperator{};

  return read();
}

bool ExpressionParser::tryReadBinaryOperator(Token & result)
{
  const Token t = peek();

  if (t == Token::QuestionMark || t == Token::Colon)
  {
    result = read();
    return true;
  }

  if (!t.isOperator() || !isInfixOperator(t))
    return false;

  result = read();
  return true;
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
  std::vector<Token>::const_iterator opEnd, int opIndex)
{
  auto left = buildExpression(exprBegin, exprBegin + (opIndex + 1), opBegin, opBegin + opIndex);
  auto right = buildExpression(exprBegin + (opIndex + 1), exprEnd, opBegin + (opIndex + 1), opEnd);
  return ast::Operation::New(*(opBegin + opIndex), left, right);
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
    if (tok.type == Token::Colon)
      return -66;
    else if (tok.type == Token::QuestionMark)
      return Operator::precedence(Operator::ConditionalOperator);
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
      throw MissingConditionalColon{};

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


LambdaParser::LambdaParser(AbstractFragment *fragment)
  : ParserBase(fragment)
  , mDecision(Undecided)
{

}

std::shared_ptr<ast::Expression> LambdaParser::parse()
{
  const Token lb = read();
  mArray = ast::ArrayExpression::New(lb);
  mLambda = ast::LambdaExpression::New(ast(), lb);

  readBracketContent();

  if (atEnd()) {
    if (mDecision == ParsingLambda)
      throw UnexpectedFragmentEnd{};
    else {
      setDecision(ParsingArray);
      return mArray;
    }
  }

  if (peek() != Token::LeftPar) {
    if (mDecision == ParsingLambda)
      throw ExpectedLeftPar{};
    else {
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
  SentinelFragment sentinel{ Token::RightBracket, fragment() };
  while (!sentinel.atEnd())
  {
    ListFragment listfrag{ &sentinel };

    if(mDecision == Undecided || mDecision == ParsingArray)
    {
      ExpressionParser ep{ &listfrag };
      try
      {
        auto elem = ep.parse();
        mArray->elements.push_back(elem);
      }
      catch (const ParserException &)
      {
        if (mDecision == ParsingArray)
          throw;
        mDecision = ParsingLambda;
        mArray = nullptr;
      }
    }


    if (mDecision == Undecided || mDecision == ParsingLambda)
    {
      auto savedPos = pos();
      listfrag.seekBegin();

      LambdaCaptureParser capp{ &listfrag };

      if (!capp.detect())
      {
        if (mDecision == ParsingLambda)
          throw CouldNotParseLambdaCapture{};
        setDecision(ParsingArray);
        seek(savedPos);
        listfrag.consumeComma();
        continue;
      }

      try
      {
        auto capture = capp.parse();
        mLambda->captures.push_back(capture);
      }
      catch (const ParserException &)
      {
        if (mDecision == ParsingLambda)
          throw;
        setDecision(ParsingArray);
      }

      if (!listfrag.atEnd())
        seek(savedPos);
    }

    listfrag.consumeComma();
  }

  const Token rb = sentinel.consumeSentinel();

  if (mArray)
    mArray->rightBracket = rb;
  if (mLambda)
    mLambda->rightBracket = rb;
}

void LambdaParser::readParams()
{
  assert(mDecision == ParsingLambda);

  mLambda->leftPar = readToken<ExpectedLeftPar>(Token::LeftPar);

  SentinelFragment sentinel{ Token::RightPar, fragment() };
  while (!sentinel.atEnd())
  {
    ListFragment listfrag{ &sentinel };

    FunctionParamParser pp{ &listfrag };
    auto param = pp.parse();
    mLambda->params.push_back(param);

    listfrag.consumeComma();
  }

  mLambda->rightPar = sentinel.consumeSentinel();
}


std::shared_ptr<ast::CompoundStatement> LambdaParser::readBody()
{
  if (atEnd())
    throw UnexpectedEndOfInput{};

  if (peek() != Token::LeftBrace)
    throw ExpectedLeftBrace{};

  ProgramParser pParser{ fragment() };
  return std::dynamic_pointer_cast<ast::CompoundStatement>(pParser.parseStatement());
}


LambdaCaptureParser::LambdaCaptureParser(AbstractFragment *fragment)
  : ParserBase(fragment)
{

}

bool LambdaCaptureParser::detect() const
{
  if (peek() == Token::Eq || unsafe_peek() == Token::Ref)
    return true;
  return unsafe_peek().type == Token::UserDefinedName;
}

ast::LambdaCapture LambdaCaptureParser::parse()
{
  if (atEnd())
    throw UnexpectedFragmentEnd{};

  ast::LambdaCapture cap;

  if (peek() == Token::Eq) {
    cap.byValueSign = read();
    if (!atEnd())
      throw UnexpectedToken{};
    return cap;
  }
  else if (peek() == Token::Ref) {
    cap.reference = read();
    if (atEnd())
      return cap;
  }

  IdentifierParser idpar{ fragment(), IdentifierParser::ParseOnlySimpleId };
  cap.name = idpar.parse()->name;
  if (atEnd())
    return cap;
  cap.assignmentSign = readToken<ExpectedEqualSign>(Token::Eq);
  ExpressionParser ep{ fragment() };
  cap.value = ep.parse();
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


ProgramParser::ProgramParser(AbstractFragment *frag)
  : ParserBase(frag)
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
  switch (t.type)
  {
  case Token::Semicolon:
    return ast::NullStatement::New(read());
  case Token::Break:
    return parseBreakStatement();
  case Token::Class:
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
    throw NotImplementedError{ "Using declarations are not implemented" };
  case Token::While:
    return parseWhileLoop();
  case Token::For:
    return parseForLoop();
  case Token::LeftBrace:
    return parseCompoundStatement();
  default:
    break;
  }

  return parseAmbiguous();
}

std::shared_ptr<ast::Statement> ProgramParser::parseAmbiguous()
{
  auto savePoint = pos();
  DeclParser dp{ fragment() };
  if (dp.detectDecl())
    return dp.parse();
  
  seek(savePoint);

  SentinelFragment sentinel{ Token::Semicolon, fragment() };
  ExpressionParser ep{ &sentinel };
  auto expr = ep.parse();
  auto semicolon = sentinel.consumeSentinel();
  return ast::ExpressionStatement::New(expr, semicolon);
}

std::shared_ptr<ast::ClassDecl> ProgramParser::parseClassDeclaration()
{
  throw UnexpectedClassKeyword{};
}

std::shared_ptr<ast::EnumDeclaration> ProgramParser::parseEnumDeclaration()
{
  /// TODO : shouldn't we also throw here but not in Parser (as in parseClassDeclaration()) 
  EnumParser ep{ fragment() };
  return ep.parse();
}

std::shared_ptr<ast::BreakStatement> ProgramParser::parseBreakStatement()
{
  Token kw = read();
  assert(kw == Token::Break);
  const Token semicolon = readToken<ExpectedSemicolon>(Token::Semicolon);
  return ast::BreakStatement::New(kw);
}

std::shared_ptr<ast::ContinueStatement> ProgramParser::parseContinueStatement()
{
  Token kw = read();
  assert(kw == Token::Continue);
  const Token semicolon = readToken<ExpectedSemicolon>(Token::Semicolon);
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

  StatementFragment exprStatement{ fragment() };
  ExpressionParser exprParser{ &exprStatement };
  auto returnValue = exprParser.parse();

  Token semicolon = read();
  assert(semicolon == Token::Semicolon);

  return ast::ReturnStatement::New(kw, returnValue);
}

std::shared_ptr<ast::CompoundStatement> ProgramParser::parseCompoundStatement()
{
  Token leftBrace = read();
  assert(leftBrace == Token::LeftBrace);

  CompoundStatementFragment frag{ fragment() };
  ProgramParser progParser{ &frag };

  std::vector<std::shared_ptr<ast::Statement>> statements = progParser.parseProgram();
  Token rightBrace = frag.consumeSentinel();
  assert(rightBrace == Token::RightBrace);

  auto ret = ast::CompoundStatement::New(leftBrace, rightBrace);
  ret->statements = std::move(statements);
  return ret;
}

std::shared_ptr<ast::IfStatement> ProgramParser::parseIfStatement()
{
  Token ifkw = read();
  assert(ifkw == Token::If);
  const Token leftpar = readToken<ExpectedLeftPar>(Token::LeftPar);

  auto ifStatement = ast::IfStatement::New(ifkw);

  {
    SentinelFragment condition{ Token::RightPar, fragment() };
    ExpressionParser exprParser{ &condition };
    ifStatement->condition = exprParser.parse();
    condition.consumeSentinel();
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
  const Token leftpar = readToken<ExpectedLeftPar>(Token::LeftPar);

  auto whileLoop = ast::WhileLoop::New(whilekw);

  {
    SentinelFragment condition{ Token::RightPar, fragment() };
    ExpressionParser exprParser{ &condition };
    whileLoop->condition = exprParser.parse();
    condition.consumeSentinel();
  }

  whileLoop->body = parseStatement();

  return whileLoop;
}

std::shared_ptr<ast::ForLoop> ProgramParser::parseForLoop()
{
  Token forkw = read();
  assert(forkw == Token::For);
  const Token leftpar = readToken<ExpectedLeftPar>(Token::LeftPar);

  auto forLoop = ast::ForLoop::New(forkw);

  {
    DeclParser initParser{ fragment() };
    if (!initParser.detectDecl())
    {
      SentinelFragment init{ Token::Semicolon, fragment() };
      ExpressionParser exprParser{ &init };
      auto initExpr = exprParser.parse();
      auto semicolon = init.consumeSentinel();
      forLoop->initStatement = ast::ExpressionStatement::New(initExpr, semicolon);
    }
    else
    {
      initParser.setDecision(DeclParser::ParsingVariable);
      forLoop->initStatement = initParser.parse();
    }
  }

  {
    SentinelFragment condition{ Token::Semicolon, fragment() };
    ExpressionParser exprParser{ &condition };
    forLoop->condition = exprParser.parse();
    condition.consumeSentinel();
  }

  {
    SentinelFragment loopIncr{ Token::RightPar, fragment() };
    ExpressionParser exprParser{ &loopIncr };
    forLoop->loopIncrement = exprParser.parse();
    loopIncr.consumeSentinel();
  }

  forLoop->body = parseStatement();

  return forLoop;
}


IdentifierParser::IdentifierParser(AbstractFragment *fragment, int opts)
  : ParserBase(fragment)
  , mOptions(opts)
{

}

std::shared_ptr<ast::Identifier> IdentifierParser::parse()
{
  Token t = peek();
  switch (t.type)
  {
  case Token::Void:
  case Token::Bool:
  case Token::Char:
  case Token::Int:
  case Token::Float:
  case Token::Double:
  case Token::Auto:
  case Token::This:
    return ast::Identifier::New(unsafe_read(), ast());
  case Token::Operator:
    return readOperatorName();
  case Token::UserDefinedName:
    return readUserDefinedName();
  default:
    break;
  }

  throw CouldNotReadIdentifier{};
}

std::shared_ptr<ast::Identifier> IdentifierParser::readOperatorName()
{
  if (!testOption(ParseOperatorName))
    throw UnexpectedOperatorKeyword{};

  Token opkw = read();
  if (atEnd())
    throw UnexpectedEndOfInput{};

  Token op = peek();
  if(op.type & Token::OperatorToken)
    return ast::OperatorName::New(opkw, read());
  else if (op == Token::LeftPar)
  {
    const Token lp = read();
    if (atEnd())
      throw UnexpectedEndOfInput{};
    if (peek() != Token::RightPar)
      throw ExpectedRightPar{};
    const Token rp = read();
    if (lp.column + 1 != rp.column)
      throw IllegalSpaceBetweenPars{};
    return ast::OperatorName::New(opkw, Token{ Token::LeftRightPar, lp.pos, 2, lp.line, lp.column, lp.src });
  }
  else if (op == Token::LeftBracket)
  {
    const Token lb = read();
    if (peek() != Token::RightBracket)
      throw ExpectedRightBracket{};
    const Token rb = read();
    if (lb.column + 1 != rb.column)
      throw IllegalSpaceBetweenBrackets{};
    return ast::OperatorName::New(opkw, Token{ Token::LeftRightBracket, lb.pos, 2, lb.line, lb.column, lb.src });
  }
  else if (op.type == Token::StringLiteral)
  {
    if (op.length != 2)
      throw ExpectedEmptyStringLiteral{};
    IdentifierParser idp{ fragment(), IdentifierParser::ParseOnlySimpleId };
    auto suffixName = idp.parse();
    return ast::LiteralOperatorName::New(opkw, op, suffixName->name, ast());
  }
  else if (op.type == Token::UserDefinedLiteral)
  {
    op = unsafe_read();
    const auto & str = text(op);
    if(str.find("\"\"") != 0)
      throw ExpectedEmptyStringLiteral{}; /// TODO ? should this have a different error than the previous
    Token quotes{ Token::StringLiteral, op.pos, 2, op.line, op.column, op.src };
    Token suffixName{ Token::UserDefinedName, op.pos + 2, op.length - 2, op.line, op.column + 2, op.src };
    return ast::LiteralOperatorName::New(opkw, quotes, suffixName, ast());
  }

  throw ExpectedOperatorSymbol{};
}

std::shared_ptr<ast::Identifier> IdentifierParser::readUserDefinedName()
{
  const Token base = read();
  if (base != Token::UserDefinedName)
    throw ExpectedUserDefinedName{};

  if(atEnd())
    return ast::Identifier::New(base, ast());

  std::shared_ptr<ast::Identifier> ret = ast::Identifier::New(base, ast());

  Token t = peek();
  if ((options() & ParseTemplateId) && t == Token::LeftAngle)
  {
    const auto savepoint = pos();
    try 
    {
      ret = readTemplateArguments(base);
    }
    catch (const ParserException & )
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
    identifiers.push_back(ast::Identifier::New(base, ast()));

    while (t == Token::ScopeResolution)
    {
      read();

      IdentifierParser idparser{ fragment(), IdentifierParser::ParseTemplateId };
      identifiers.push_back(idparser.parse());

      if (atEnd())
        break;
      else
        t = peek();
    }

    ret = ast::ScopedIdentifier::New(identifiers.begin(), identifiers.end());
  }

  return ret;
}

std::shared_ptr<ast::Identifier> IdentifierParser::readTemplateArguments(const Token & base)
{
  const Token leftangle = read();

  std::vector<ast::NodeRef> args;

  TemplateArgumentListFragment talistfragment{ fragment() };
  while (!talistfragment.atEnd())
  {
    TemplateArgumentFragment frag{ &talistfragment };
    TemplateArgParser argparser{ &frag };
    args.push_back(argparser.parse());
    frag.consumeComma();
  }

  talistfragment.consumeEnd();

  return ast::TemplateIdentifier::New(base, std::move(args), leftangle, talistfragment.right_angle, ast());
}

TemplateArgParser::TemplateArgParser(AbstractFragment *fragment)
  : ParserBase(fragment)
{

}

std::shared_ptr<ast::Node> TemplateArgParser::parse()
{
  auto p = pos();

  TypeParser tp{ fragment() };
  if (tp.detect())
  {
    try
    {
      auto type = tp.parse();
      if (atEnd())
        return ast::TypeNode::New(type);
    }
    catch (const ParserException &)
    {

    }
  }

  seek(p);

  ExpressionParser ep{ fragment() };
  return ep.parse();
}



TypeParser::TypeParser(AbstractFragment *fragment)
  : ParserBase(fragment)
  , mReadFunctionSignature(true)
{

}

ast::QualifiedType TypeParser::parse()
{
  ast::QualifiedType ret;

  if (peek() == Token::Const)
    ret.constQualifier = unsafe_read();

  IdentifierParser idparser{ fragment() };
  ret.type = idparser.parse();

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
    auto save_point = pos();
    try
    {
      auto fsig = tryReadFunctionSignature(ret);
      return fsig;
    }
    catch (const ParserException &)
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
  return peek().type & Token::Identifier;
}

ast::QualifiedType TypeParser::tryReadFunctionSignature(const ast::QualifiedType & rt)
{
  ast::QualifiedType ret;
  ret.functionType = std::make_shared<ast::FunctionType>();
  ret.functionType->returnType = rt;

  const Token leftPar = unsafe_read();
  SentinelFragment sentinel{ Token::RightPar, fragment() };
  while (!sentinel.atEnd())
  {
    ListFragment listfrag{ &sentinel };
    TypeParser tp{ &listfrag };
    auto param = tp.parse();
    ret.functionType->params.push_back(param);

    if (!listfrag.atEnd())
      throw UnexpectedToken{};

    listfrag.consumeComma();
  }

  sentinel.consumeSentinel();

  return ret;
}



FunctionParamParser::FunctionParamParser(AbstractFragment *fragment)
  : ParserBase(fragment)
{

}

ast::FunctionParameter FunctionParamParser::parse()
{
  ast::FunctionParameter fp;

  TypeParser tp{ fragment() };
  fp.type = tp.parse();

  if (atEnd())
    return fp;

  IdentifierParser ip{ fragment(), IdentifierParser::ParseOnlySimpleId };
  auto name = ip.parse();
  fp.name = name->name;

  if (atEnd())
    return fp;

  if (peek() != Token::Eq)
    throw ExpectedEqualSign{};

  const Token eqSign = read();
  ExpressionParser ep{ fragment() };
  auto defaultVal = ep.parse();
  fp.defaultValue = defaultVal;

  return fp;
}

ExpressionListParser::ExpressionListParser(AbstractFragment *fragment)
  : ParserBase(fragment)
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
    Fragment f{ fragment() };
    ExpressionParser expr{ &f };
    auto e = expr.parse();
    result.push_back(e);
    if (!atEnd())
      read(); // reads the comma
  }

  return result;
}

ExpressionListParser::Fragment::Fragment(AbstractFragment *parent)
  : AbstractFragment(parent)
{

}

ExpressionListParser::Fragment::~Fragment()
{

}


bool ExpressionListParser::Fragment::atEnd() const
{
  return parent()->atEnd() || data()->peek() == Token::Comma;
}




DeclParser::DeclParser(AbstractFragment *fragment, std::shared_ptr<ast::Identifier> cn)
  : ParserBase(fragment)
  , mDecision(Undecided)
  , mClassName(cn)
  , mParamsAlreadyRead(false)
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
      throw IllegalUseOfVirtual{};
  }

  readOptionalStatic();

  if (readOptionalExplicit())
  {
    if (!isParsingMember())
      throw IllegalUseOfExplicit{};
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
  TypeParser tp{ fragment() };
  try
  {
    mType = tp.parse();
  }
  catch (const ParserException & )
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
  IdentifierParser ip{ fragment() };
  ip.setOptions(IdentifierParser::ParseSimpleId | IdentifierParser::ParseOperatorName);

  try
  {
    mName = ip.parse();
  }
  catch (const ParserException &)
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
    auto overload = ast::OperatorOverloadDecl::New(ast(), mName);
    overload->returnType = mType;
    mFuncDecl = overload;
    return true;
  }
  else if (mName->is<ast::LiteralOperatorName>())
  {
    mDecision = ParsingFunction;
    auto lon = ast::OperatorOverloadDecl::New(ast(), mName);
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
  if (mDecision == NotADecl)
    throw ImplementationError{"Calling DeclParser::parse() when decision = NotADecl"};

  if (mDecision == ParsingDestructor)
    return parseDestructor();
  else if (mDecision == ParsingConstructor)
    return parseConstructor();
  else if (mDecision == ParsingCastDecl || mDecision == ParsingFunction)
    return parseFunctionDecl();
  else if (mDecision == ParsingVariable)
  {
    if (mVarDecl == nullptr) /// TODO : is this always true ?
      mVarDecl = ast::VariableDecl::New(mType, mName);
    mVarDecl->staticSpecifier = mStaticKw;
    return parseVarDecl();
  }

  assert(mDecision == Undecided);

  if (peek() == Token::LeftBrace || peek() == Token::Eq)
  {
    mDecision = ParsingVariable;

    mVarDecl = ast::VariableDecl::New(mType, mName);
    mVarDecl->staticSpecifier = mStaticKw;
    return parseVarDecl();
  }
  else if (peek() == Token::LeftPar)
  {
    mFuncDecl = ast::FunctionDecl::New(mName);
    mFuncDecl->returnType = mType;
    mFuncDecl->staticKeyword = mStaticKw;
    mFuncDecl->virtualKeyword = mVirtualKw;

    mVarDecl = ast::VariableDecl::New(mType, mName);
    mVarDecl->staticSpecifier = mStaticKw;
  }
  else
  {
    throw UnexpectedToken{};
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
      throw UnexpectedToken{};
    mDecision = ParsingFunction;
    mVarDecl = nullptr;
    mFuncDecl->body = readFunctionBody();
    return mFuncDecl;
  }
  else if (peek() == Token::Semicolon)
  {
    if (mDecision == ParsingFunction)
      throw ExpectedLeftBrace{};

    mVarDecl->semicolon = read();
    return mVarDecl;
  }

  throw UnexpectedToken{};
}

std::shared_ptr<ast::VariableDecl> DeclParser::parseVarDecl()
{
  if (peek() == Token::Eq)
  {
    const Token eqsign = read();
    StatementFragment exprFrag{ fragment() };
    ExpressionParser ep{ &exprFrag };
    auto expr = ep.parse();
    mVarDecl->init = ast::AssignmentInitialization::New(eqsign, expr);
  }
  else if (peek() == Token::LeftBrace)
  {
    const Token leftBrace = read();
    SentinelFragment sentinel{ Token::RightBrace, fragment() };
    ExpressionListParser argsParser{ &sentinel };
    auto args = argsParser.parse();
    const Token rightbrace = sentinel.consumeSentinel();
    mVarDecl->init = ast::BraceInitialization::New(leftBrace, std::move(args), rightbrace);
  }
  else if (peek() == Token::LeftPar)
  {
    const Token leftpar = read();
    SentinelFragment sentinel{ Token::RightPar, fragment() };
    ExpressionListParser argsParser{ &sentinel };
    auto args = argsParser.parse();
    const Token rightpar = sentinel.consumeSentinel();
    mVarDecl->init = ast::ConstructorInitialization::New(leftpar, std::move(args), rightpar);
  }
  else if (peek() != Token::Semicolon)
    throw ImplementationError{"DeclParser::parseVarDecl() : a semicolon was expected"};

  const Token semicolon = readToken<ExpectedSemicolon>(Token::Semicolon);

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
    IdentifierParser idp{ fragment(), IdentifierParser::ParseOnlySimpleId | IdentifierParser::ParseTemplateId };
    auto id = idp.parse();
    if (peek() == Token::LeftBrace)
    {
      const Token leftBrace = unsafe_read();
      SentinelFragment sentinel{ Token::RightBrace, fragment() };
      ExpressionListParser argsParser{ &sentinel };
      auto args = argsParser.parse();
      const Token rightBrace = sentinel.consumeSentinel();
      auto braceinit = ast::BraceInitialization::New(leftBrace, std::move(args), rightBrace);
      ctor->memberInitializationList.push_back(ast::MemberInitialization{ id, braceinit });
    }
    else if (peek() == Token::LeftPar)
    {
      const Token leftpar = unsafe_read();
      SentinelFragment sentinel{ Token::RightPar, fragment() };
      ExpressionListParser argsParser{ &sentinel };
      auto args = argsParser.parse();
      const Token rightpar = sentinel.consumeSentinel();
      auto ctorinit = ast::ConstructorInitialization::New(leftpar, std::move(args), rightpar);
      ctor->memberInitializationList.push_back(ast::MemberInitialization{ id, ctorinit });
    }

    if (peek() == Token::LeftBrace)
      break;
    else if (peek() != Token::Comma)
      throw ExpectedComma{};

    unsafe_read(); // reads the comma ','
  }
}

std::shared_ptr<ast::FunctionDecl> DeclParser::parseDestructor()
{
  const Token leftpar = read();
  if (leftpar != Token::LeftPar)
    throw ExpectedLeftPar{};
  const Token rightpar = read();
  if (rightpar != Token::RightPar)
    throw ExpectedRightPar{};

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
  if (mDecision != Undecided)
    throw ImplementationError{"DeclParser::setDecision() : decision already set"};

  mDecision = d;
  if (mDecision == ParsingVariable)
  {
    mFuncDecl = nullptr;
  }
  else if (isParsingFunction())
  {
    mVarDecl = nullptr;
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


void DeclParser::readArgs()
{
  const Token leftPar = read();
  if (leftPar != Token::LeftPar)
    throw ExpectedLeftPar{};

  std::vector<std::shared_ptr<ast::Expression>> args;
  SentinelFragment sentinel{ Token::RightPar, fragment() };
  while (!sentinel.atEnd())
  {
    ListFragment listfrag{ &sentinel };
    ExpressionParser ep{ &listfrag };
    auto expr = ep.parse();
    args.push_back(expr);
      
    listfrag.consumeComma();
  }

  const Token rightpar = sentinel.consumeSentinel();
  mVarDecl->init = ast::ConstructorInitialization::New(leftPar, std::move(args), rightpar);
}

void DeclParser::readParams()
{
  const Token leftpar = readToken<ExpectedLeftPar>(Token::LeftPar);

  SentinelFragment sentinel{ Token::RightPar, fragment() };
  while (!sentinel.atEnd())
  {
    ListFragment listfrag{ &sentinel };
    FunctionParamParser pp{ &listfrag };
    auto param = pp.parse();
    mFuncDecl->params.push_back(param);

    listfrag.consumeComma();
  }

  sentinel.consumeSentinel();
}


void DeclParser::readArgsOrParams()
{
  const Token leftPar = read();
  assert(leftPar == Token::LeftPar);

  if (mDecision == Undecided || mDecision == ParsingVariable)
    mVarDecl->init = ast::ConstructorInitialization::New(leftPar, {}, parser::Token{});

  SentinelFragment sentinel{ Token::RightPar, fragment() };
  while (!sentinel.atEnd())
  {
    ListFragment listfrag{ &sentinel };

    if (mDecision == Undecided || mDecision == ParsingVariable)
    {
      ExpressionParser ep{ &listfrag };
      try
      {
        auto expr = ep.parse();
        mVarDecl->init->as<ast::ConstructorInitialization>().args.push_back(expr);
      }
      catch (const ParserException &)
      {
        if (mDecision == ParsingVariable)
          throw;
        else
          mDecision = ParsingFunction;

        mVarDecl = nullptr;
      }
    }

    auto position = pos();

    if (mDecision == Undecided || isParsingFunction())
    {
      listfrag.seekBegin();

      FunctionParamParser pp{ &listfrag };
      try
      {
        auto param = pp.parse();
        mFuncDecl->params.push_back(param);
      }
      catch (const ParserException &)
      {
        if (isParsingFunction())
          throw;
        else
          mDecision = ParsingVariable;

        mFuncDecl = nullptr;
      }
    }

    if (!listfrag.atEnd())
      listfrag.data()->seek(position);
    assert(listfrag.atEnd());

    listfrag.consumeComma();
  }

  const Token rightpar = sentinel.consumeSentinel();

  if (mVarDecl != nullptr)
    mVarDecl->init->as<ast::ConstructorInitialization>().right_par = rightpar;
}

bool DeclParser::readOptionalConst()
{
  if (peek() != Token::Const)
    return false;
  
  if (mDecision == ParsingVariable)
    throw UnexpectedToken{};
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

  auto p = pos();
  const Token eqSign = read();

  if (atEnd())
    throw UnexpectedEndOfInput{};

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
    throw UnexpectedEndOfInput{};

  const Token semicolon = readToken<ExpectedSemicolon>(Token::Semicolon);

  return true;
}

bool DeclParser::readOptionalDefaultSpecifier()
{
  if (mDecision == ParsingVariable)
    return false;

  if (peek() != Token::Eq)
    return false;

  auto p = pos();
  const Token eqSign = read();

  if (atEnd())
    throw UnexpectedEndOfInput{};

  if (peek() != Token::Default)
  {
    seek(p);
    return false;
  }

  const Token defspec = read();
  mFuncDecl->defaultKeyword = defspec;

  mDecision = ParsingFunction;
  mVarDecl = nullptr;

  const Token semicolon = readToken<ExpectedSemicolon>(Token::Semicolon);

  return true;
}

bool DeclParser::readOptionalVirtualPureSpecifier()
{
  if (mDecision == ParsingVariable)
    return false;

  if (peek() != Token::Eq)
    return false;

  auto p = pos();
  const Token eqSign = read();

  if (peek().type != Token::OctalLiteral)
  {
    seek(p);
    return false;
  }

  mFuncDecl->virtualPure = read();

  if (text(mFuncDecl->virtualPure) != "0")
    throw ExpectedZero{};

  mDecision = ParsingFunction;
  mVarDecl = nullptr;

  const Token semicolon = readToken<ExpectedSemicolon>(Token::Semicolon);

  return true;
}


std::shared_ptr<ast::CompoundStatement> DeclParser::readFunctionBody()
{
  if (peek() != Token::LeftBrace)
    throw ExpectedLeftBrace{};

  ProgramParser pParser{ fragment() };
  return std::dynamic_pointer_cast<ast::CompoundStatement>(pParser.parseStatement());
}

bool DeclParser::detectCtorDecl()
{
  if (!mExplicitKw.isValid())
    return false;

  auto p = pos();
  IdentifierParser ip{ fragment() };
  std::shared_ptr<ast::Identifier> iden;
  try
  {
    iden = ip.parse();
    if (!isClassName(iden))
    {
      seek(p);
      return false;
    }
  }
  catch (const ParserException &)
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
    throw UnexpectedEndOfInput{};

  IdentifierParser ip{ fragment(), IdentifierParser::ParseSimpleId | IdentifierParser::ParseTemplateId };
  auto iden = ip.parse();

  if (!isClassName(iden))
    throw ExpectedCurrentClassName{};

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

  const auto p = pos();
  
  const Token opKw = read();
  TypeParser tp{ fragment() };
  tp.setReadFunctionSignature(false); // people should use a typedef in this situtation
  ast::QualifiedType type;
  try
  {
    type = tp.parse();
  }
  catch (const ParserException &)
  {
    if (mExplicitKw.isValid())
      throw CouldNotReadType{};
    else {
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
  return compareName(name, mClassName, fragment()->data());
}

bool compareName(const std::shared_ptr<ast::Identifier> & a, const std::shared_ptr<ast::Identifier> & b, const std::shared_ptr<ParserData> & data);

static bool compare_template_names(const std::shared_ptr<ast::TemplateIdentifier> & a, const std::shared_ptr<ast::TemplateIdentifier> & b, const std::shared_ptr<ParserData> & data)
{
  if (a->arguments.size() != b->arguments.size())
    return false;

  if (a->getName() != b->getName())
    return false;

  for (size_t i(0); i < a->arguments.size(); ++i)
  {
    if (!(a->arguments.at(i)->is<ast::Identifier>() && b->arguments.at(i)->is<ast::Identifier>()))
      return false;

    if (!compareName(std::dynamic_pointer_cast<ast::Identifier>(a->arguments.at(i)), std::dynamic_pointer_cast<ast::Identifier>(b->arguments.at(i)), data))
      return false;
  }

  return false;
}

static bool compare_qualified_names(const std::shared_ptr<ast::ScopedIdentifier> & a, const std::shared_ptr<ast::ScopedIdentifier> & b, const std::shared_ptr<ParserData> & data)
{
  /// TODO
  return false;
}

bool compareName(const std::shared_ptr<ast::Identifier> & a, const std::shared_ptr<ast::Identifier> & b, const std::shared_ptr<ParserData> & data)
{
  if (a->type() != b->type())
    return false;

  switch (a->type())
  {
  case ast::NodeType::SimpleIdentifier:
    return a->getName() == b->getName();
  case ast::NodeType::OperatorName:
  case ast::NodeType::LiteralOperatorName:
    return a->name.type == b->name.type;
  case ast::NodeType::TemplateIdentifier:
    return compare_template_names(std::dynamic_pointer_cast<ast::TemplateIdentifier>(a), std::dynamic_pointer_cast<ast::TemplateIdentifier>(b), data);
  case ast::NodeType::QualifiedIdentifier:
    return compare_qualified_names(std::dynamic_pointer_cast<ast::ScopedIdentifier>(a), std::dynamic_pointer_cast<ast::ScopedIdentifier>(b), data);
  default:
    break;
  }

  return false;
}



EnumValueParser::EnumValueParser(AbstractFragment *fragment)
  : ParserBase(fragment)
{

}

void EnumValueParser::parse()
{
  while (!atEnd())
  {
    ListFragment frag{ fragment() };
    if (frag.atEnd())
    {
      frag.consumeComma();
      continue;
    }

    IdentifierParser idparser{ &frag, IdentifierParser::ParseOnlySimpleId };
    auto name = idparser.parse();
    if (frag.atEnd())
    {
      values.push_back(ast::EnumValueDeclaration{ name, nullptr });
      frag.consumeComma();
      continue;
    }

    if (frag.peek() != Token::Eq)
      throw ExpectedEqualSign{};

    const Token equalsign = unsafe_read();

    ExpressionParser valparser{ &frag };
    auto expr = valparser.parse();

    values.push_back(ast::EnumValueDeclaration{ name, expr });
    frag.consumeComma();
  }
}


EnumParser::EnumParser(AbstractFragment *fragment)
  : ParserBase(fragment)
{

}

std::shared_ptr<ast::EnumDeclaration> EnumParser::parse()
{
  const Token & etok = read();

  Token ctok;
  if (peek() == Token::Class)
    ctok = read();

  std::shared_ptr<ast::Identifier> enum_name;
  {
    IdentifierParser idparser{ fragment(), IdentifierParser::ParseOnlySimpleId };
    enum_name = idparser.parse();
  }

  if (peek() != Token::LeftBrace)
    throw ExpectedLeftBrace{};

  read(); // reads left brace

  SentinelFragment sentinel{ Token::RightBrace, fragment() };
  EnumValueParser value_parser{ &sentinel };
  value_parser.parse();

  if (peek() != Token::RightBrace)
    throw ExpectedRightBrace{};

  read(); // reads right brace

  const Token semicolon = readToken<ExpectedSemicolon>(Token::Semicolon);

  return ast::EnumDeclaration::New(etok, ctok, enum_name, std::move(value_parser.values));
}



ClassParser::ClassParser(AbstractFragment *fragment)
  : ParserBase(fragment)
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

  const Token leftbrace = readToken<ExpectedLeftBrace>(Token::LeftBrace);

  while (!readClassEnd())
    readNode();

  return mClass;
}

void ClassParser::parseAccessSpecifier()
{
  const Token visibility = unsafe_read();

  if (peek() != Token::Colon)
    throw ExpectedColon{};

  const Token colon = unsafe_read();

  mClass->content.push_back(ast::AccessSpecifier::New(visibility, colon));
}


std::shared_ptr<ast::Identifier> ClassParser::readClassName()
{
  auto opts = mTemplateSpecialization ? IdentifierParser::ParseTemplateId : 0;
  IdentifierParser nameParser{ fragment(), opts | IdentifierParser::ParseSimpleId };
  return nameParser.parse();
}

void ClassParser::readOptionalParent()
{
  if (atEnd())
    throw UnexpectedEndOfInput{};

  if (peek() != Token::Colon)
    return;

  const Token colon = read();

  if (atEnd())
    throw UnexpectedEndOfInput{};

  IdentifierParser nameParser{ fragment(), IdentifierParser::ParseTemplateId}; // TODO : forbid read operator name directly here
  auto parent = nameParser.parse();
  //if(parent->is<ast::OperatorName>())
  //  throw Error{ "Unexpected operator name read after ':' " };

  mClass->parent = parent;
}

void ClassParser::readDecl()
{
  if (atEnd())
    throw UnexpectedEndOfInput{};

  DeclParser dp{ fragment(), mClass->name };
  
  if (!dp.detectDecl())
    throw ExpectedDeclaration{};

  mClass->content.push_back(dp.parse());
}

void ClassParser::readNode()
{
  switch (peek().type)
  {
  case Token::Public:
  case Token::Protected:
  case Token::Private:
    parseAccessSpecifier();
    return;
  default:
    break;
  }

  readDecl();
}

bool ClassParser::readClassEnd()
{
  if (peek().type != Token::RightBrace)
    return false;

  const Token rightbrace = unsafe_read();
  const Token semicolon = readToken<ExpectedSemicolon>(Token::Semicolon);

  return true;
}

Token ClassParser::read(Token::Type tt)
{
  if (atEnd())
    throw UnexpectedEndOfInput{};

  Token r = read();
  if (r != tt)
    throw UnexpectedToken{};
  return r;
}

Parser::Parser()
  : ProgramParser(nullptr)
{

}

Parser::Parser(const SourceFile & source)
  : ProgramParser(nullptr)
{
  m_fragment = std::make_unique<ScriptFragment>(std::make_shared<ParserData>(source));
  m_fragment->data()->mAst = std::make_shared<ast::AST>(source);
  reset(m_fragment.get());
}

std::shared_ptr<ast::AST> Parser::parse(const SourceFile & source)
{
  ScriptFragment frag{ std::make_shared<ParserData>(source) };
  reset(&frag);

  std::shared_ptr<ast::AST> ret = std::make_shared<ast::AST>(source);
  frag.data()->mAst = ret;

  try
  {
    while (!atEnd())
    {
      ret->add(parseStatement());
      fragment()->data()->clearBuffer();
    }
  }
  catch (const ParserException & e)
  {
    ret->setErrorFlag();
    if(atEnd())
      ret->log(diagnostic::error() << e.what());
    else
      ret->log(diagnostic::error() << diagnostic::pos(peek().line, peek().column) << e.what());
  }

  return ret;
}

std::shared_ptr<ast::AST> Parser::parseExpression(const SourceFile & source)
{
  ScriptFragment frag{ std::make_shared<ParserData>(source) };
  reset(&frag);

  std::shared_ptr<ast::AST> ret = std::make_shared<ast::AST>(source);
  frag.data()->mAst = ret;

  try
  {
    ExpressionParser ep{ &frag };
    auto expr = ep.parse();
    ret->setExpression(expr);
  }
  catch (const ParserException & e)
  {
    ret->setErrorFlag();
    if (atEnd())
      ret->log(diagnostic::error() << e.what());
    else
      ret->log(diagnostic::error() << diagnostic::pos(peek().line, peek().column) << e.what());
  }

  return ret;
}


std::shared_ptr<ast::ClassDecl> Parser::parseClassDeclaration()
{
  ClassParser cp{ fragment() };
  return cp.parse();
}

} // parser

} // namespace script

