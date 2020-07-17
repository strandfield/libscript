// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/parser/lexer.h"

#include <cstring>
#include <memory>
#include <stdexcept>

namespace script
{

namespace parser
{

Lexer::Lexer()
  : mSource(nullptr)
  , mLength(0)
  , mPos(0)
  , mLine(0)
  , mColumn(0)
{

}

Lexer::Lexer(const SourceFile & src)
  : mSourceFile(src)
  , mSource(nullptr)
  , mLength(0)
  , mPos(0)
  , mLine(0)
  , mColumn(0)
{
  mSource = mSourceFile.data();
  mLength = mSourceFile.content().size();
  consumeDiscardable(); /// TODO : should it be moved to another method like 'start()'
  // in such case, we should also add a 'isReady()' method
}


Token Lexer::read()
{
  if (this->atEnd())
    throw std::runtime_error{ "Lexer::read() : reached end of input" };

  Position p = position();

  char c = readChar();
  auto ct = ctype(c);
  
  Token result;
  switch (ct)
  {
  case Digit:
    result = readNumericLiteral(p);
    break;
  case DoubleQuote:
    result = readStringLiteral(p);
    break;
  case SingleQuote:
    result = readCharLiteral(p);
    break;
  case Letter:
  case Underscore:
    result = readIdentifier(p);
    break;
  case LeftPar:
    result = create(p, 1, Token::LeftPar, Token::Punctuator);
    break;
  case RightPar:
    result = create(p, 1, Token::RightPar, Token::Punctuator);
    break;
  case LeftBrace:
    result = create(p, 1, Token::LeftBrace, Token::Punctuator);
    break;
  case RightBrace:
    result = create(p, 1, Token::RightBrace, Token::Punctuator);
    break;
  case LeftBracket:
    result = create(p, 1, Token::LeftBracket, Token::Punctuator);
    break;
  case RightBracket:
    result = create(p, 1, Token::RightBracket, Token::Punctuator);
    break;
  case Semicolon:
    result = create(p, 1, Token::Semicolon, Token::Punctuator);
    break;
  case Colon:
    result = readColonOrColonColon(p);
    break;
  case QuestionMark:
    result = create(p, 1, Token::QuestionMark, Token::OperatorToken);
    break;
  case Comma:
    result = create(p, 1, Token::Comma, Token::OperatorToken);
    break;
  case Dot:
    result = create(p, 1, Token::Dot, Token::OperatorToken);
    break;
  case Punctuator:
    result = readFromPunctuator(p);
    break;
  default:
    throw std::runtime_error{ "Lexer::read() : Unexpected input char" };
  }

  consumeDiscardable();
  return result;
}

std::string Lexer::text(const Token & t) const
{
  // @TODO: find a way to ensure that 't' was generated from 'mSource' 
  return std::string{ mSource + t.pos, mSource + t.pos + t.length };
}

bool Lexer::atEnd() const
{
  return mPos == mLength;
}

int Lexer::pos() const
{
  return mPos;
}

int Lexer::line() const
{
  return mLine;
}

int Lexer::col() const
{
  return mColumn;
}

Lexer::Position Lexer::position() const
{
  return Position{ pos(), line(), col() };
}

Lexer::Position Lexer::position(const Token & tok)
{
  return Position{ tok.pos, tok.line, tok.column };
}

void Lexer::seek(const Position & pos)
{
  mPos = pos.pos;
  mColumn = pos.col;
  mLine = pos.line;
}

void Lexer::reset()
{
  mSourceFile = SourceFile{};
  mSource = nullptr;
  mPos = 0;
  mLine = 0;
  mColumn = 0;
}

void Lexer::setSource(const SourceFile & src)
{
  reset();
  mSourceFile = src;
  if (!mSourceFile.isLoaded())
    mSourceFile.load();
  mSource = src.data();
  mLength = src.content().size();
  consumeDiscardable();
}

char Lexer::readChar()
{
  // @TODO: remove bounds checking
  // All calls to this function seem to perform a check for atEnd().

  if (atEnd())
    throw std::runtime_error{ "Lexer::readChar() : end of input" };

  auto it = mSource + mPos;
  if (*it == '\n') {
    mLine++;
    mColumn = 0;
  }
  else {
    mColumn++;
  }

  mPos++;
  return *it;
}

char Lexer::charAt(const Position & pos)
{
  return mSource[pos.pos];
}

char Lexer::currentChar() const
{
  return mSource[mPos];
}

void Lexer::consumeDiscardable()
{
  while (!atEnd() && isDiscardable(peekChar())) 
    readChar();
}

Token Lexer::create(const Position & pos, int length, Token::Id type, int flags)
{
  return Token{ type, flags, pos.pos, length, pos.line, pos.col };
}

Token Lexer::create(const Position & pos, Token::Id type, int flags)
{
  return Token{ type, flags, pos.pos, this->pos() - pos.pos, pos.line, pos.col };
}

Lexer::CharacterType Lexer::ctype(char c)
{
  static const CharacterType map[] = {
    Invalid, // NUL    (Null char.)
    Invalid, // SOH    (Start of Header)
    Invalid, // STX    (Start of Text)
    Invalid, // ETX    (End of Text)
    Invalid, // EOT    (End of Transmission)
    Invalid, // ENQ    (Enquiry)
    Invalid, // ACK    (Acknowledgment)
    Invalid, // BEL    (Bell)
    Invalid, //  BS    (Backspace)
    Tabulation, //  HT    (Horizontal Tab)
    LineBreak, //  LF    (Line Feed)
    Invalid, //  VT    (Vertical Tab)
    Invalid, //  FF    (Form Feed)
    CarriageReturn, //  CR    (Carriage Return)
    Invalid, //  SO    (Shift Out)
    Invalid, //  SI    (Shift In)
    Invalid, // DLE    (Data Link Escape)
    Invalid, // DC1    (XON)(Device Control 1)
    Invalid, // DC2    (Device Control 2)
    Invalid, // DC3    (XOFF)(Device Control 3)
    Invalid, // DC4    (Device Control 4)
    Invalid, // NAK    (Negative Acknowledgement)
    Invalid, // SYN    (Synchronous Idle)
    Invalid, // ETB    (End of Trans. Block)
    Invalid, // CAN    (Cancel)
    Invalid, //  EM    (End of Medium)
    Invalid, // SUB    (Substitute)
    Invalid, // ESC    (Escape)
    Invalid, //  FS    (File Separator)
    Invalid, //  GS    (Group Separator)
    Invalid, //  RS    (Request to Send)(Record Separator)
    Invalid, //  US    (Unit Separator)
    Space, //  SP    (Space)
    Punctuator, //   !    (exclamation mark)
    DoubleQuote, //   "    (double quote)
    Punctuator, //   #    (number sign)
    Punctuator, //   $    (dollar sign)
    Punctuator, //   %    (percent)
    Punctuator, //   &    (ampersand)
    SingleQuote, //   '    (single quote)
    LeftPar, //   (    (left opening parenthesis)
    RightPar, //   )    (right closing parenthesis)
    Punctuator, //   *    (asterisk)
    Punctuator, //   +    (plus)
    Comma, //   ,    (comma)
    Punctuator, //   -    (minus or dash)
    Dot, //   .    (dot)
    Punctuator, //   /    (forward slash)
    Digit, //   0
    Digit, //   1
    Digit, //   2
    Digit, //   3
    Digit, //   4
    Digit, //   5
    Digit, //   6
    Digit, //   7
    Digit, //   8
    Digit, //   9
    Colon, //   :    (colon)
    Semicolon, //   ;    (semi-colon)
    Punctuator, //   <    (less than sign)
    Punctuator, //   =    (equal sign)
    Punctuator, //   >    (greater than sign)
    QuestionMark, //   ?    (question mark)
    Punctuator, //   @    (AT symbol)
    Letter, //   A
    Letter, //   B
    Letter, //   C
    Letter, //   D
    Letter, //   E
    Letter, //   F
    Letter, //   G
    Letter, //   H
    Letter, //   I
    Letter, //   J
    Letter, //   K
    Letter, //   L
    Letter, //   M
    Letter, //   N
    Letter, //   O
    Letter, //   P
    Letter, //   Q
    Letter, //   R
    Letter, //   S
    Letter, //   T
    Letter, //   U
    Letter, //   V
    Letter, //   W
    Letter, //   X
    Letter, //   Y
    Letter, //   Z
    LeftBracket, //   [    (left opening bracket)
    Punctuator, //   \    (back slash)
    RightBracket, //   ]    (right closing bracket)
    Punctuator, //   ^    (caret cirumflex)
    Underscore, //   _    (underscore)
    Punctuator, //   `
    Letter, //   a
    Letter, //   b
    Letter, //   c
    Letter, //   d
    Letter, //   e
    Letter, //   f
    Letter, //   g
    Letter, //   h
    Letter, //   i
    Letter, //   j
    Letter, //   k
    Letter, //   l
    Letter, //   m
    Letter, //   n
    Letter, //   o
    Letter, //   p
    Letter, //   q
    Letter, //   r
    Letter, //   s
    Letter, //   t
    Letter, //   u
    Letter, //   v
    Letter, //   w
    Letter, //   x
    Letter, //   y
    Letter, //   z
    LeftBrace, //   {    (left opening brace)
    Punctuator, //   |    (vertical bar)
    RightBrace, //   }    (right closing brace)
    Punctuator, //   ~    (tilde)
    Invalid, // DEL    (delete)
  };

  if(c <= 127)
    return map[c];
  return Other;
}

bool Lexer::isDiscardable(char c)
{
  return ctype(c) == Space || ctype(c) == LineBreak || ctype(c) == CarriageReturn || ctype(c) == Tabulation;
}


template<>
bool Lexer::checkAfter<Token::DecimalLiteral>() const
{
  return atEnd() || ctype(peekChar()) != Letter;
}


Token Lexer::readNumericLiteral(const Position & start)
{
  if (atEnd()) {
    if (charAt(start) == '0')
      return create(start, 1, Token::OctalLiteral, Token::Literal);
    else
      return create(start, 1, Token::IntegerLiteral, Token::Literal);
  }

  char c = peekChar();

  // Reading binary, octal or hexadecimal number
  // eg. : 0b00110111
  //       018
  //       0xACDBE
  if (charAt(start) == '0' && c != '.')
  {
    if (c == 'x') // hexadecimal
      return readHexa(start);
    else if (c == 'b') // binary
      return readBinary(start);
    else if (Lexer::isDigit(c))// octal
      return readOctal(start);
    else // it is zero
    {
      if (!checkAfter<Token::DecimalLiteral>())
        throw std::runtime_error{ "Lexer::readNumericLiteral() : error" };
      return create(start, Token::OctalLiteral, Token::Literal);
    }
  }

  return readDecimal(start);
}



template<>
bool Lexer::checkAfter<Token::HexadecimalLiteral>() const
{
  return atEnd() || ctype(peekChar()) != Letter;
}

Token Lexer::readHexa(const Position & start)
{
  const char x = readChar();
  assert(x == 'x');

  if (atEnd())  // input ends with '0x' -> error
    throw std::runtime_error{ "Lexer::readHexa() : unexpected end of input" };

  while (!atEnd() && Lexer::isHexa(peekChar()))
    readChar();

  if(pos() - start.pos == 2 || !checkAfter<Token::HexadecimalLiteral>()) // e.g. 0x+
    throw std::runtime_error{ "Lexer::readHexa() : unexpected end of input" };
  
  return create(start, Token::HexadecimalLiteral, Token::Literal);
}

template<>
bool Lexer::checkAfter<Token::OctalLiteral>() const
{
  return checkAfter<Token::HexadecimalLiteral>();
}

Token Lexer::readOctal(const Position & start)
{
  while (!atEnd() && Lexer::isOctal(peekChar()))
    readChar();

  if (!checkAfter<Token::OctalLiteral>())
    throw std::runtime_error{ "Lexer::readOCtal() : unexpected end of input" };

  return create(start, Token::OctalLiteral, Token::Literal);
}


template<>
bool Lexer::checkAfter<Token::BinaryLiteral>() const
{
  return checkAfter<Token::HexadecimalLiteral>();
}

Token Lexer::readBinary(const Position & start)
{
  const char b = readChar();
  assert(b == 'b');

  if (atEnd())  // input ends with '0b' -> error
    throw std::runtime_error{ "Lexer::readBinary() : unexpected end of input" };

  while (!atEnd() && Lexer::isBinary(peekChar()))
    readChar();

  if (!checkAfter<Token::BinaryLiteral>())
    throw std::runtime_error{ "Lexer::readBinary() : unexpected end of input" };

  return create(start, Token::BinaryLiteral, Token::Literal);
}

Token Lexer::readDecimal(const Position & start)
{
  // Reading decimal numbers
  // eg. : 25
  //       3.14
  //       3.14f
  //       100e100
  //       6.02e23
  //       6.67e-11

  while (!atEnd() && Lexer::isDigit(peekChar()))
    readChar();

  if (atEnd())
    create(start, Token::IntegerLiteral, Token::Literal);

  bool is_decimal = false;

  if (peekChar() == '.')
  {
    readChar();
    is_decimal = true;

    while (!atEnd() && Lexer::isDigit(peekChar()))
      readChar();

    if (atEnd())
      return create(start, Token::DecimalLiteral, Token::Literal);
  }

  if (peekChar() == 'e')
  {
    readChar();
    is_decimal = true;

    if (atEnd())
      throw std::runtime_error{ "Lexer::readDecimal() : unexpected end of input while reading floating point literal" };

    if (peekChar() == '+' || peekChar() == '-')
    {
      readChar();
      if (atEnd())
        throw std::runtime_error{ "Lexer::readDecimal() : unexpected end of input while reading floating point literal" };
    }

    while (!atEnd() && Lexer::isDigit(peekChar()))
      readChar();

    if (atEnd())
      return create(start, Token::DecimalLiteral, Token::Literal);
  }


  if (peekChar() == 'f') // eg. 125.f
  {
    readChar();
    is_decimal = true;
  }
  else
  {
    if (tryReadLiteralSuffix())
      return create(start, Token::UserDefinedLiteral, Token::Literal);
  }

  if(!checkAfter<Token::DecimalLiteral>())
    throw std::runtime_error{ "Lexer::readDecimal() : unexpected input char after floating point literal" };

  return create(start, is_decimal ? Token::DecimalLiteral : Token::IntegerLiteral, Token::Literal);
}

bool Lexer::tryReadLiteralSuffix()
{
  auto cpos = pos();

  if (!this->atEnd() && (Lexer::isLetter(peekChar()) || peekChar() == '_'))
    readChar();
  else
    return false;

  while (!this->atEnd() && (Lexer::isLetter(peekChar()) || Lexer::isDigit(peekChar()) || peekChar() == '_'))
    readChar();

  const bool read = (cpos != pos());
  return read;
}

Token Lexer::readIdentifier(const Position & start)
{
  while (!this->atEnd() && (Lexer::isLetter(peekChar()) || Lexer::isDigit(peekChar()) || peekChar() == '_'))
    readChar();

  Token::Id id = identifierType(start.pos, pos());

  int keyword_flag = id != Token::UserDefinedName ? Token::Keyword : 0;
  int literal_flag = (id == Token::False || id == Token::True) ? Token::Literal : 0;

  return create(start, pos() - start.pos, id, Token::Identifier | keyword_flag | literal_flag);
}


struct Keyword {
  const char *name;
  Token::Id toktype;
};

const Keyword l2k[] = {
  { "if", Token::If },
};

const Keyword l3k[] = {
  { "for", Token::For },
  { "int", Token::Int },
};

const Keyword l4k[] = {
  { "auto", Token::Auto },
  { "bool", Token::Bool },
  { "char", Token::Char },
  { "else", Token::Else },
  { "enum", Token::Enum },
  { "this", Token::This },
  { "true", Token::True },
  { "void", Token::Void },
};

const Keyword l5k[] = {
  { "break", Token::Break },
  { "class", Token::Class },
  { "const", Token::Const },
  { "false", Token::False },
  { "float", Token::Float },
  { "using", Token::Using },
  { "while", Token::While },
};

const Keyword l6k[] = {
  { "delete", Token::Delete },
  { "double", Token::Double },
  { "export", Token::Export },
  { "friend", Token::Friend },
  { "import", Token::Import },
  { "public", Token::Public },
  { "return", Token::Return },
  { "static", Token::Static },
  { "struct", Token::Struct },
  { "typeid", Token::Typeid },
};

const Keyword l7k[] = {
  { "default", Token::Default },
  { "mutable", Token::Mutable },
  { "private", Token::Private },
  { "typedef", Token::Typedef },
  { "virtual", Token::Virtual },
};

const Keyword l8k[] = {
  { "continue", Token::Continue },
  { "explicit", Token::Explicit },
  { "operator", Token::Operator },
  { "template", Token::Template },
  { "typename", Token::Typename },
};

const Keyword l9k[] = {
  { "namespace", Token::Namespace },
  { "protected", Token::Protected },
};

static Token::Id findKeyword(const Keyword * keywords, int arraySize, const char *str, int length)
{
  for (int i(0); i < arraySize; ++i) {
    if (std::memcmp(keywords[i].name, str, length) == 0)
      return keywords[i].toktype;
  }
  return Token::UserDefinedName;
}

Token::Id Lexer::identifierType(int begin, int end) const
{
  const char *str = mSource + begin;
  const int l = end - begin;

  switch (l) {
  case 1:
    return Token::UserDefinedName;
  case 2:
    return findKeyword(l2k, sizeof(l2k) / sizeof(Keyword), str, l);
  case 3:
    return findKeyword(l3k, sizeof(l3k) / sizeof(Keyword), str, l);
  case 4:
    return findKeyword(l4k, sizeof(l4k) / sizeof(Keyword), str, l);
  case 5:
    return findKeyword(l5k, sizeof(l5k) / sizeof(Keyword), str, l);
  case 6:
    return findKeyword(l6k, sizeof(l6k) / sizeof(Keyword), str, l);
  case 7:
    return findKeyword(l7k, sizeof(l7k) / sizeof(Keyword), str, l);
  case 8:
    return findKeyword(l8k, sizeof(l8k) / sizeof(Keyword), str, l);
  case 9:
    return findKeyword(l9k, sizeof(l9k) / sizeof(Keyword), str, l);
  default:
    break;
  }

  return Token::UserDefinedName;
}


template<>
bool Lexer::checkAfter<Token::StringLiteral>() const
{
  return atEnd() || ctype(peekChar()) != Digit;
}

Token Lexer::readStringLiteral(const Position & start)
{
  while (!atEnd() && peekChar() != '"')
  {
    if (peekChar() == '\\')
    {
      readChar();
      if (!atEnd())
        readChar();
    }
    else if (peekChar() == '\n')
      throw std::runtime_error{ "Lexer::readStringLiteral() : end of line reached before end of string literal " };
    else
      readChar();
  }

  if(atEnd())
    throw std::runtime_error{ "Lexer::readStringLiteral() : unexpected end of input before end of string literal " };

  assert(peekChar() == '"');
  readChar();

  if (tryReadLiteralSuffix())
    return create(start, Token::UserDefinedLiteral, Token::Literal);
  
  if (!checkAfter<Token::StringLiteral>())
    throw std::runtime_error{ "Lexer::readStringLiteral() : unexpected character after string literal" };

  return create(start, Token::StringLiteral, Token::Literal);
}

Token Lexer::readCharLiteral(const Position & start)
{
  if(atEnd())
    throw std::runtime_error{ "Lexer::readCharLiteral() : unexpected end of input before end of char-literal " };

  readChar();

  if (atEnd())
    throw std::runtime_error{ "Lexer::readCharLiteral() : unexpected end of input before end of char-literal " };

  if(ctype(readChar()) != SingleQuote)
    throw std::runtime_error{ "Lexer::readCharLiteral() : malformed char-literal " };

  return create(start, Token::StringLiteral, Token::Literal);
}

Token Lexer::readFromPunctuator(const Position & start)
{
  char p = mSource[mPos - 1]; /// bad, TODO : clean up
  if (p == '/')
  {
    if (atEnd())
      return create(start, Token::Div, Token::OperatorToken);
    if (peekChar() == '/')
      return readSingleLineComment(start);
    else if (peekChar() == '*')
      return readMultiLineComment(start);
    else
      return readOperator(start);
  }
  
  return this->readOperator(start);
}


Token Lexer::readColonOrColonColon(const Position & start)
{
  if (atEnd())
    return create(start, Token::Colon, Token::OperatorToken);

  if (peekChar() == ':')
  {
    readChar();
    return create(start, Token::ScopeResolution, Token::OperatorToken);
  }

  return create(start, Token::Colon, Token::OperatorToken);
}


struct OperatorLexeme {
  const char *name;
  Token::Id toktype;
};


const OperatorLexeme l1op[] = {
  { "+", Token::Plus },
  { "-", Token::Minus },
  { "!", Token::LogicalNot },
  { "~", Token::BitwiseNot },
  { "*", Token::Mul },
  { "/", Token::Div },
  { "%", Token::Remainder },
  { "<", Token::Less },
  { ">", Token::GreaterThan },
  { "&", Token::BitwiseAnd },
  { "^", Token::BitwiseXor },
  { "|", Token::BitwiseOr },
  { "=", Token::Eq },
};

const OperatorLexeme l2op[] = {
  { "++", Token::PlusPlus },
  { "--", Token::MinusMinus },
  { "<<", Token::LeftShift },
  { ">>", Token::RightShift },
  { "<=", Token::LessEqual },
  { ">=", Token::GreaterThanEqual },
  { "==", Token::EqEq },
  { "!=", Token::Neq },
  { "&&", Token::LogicalAnd },
  { "||", Token::LogicalOr },
  { "*=", Token::MulEq },
  { "/=", Token::DivEq },
  { "%=", Token::RemainderEq },
  { "+=", Token::AddEq },
  { "-=", Token::SubEq },
  { "&=", Token::BitAndEq },
  { "|=", Token::BitOrEq },
  { "^=", Token::BitXorEq },
};


const OperatorLexeme l3op[] = {
  { "<<=", Token::LeftShiftEq },
  { ">>=", Token::RightShiftEq },
};


static Token::Id findOperator(const OperatorLexeme * ops, int arraySize, const char *str, int length)
{
  for (int i(0); i < arraySize; ++i) {
    if (std::memcmp(ops[i].name, str, length) == 0)
      return ops[i].toktype;
  }
  return Token::Invalid;
}


Token::Id Lexer::getOperator(int begin, int end) const
{
  const char *str = mSource + begin;
  const int l = end - begin;

  switch (l) {
  case 1:
    return findOperator(l1op, sizeof(l1op) / sizeof(OperatorLexeme), str, l);
  case 2:
    return findOperator(l2op, sizeof(l2op) / sizeof(OperatorLexeme), str, l);
  case 3:
    return findOperator(l3op, sizeof(l3op) / sizeof(OperatorLexeme), str, l);
  default:
    break;
  }

  return Token::Invalid;
}

Token Lexer::readOperator(const Position & start)
{
  Token::Id op = getOperator(start.pos, pos());
  if (op == Token::Invalid)
    throw std::runtime_error{ "Lexer::readOperator() : no operator found starting with given chars" };
  
  while (!atEnd())
  {
    Position p = position();
    readChar();
    Token::Id candidate = getOperator(start.pos, pos());
    if (candidate == Token::Invalid)
    {
      seek(p);
      break;
    }
    else
      op = candidate;
  }

  return create(start, op, Token::OperatorToken);
}

Token Lexer::readSingleLineComment(const Position & start)
{
  readChar(); // reads the second '/'

  while (!atEnd() && peekChar() != '\n')
    readChar();

  return create(start, Token::SingleLineComment, 0);
}

Token Lexer::readMultiLineComment(const Position & start)
{
  readChar(); // reads the '*' after opening '/'

  do {
    while (!atEnd() && peekChar() != '*')
      readChar();

    if (atEnd())
      throw std::runtime_error{ "Lexer::readMultiLineComment() : unexpected end of input before end of comment" };

    assert(peekChar() == '*');
    readChar(); // reads the '*'

    if (atEnd())
      throw std::runtime_error{ "Lexer::readMultiLineComment() : unexpected end of input before end of comment" };

  } while (peekChar() != '/');

  readChar(); // reads the closing '/'
  return create(start, Token::MultiLineComment, 0);
}


} // namespace parser

} // namespace script
