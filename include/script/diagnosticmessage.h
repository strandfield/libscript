// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_DIAGNOSTIC_MESSAGE_H
#define LIBSCRIPT_DIAGNOSTIC_MESSAGE_H

#include "libscriptdefs.h"

#include "script/exception.h"
#include "script/operators.h"
#include "script/parser/token.h"

#include <string>

namespace script
{

enum class AccessSpecifier;
class Engine;
class Type;

namespace parser
{
class ParserException;
} // namespace parser

namespace compiler
{
class CompilerException;
} // namespace compiler

namespace diagnostic
{

const std::string & format(const std::string & str);
std::string format(std::string str, const std::string & a);
std::string format(std::string str, const std::string & arg1, const std::string & arg2);
std::string format(std::string str, const std::string & arg1, const std::string & arg2, const std::string & arg3);
std::string format(std::string str, const std::string & arg1, const std::string & arg2, const std::string & arg3, const std::string & arg4);

enum Severity {
  Info = 1,
  Warning = 2,
  Error = 3,
};

enum Verbosity {
  Terse = 1,
  Normal = 2,
  Verbose = 3,
  Pedantic = 4,
};

/// Format : [Severity]line:col: content
class LIBSCRIPT_API Message
{
public:
  Message(Severity severity = Info, ErrorCode code = ErrorCode::NoError);
  Message(const Message &) = default;

  Message(const std::string & str, Severity severity = Info, ErrorCode code = ErrorCode::NoError);
  Message(std::string && str, Severity severity = Info, ErrorCode code = ErrorCode::NoError);
  ~Message() = default;

  inline Severity severity() const { return mSeverity; }
  std::string message() const;
  inline std::string to_string() const { return message(); }
  const std::string & content() const;

  inline const ErrorCode & code() const { return mCode; }

  inline int line() const { return mLine; }
  inline int column() const { return mColumn; }

  Message & operator=(const Message & other) = default;
  Message & operator=(Message && other);

private:
  friend class MessageBuilder;
  int16_t mLine;
  int16_t mColumn;
  Severity mSeverity;
  ErrorCode mCode;
  std::string mContent;
};


struct line_t { int line; };
line_t line(int l);

struct pos_t { int line;  int column; };
pos_t pos(int l, int column);
inline pos_t nullpos() { return pos_t{ -1, -1 }; }

bool isCompilerError(ErrorCode code);

class MessageBuilder
{
public:
  MessageBuilder(Severity s, Engine *e = nullptr);
  MessageBuilder(const MessageBuilder &) = default;
  ~MessageBuilder() = default;
  
  inline Engine * engine() const { return mEngine; }

  inline static std::string repr(bool b) { return std::to_string(b); }
  inline static std::string repr(char c) { return std::string{ c }; }
  inline static std::string repr(unsigned int n) { return std::to_string(int(n)); }
  inline static std::string repr(unsigned long n) { return std::to_string(int(n)); }
  inline static std::string repr(int n) { return std::to_string(n); }
  inline static const std::string & repr(const std::string & str) { return str; }
  static std::string repr(script::AccessSpecifier as);
  static std::string repr(script::OperatorName op);
  std::string repr(const Type & t) const;
  std::string repr(const parser::Token & tok) const;
  const std::string & repr(const parser::Token::Type & tok) const;

  template<typename...Args>
  std::string format(const std::string & fmt, const Args &... args)
  {
    return diagnostic::format(fmt, repr(args)...);
  }

  MessageBuilder & operator<<(int n);
  MessageBuilder & operator<<(line_t l);
  MessageBuilder & operator<<(pos_t p);
  MessageBuilder & operator<<(const std::string & str);
  MessageBuilder & operator<<(std::string && str);
  MessageBuilder & operator<<(const Exception & ex);
  MessageBuilder & operator<<(const parser::ParserException & ex);
  MessageBuilder & operator<<(const compiler::CompilerException & ex);
  inline MessageBuilder & operator<<(const char *str) { return (*this) << std::string{ str }; }

  inline MessageBuilder & operator<<(Engine *e) { mEngine = e; return *(this); }

  template<typename T>
  MessageBuilder & operator<<(const T & as)
  {
    return (*this) << repr(as);
  }

  Message build() const;
  operator Message() const;

  Engine *mEngine;
  ErrorCode mCode;
  Severity mSeverity;
  std::string mBuffer;
  int mLine;
  int mColumn;
};

MessageBuilder info(Engine *e = nullptr);
MessageBuilder warning(Engine *e = nullptr);
MessageBuilder error(Engine *e = nullptr);

} // namespace diagnostic

} // namespace script

#endif // LIBSCRIPT_DIAGNOSTIC_MESSAGE_H
