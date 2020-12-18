// Copyright (C) 2020 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_PARSER_BASE_H
#define LIBSCRIPT_PARSER_BASE_H

#include "script/ast/ast_p.h"

#include "script/parser/parsererrors.h"
#include "script/parser/token-reader.h"

namespace script
{

namespace parser
{

struct LIBSCRIPT_API ParserContext
{
private:
  const char* m_source;
  size_t m_size;
  std::vector<Token> m_tokens;
public:
  explicit ParserContext(const char* src);
  explicit ParserContext(const std::string& str);
  ParserContext(const char* src, size_t s);
  ParserContext(const char* src, std::vector<Token> tokens);
  ~ParserContext();

  const char* source() const { return m_source; }
  size_t source_length() const { return m_size; }

  const std::vector<Token>& tokens() const { return m_tokens; }
};

class LIBSCRIPT_API ParserBase : protected TokenReader
{
public:
  ParserBase(std::shared_ptr<ParserContext> shared_context, const TokenReader& reader);
  virtual ~ParserBase();

  void reset(std::shared_ptr<ParserContext> shared_context, const TokenReader& reader);

  const Fragment& fragment() const;
  Fragment::iterator iterator() const;
  bool atEnd() const;

protected:
 
  const std::shared_ptr<ParserContext>& context() const;

  size_t offset() const;

  template<typename T>
  auto parse_and_seek(T& parser) -> decltype(parser.parse())
  {
    auto ret = parser.parse();
    seek(parser.iterator());
    return ret;
  }

  SyntaxError SyntaxErr(ParserError e)
  {
    SyntaxError err{ e };
    err.offset = offset();
    return err;
  }

  template<typename T>
  SyntaxError SyntaxErr(ParserError e, T&& d)
  {
    SyntaxError err{ e, std::forward<T>(d) };
    err.offset = offset();
    return err;
  }

protected:
  std::shared_ptr<ParserContext> m_context;
};

} // namespace parser

} // namespace script

#endif // LIBSCRIPT_PARSER_BASE_H
