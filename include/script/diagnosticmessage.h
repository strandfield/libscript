// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_DIAGNOSTIC_MESSAGE_H
#define LIBSCRIPT_DIAGNOSTIC_MESSAGE_H

#include "libscriptdefs.h"

#include "script/exception.h"
#include "script/sourcefile.h"

#include "script/operators.h"
#include "script/parser/token.h"

#include <string>
#include <system_error>

namespace script
{

enum class AccessSpecifier;
class Engine;
class Type;

namespace parser
{
class SyntaxError;
} // namespace parser

namespace compiler
{
class CompilationFailure;
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

template<diagnostic::Severity S>
class Message
{
private:
  std::error_code m_error_code;
  SourceLocation m_location;
  std::string m_content;

public:
  Message() = default;
  Message(const Message<S>&) = default;
  Message(Message<S>&&) = default;
  ~Message() = default;

  Message(std::error_code ec, std::string text)
    : m_error_code(ec),
      m_location(),
      m_content(std::move(text))
  {

  }

  Message(std::error_code ec, SourceLocation loc, std::string text)
    : m_error_code(ec),
      m_location(loc),
      m_content(std::move(text))
  {

  }

  constexpr Severity severity() const { return S; }
  constexpr bool isInfo() const { return severity() == Severity::Info; }
  constexpr bool isWarning() const { return severity() == Severity::Warning; }
  constexpr bool isError() const { return severity() == Severity::Error; }

  std::error_code errorCode() const { return m_error_code; }
  const SourceLocation& location() const { return m_location; }
  const std::string& content() const { return m_content; }
  
  Message& operator=(const Message<S>&) = default;
  Message& operator=(Message<S>&&) = default;
};

/// Format : [Severity]line:col: content
class LIBSCRIPT_API DiagnosticMessage
{
public:
  DiagnosticMessage() = default;
  DiagnosticMessage(const DiagnosticMessage&) = default;
  DiagnosticMessage(DiagnosticMessage&&) = default;
  ~DiagnosticMessage() = default;

  DiagnosticMessage(Severity s, std::error_code ec, SourceLocation loc, std::string text);
  DiagnosticMessage(Severity s, std::error_code ec, std::string text);

  inline Severity severity() const { return mSeverity; }
  std::string message() const;
  inline std::string to_string() const { return message(); }
  const std::string& content() const;

  inline const std::error_code& code() const { return mCode; }

  const SourceLocation& location() const { return mLocation; }
  inline int line() const { return static_cast<int>(location().m_pos.line); }
  inline int column() const { return static_cast<int>(location().m_pos.col); }

  DiagnosticMessage& operator=(const DiagnosticMessage& ) = default;
  DiagnosticMessage& operator=(DiagnosticMessage&& ) = default;

private:
  friend class MessageBuilder;
  Severity mSeverity = Severity::Info;
  SourceLocation mLocation;
  std::error_code mCode;
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
  virtual ~MessageBuilder();
  
  inline Engine * engine() const { return mEngine; }

  inline static std::string repr(bool b) { return std::to_string(b); }
  inline static std::string repr(char c) { return std::string{ c }; }
  //inline static std::string repr(unsigned int n) { return std::to_string(int(n)); }
  //inline static std::string repr(unsigned long n) { return std::to_string(int(n)); }
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
  MessageBuilder & operator<<(size_t n);
  MessageBuilder & operator<<(line_t l);
  MessageBuilder & operator<<(pos_t p);
  MessageBuilder & operator<<(const std::string & str);
  MessageBuilder & operator<<(std::string && str);
  MessageBuilder& operator<<(const parser::SyntaxError& ex);
  MessageBuilder& operator<<(const compiler::CompilationFailure& ex);
  inline MessageBuilder & operator<<(const char *str) { return (*this) << std::string{ str }; }

  inline MessageBuilder & operator<<(Engine *e) { mEngine = e; return *(this); }

  template<typename T>
  MessageBuilder & operator<<(const T & as)
  {
    return (*this) << repr(as);
  }

  DiagnosticMessage build() const;
  operator DiagnosticMessage() const;

  Engine *mEngine;
  std::error_code mCode;
  Severity mSeverity;
  std::string mBuffer;
  int mLine;
  int mColumn;
};

MessageBuilder info(Engine *e = nullptr);
MessageBuilder warning(Engine *e = nullptr);
MessageBuilder error(Engine *e = nullptr);

} // namespace diagnostic

typedef diagnostic::DiagnosticMessage DiagnosticMessage;

} // namespace script

#endif // LIBSCRIPT_DIAGNOSTIC_MESSAGE_H
