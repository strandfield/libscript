// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_LEXER_H
#define LIBSCRIPT_LEXER_H

#include "script/sourcefile.h"
#include "script/parser/token.h"

namespace script
{

namespace parser
{

class LIBSCRIPT_API Lexer
{
public:
  Lexer();
  Lexer(const SourceFile & src);

  Token read();

  bool atEnd() const;

  size_t pos() const;

  void seek(size_t off);
  void reset();
  void setSource(const SourceFile & src);

  enum CharacterType {
    Invalid,
    Space,
    Letter,
    Digit,
    Dot,
    SingleQuote,
    DoubleQuote,
    LeftPar,
    RightPar,
    LeftBrace,
    RightBrace,
    LeftBracket,
    RightBracket,
    Punctuator,
    Underscore,
    Semicolon,
    Colon, 
    QuestionMark,
    Comma,
    Tabulation,
    LineBreak,
    CarriageReturn,
    Other,
  };

  static CharacterType ctype(char c);
  inline static bool isLetter(char c) { return ctype(c) == Letter; }
  inline static bool isDigit(char c) { return ctype(c) == Digit; }
  inline static bool isBinary(char c) { return c == '0' || c == '1'; }
  inline static bool isOctal(char c) { return '0' <= c && c <= '7'; }
  inline static bool isDecimal(char c) { return isDigit(c); }
  inline static bool isHexa(char c) { return isDecimal(c) || ('a' <= c && c <= 'f') || ('A' <= c && c <= 'F'); }
  static bool isDiscardable(char c);
  inline static bool isSpace(char c) { return ctype(c) == Space; }

protected:
  char readChar();
  char charAt(size_t pos);
  char currentChar() const;
  inline char peekChar() const { return currentChar(); }
  void consumeDiscardable();
  Token create(size_t pos, int length, Token::Id type, int flags);
  Token create(size_t pos, Token::Id type, int flags);
  Token readNumericLiteral(size_t pos);
  Token readHexa(size_t pos);
  Token readOctal(size_t pos);
  Token readBinary(size_t pos);
  Token readDecimal(size_t pos);
  Token readIdentifier(size_t pos);
  Token::Id identifierType(int begin, int end) const;
  Token readStringLiteral(size_t pos);
  Token readCharLiteral(size_t pos);
  Token::Id getOperator(int begin, int end) const;
  Token readOperator(size_t pos);
  Token readColonOrColonColon(size_t pos);
  Token readFromPunctuator(size_t pos);
  Token readSingleLineComment(size_t pos);
  Token readMultiLineComment(size_t pos);
  bool tryReadLiteralSuffix();

  template<Token::Id TT>
  bool checkAfter() const; // says if next char is legal

protected:


private:
  const char *mSource;
  SourceFile mSourceFile;
  size_t m_size;
  size_t m_pos;
};

} // parser

} // namespace script

#endif // LIBSCRIPT_LEXER_H
