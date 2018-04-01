// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/parser/token.h"

namespace script
{

namespace parser
{

Token::Token()
  : type(Token::Invalid)
  , pos(-1)
  , src(-1)
  , length(0)
  , line(-1)
  , column(-1)
{

}

Token::Token(Type t, int pos, int size, int line, int column, int source)
  : type(t)
  , pos(pos)
  , src(source)
  , length(size)
  , line(line)
  , column(column)
{

}

bool Token::operator==(const Token & other) const
{
  return this->type == other.type && this->length == other.length
    && this->pos == other.pos && this->src == other.src;
}

bool Token::operator==(Type tok) const
{
  return this->type == tok;
}

} // namespace parser

} // namespace script

