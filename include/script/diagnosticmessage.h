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

class OverloadResolution;

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

  explicit DiagnosticMessage(Severity s);
  DiagnosticMessage(Severity s, std::error_code ec, SourceLocation loc, std::string text);
  DiagnosticMessage(Severity s, std::error_code ec, std::string text);

  template<diagnostic::Severity S>
  DiagnosticMessage(const Message<S>& mssg)
    : mSeverity(mssg.severity()),
      mLocation(mssg.location()),
      mCode(mssg.errorCode()),
      mContent(mssg.content())
  {

  }

  Severity severity() const { return mSeverity; }
  void setSeverity(Severity sev);

  std::string message() const;
  std::string to_string() const { return message(); }
  const std::string& content() const;
  void setContent(std::string str);

  const std::error_code& code() const { return mCode; }
  void setCode(std::error_code ec);

  const SourceLocation& location() const { return mLocation; }
  int line() const { return static_cast<int>(location().m_pos.line); }
  int column() const { return static_cast<int>(location().m_pos.col); }
  void setLocation(const SourceLocation& loc);

  DiagnosticMessage& operator=(const DiagnosticMessage& ) = default;
  DiagnosticMessage& operator=(DiagnosticMessage&& ) = default;

private:
  friend class MessageBuilder;
  Severity mSeverity = Severity::Info;
  SourceLocation mLocation;
  std::error_code mCode;
  std::string mContent;
};

class MessageBuilder
{
public:
  explicit MessageBuilder(Engine *e);
  MessageBuilder(const MessageBuilder &) = delete;
  virtual ~MessageBuilder();
  
  Engine* engine() const { return mEngine; }

  Verbosity verbosity() const;
  void setVerbosity(Verbosity ver);

  virtual void build(DiagnosticMessage& mssg, const parser::SyntaxError& ex);
  virtual void build(DiagnosticMessage& mssg, const compiler::CompilationFailure& ex);

  template<typename T>
  DiagnosticMessage info(T&& ex)
  {
    DiagnosticMessage mssg{ Severity::Info };
    build(mssg, ex);
    return mssg;
  }

  template<typename T>
  DiagnosticMessage warning(T&& ex)
  {
    DiagnosticMessage mssg{ Severity::Warning };
    build(mssg, ex);
    return mssg;
  }

  template<typename T>
  DiagnosticMessage error(T&& ex)
  {
    DiagnosticMessage mssg{ Severity::Error };
    build(mssg, ex);
    return mssg;
  }

  std::string produce(const OverloadResolution& resol) const;

private:
  Engine *mEngine;
  Verbosity mVerbosity = Verbosity::Normal;
};

} // namespace diagnostic

typedef diagnostic::DiagnosticMessage DiagnosticMessage;

} // namespace script

#endif // LIBSCRIPT_DIAGNOSTIC_MESSAGE_H
