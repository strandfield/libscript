// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/parser/errors.h"
#include "script/parser/parsererrors.h"

namespace script
{

namespace errors
{

class ParserCategory : public std::error_category
{
public:

  const char* name() const noexcept override
  {
    return "parser-category";
  }

  std::string message(int) const override
  {
    return "parser-error";
  }
};

const std::error_category& parser_category() noexcept
{
  static ParserCategory static_instance = {};
  return static_instance;
}

} // namespace errors

namespace parser
{

ParserErrorData::~ParserErrorData()
{

}

} // namespace parser

} // namespace script

