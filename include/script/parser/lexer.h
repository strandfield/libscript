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
  Lexer(const SourceFile & src, int id = 0);

  Token read();
  std::string text(const Token & t) const; // TODO : replace with string_view
  bool sameIdentifier(const Token & a, const Token & b);

  bool atEnd() const;

  int pos() const;
  int line() const;
  int col() const;

  struct Position {
    int pos;
    int line;
    int col;
  };
  Position position() const;
  static Position position(const Token & tok);

  void seek(int pos);
  void seek(const Position & pos);
  void reset();
  void setSource(const SourceFile & src, int id = 0);

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
  void unread(int n = 1);
  char charAt(const Position & pos);
  char currentChar() const;
  inline char peekChar() const { return currentChar(); }
  void consumeDiscardable();
  void seek(int pos, int line, int col);
  Token create(const Position & pos, int length, Token::Type type);
  Token create(const Position & pos, Token::Type type);
  Token readNumericLiteral(const Position & pos);
  Token readHexa(const Position & pos);
  Token readOctal(const Position & pos);
  Token readBinary(const Position & pos);
  Token readDecimal(const Position & pos);
  Token readIdentifier(const Position & pos);
  Token::Type identifierType(int begin, int end) const;
  Token readStringLiteral(const Position & pos);
  Token readCharLiteral(const Position & pos);
  Token::Type getOperator(int begin, int end) const;
  Token readOperator(const Position & pos);
  Token readColonOrColonColon(const Position & pos);
  Token readFromPunctuator(const Position & pos);
  Token readSingleLineComment(const Position & pos);
  Token readMultiLineComment(const Position & pos);
  bool tryReadLiteralSuffix();

  template<Token::Type TT>
  bool checkAfter() const; // says if next char is legal

protected:


private:
  const char *mSource;
  SourceFile mSourceFile;
  int mLength;
  int mId;
  int mLine;
  int mColumn;
  int mPos;
};

} // parser

} // namespace script

#endif // LIBSCRIPT_LEXER_H
