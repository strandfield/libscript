// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_TOKEN_H
#define LIBSCRIPT_TOKEN_H

#include "libscriptdefs.h"

namespace script
{

namespace parser
{

class Token
{
public:
  Token();
  Token(const Token & other) = default;
  ~Token() = default;


  enum Kind {
    Punctuator = 0x010000,
    Literal    = 0x020000,
    OperatorToken   = 0x040000,
    Identifier = 0x080000,
    Keyword    = 0x100000 | Identifier,
  };

  enum Type {
    Invalid = 0,
    // literals 
    IntegerLiteral = Literal | 1,
    DecimalLiteral = Literal | 2,
    BinaryLiteral = Literal | 3,
    OctalLiteral = Literal | 4,
    HexadecimalLiteral = Literal | 5,
    // do we need to create a CharLiteral 
    StringLiteral = Literal | 6,
    // punctuators 
    LeftPar = Punctuator | 7,
    RightPar = Punctuator | 8,
    LeftBracket = Punctuator | 9,
    RightBracket = Punctuator | 10,
    LeftBrace = Punctuator | 11,
    RightBrace = Punctuator | 12,
    Semicolon = Punctuator | 13,
    Colon = Punctuator | 14,
    Dot = Punctuator | 15,
    QuestionMark = Punctuator | 16,
    SlashSlash = Punctuator | 17,
    SlashStar = Punctuator | 18,
    StarSlash = Punctuator | 19,
    // keywords 
    Auto = Keyword | 20,
    Bool = Keyword | 21,
    Break = Keyword | 22,
    Char = Keyword | 23,
    Class = Keyword | 24,
    Const = Keyword | 25,
    Continue = Keyword | 26,
    Default = Keyword | 27,
    Delete = Keyword | 28,
    Double = Keyword | 29,
    Else = Keyword | 30,
    Enum = Keyword | 31,
    Explicit = Keyword | 32,
    Export = Keyword | 33,
    False = Literal | Keyword | 34,
    Float = Keyword | 35,
    For = Keyword | 36,
    Friend = Keyword | 37,
    If = Keyword | 38,
    Import = Keyword | 39,
    Int = Keyword | 40,
    Mutable = Keyword | 41,
    Namespace = Keyword | 42,
    Operator = Keyword | 43,
    Private = Keyword | 44,
    Protected = Keyword | 45,
    Public = Keyword | 46,
    Return = Keyword | 47,
    Static = Keyword | 48,
    Struct = Keyword | 49,
    Template = Keyword | 50,
    This = Keyword | 51,
    True = Literal | Keyword | 52,
    Typedef = Keyword | 53,
    Typeid = Keyword | 54,
    Typename = Keyword | 55,
    Using = Keyword | 56,
    Virtual = Keyword | 57,
    Void = Keyword | 58,
    While = Keyword | 59,
    //Operators
    ScopeResolution = OperatorToken | 60,
    PlusPlus = OperatorToken | 61,
    MinusMinus = OperatorToken | 62,
    Plus = OperatorToken | Punctuator | 63,
    Minus = OperatorToken | Punctuator | 64,
    LogicalNot = OperatorToken | Punctuator | 65,
    BitwiseNot = OperatorToken | Punctuator | 66,
    Mul = OperatorToken | Punctuator | 67,
    Div = OperatorToken | Punctuator | 68,
    Remainder = OperatorToken | Punctuator | 69,
    LeftShift = OperatorToken | 70,
    RightShift = OperatorToken | 71,
    Less = OperatorToken | Punctuator | 72,
    GreaterThan = OperatorToken | Punctuator | 73,
    LessEqual = OperatorToken | 74,
    GreaterThanEqual = OperatorToken | 75,
    EqEq = OperatorToken | 76,
    Neq = OperatorToken | 77,
    BitwiseAnd = OperatorToken | Punctuator | 78,
    BitwiseOr = OperatorToken | Punctuator | 79,
    BitwiseXor = OperatorToken | Punctuator | 80,
    LogicalAnd = OperatorToken | 81,
    LogicalOr = OperatorToken | 82,
    Eq = OperatorToken | 83,
    MulEq = OperatorToken | 84,
    DivEq = OperatorToken | 85,
    AddEq = OperatorToken | 86,
    SubEq = OperatorToken | 87,
    RemainderEq = OperatorToken | 88,
    LeftShiftEq = OperatorToken | 89,
    RightShiftEq = OperatorToken | 90,
    BitAndEq = OperatorToken | 91,
    BitOrEq = OperatorToken | 92,
    BitXorEq = OperatorToken | 93,
    Comma = OperatorToken | 94,
    UserDefinedName = Identifier | 95,
    UserDefinedLiteral = Literal | 96,
    SingleLineComment = 97,
    LeftRightPar = 98,
    LeftRightBracket = 99,
    //perhaps it would be better to have two tokens for
    //multiline comments : an opening token and a  closing one
    MultiLineComment = 100,
    //alias
    Ampersand = BitwiseAnd,
    Ref = Ampersand,
    RefRef = LogicalAnd,
    LeftAngle = Less,
    RightAngle = GreaterThan,
    LeftLeftAngle = LeftShift,
    RightRightAngle = RightShift,
    Tilde = BitwiseNot,
    Asterisk = Mul,
    Star = Asterisk,
    Zero = OctalLiteral, // Zero is an octal literal of length 1
  };

  Token(Type t, int pos, int size, int line, int column, int src = 0);

  Type type;
  int pos : 24;
  int src : 8;
  int length;
  uint16 line;
  uint16 column;

  inline bool isValid() const { return type != Invalid; }

  inline bool isOperator() const { return type & OperatorToken; }
  inline bool isIdentifier() const { return type & Identifier; }
  inline bool isKeyword() const { return type & Keyword; }
  inline bool isLiteral() const { return type & Literal; }

  bool operator==(const Token & other) const;
  inline bool operator!=(const Token & other) const { return !operator==(other); }
  bool operator==(Type tok) const;
  inline bool operator!=(Type tok) const { return !operator==(tok); }

  Token & operator=(const Token & other) = default;

};


} // parser

} // namespace script


#endif // LIBSCRIPT_TOKEN_H
