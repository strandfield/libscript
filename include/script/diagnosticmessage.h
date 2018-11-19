// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_DIAGNOSTIC_MESSAGE_H
#define LIBSCRIPT_DIAGNOSTIC_MESSAGE_H

#include "libscriptdefs.h"

#include <string>

namespace script
{

enum class AccessSpecifier;
class Engine;
class Type;

namespace diagnostic
{

std::string format(std::string str);
std::string format(std::string str, const std::string & a);

template<typename...Args>
std::string format(std::string str, const std::string & a, const Args &... args)
{
  str = format(str, a);
  return format(str, args...);
}

inline std::string repr(bool b) { return std::to_string(b); }
inline std::string repr(char c) { return std::string{ c }; }
inline std::string repr(int n) { return std::to_string(n); }
inline const std::string & repr(const std::string & str) { return str; }

inline std::string repr(bool b, Engine *) { return repr(b); }
inline std::string repr(char c, Engine *) { return repr(c); }
inline std::string repr(unsigned int n, Engine *) { return repr(int(n)); }
inline std::string repr(unsigned long n, Engine *) { return repr(int(n)); }
inline std::string repr(int n, Engine *) { return repr(n); }
inline const std::string & repr(const std::string & str, Engine *) { return str; }

std::string repr(const Type & t, Engine *e = nullptr);
std::string repr(const AccessSpecifier & as, Engine *e = nullptr);


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

class LIBSCRIPT_API Code
{
public:
  Code() = default;
  Code(const Code &) = default;
  ~Code() = default;

  Code(const std::string & str)
    : mValue(str) { }

  const std::string & value() const { return mValue; }

  Code & operator=(const Code &) = default;

private:
  std::string mValue;
};

/// Format : [Severity](Code)line:col: content
class LIBSCRIPT_API Message
{
public:
  Message() = default;
  Message(const Message &) = default;

  Message(const std::string & str);
  Message(std::string && str);
  ~Message() = default;

  Severity severity() const;
  std::string code() const;
  const std::string & message() const;
  inline const std::string & to_string() const { return message(); }
  std::string content() const;

  int line() const;
  int column() const;

  Message & operator=(const Message & other) = default;
  Message & operator=(Message && other);

private:
  static bool read_severity(const std::string & message, size_t & pos);
  static bool read_code(const std::string & message, size_t & pos);
  static bool read_pos(const std::string & message, size_t & pos);

private:
  std::string mMessage;
};


struct line_t { int line; };
line_t line(int l);

struct pos_t { int line;  int column; };
pos_t pos(int l, int column);

class MessageBuilder
{
public:
  MessageBuilder(Severity s, Engine *e = nullptr);
  MessageBuilder(const MessageBuilder &) = default;
  ~MessageBuilder() = default;
  
  inline Engine * engine() const { return mEngine; }

  template<typename...Args>
  std::string format(const std::string & fmt, const Args &... args)
  {
    return diagnostic::format(fmt, repr(args, engine())...);
  }

  MessageBuilder & operator<<(const Code & c);
  MessageBuilder & operator<<(int n);
  MessageBuilder & operator<<(line_t l);
  MessageBuilder & operator<<(pos_t p);
  MessageBuilder & operator<<(const std::string & str);
  MessageBuilder & operator<<(std::string && str);
  inline MessageBuilder & operator<<(const char *str) { return (*this) << std::string{ str }; }

  inline MessageBuilder & operator<<(Engine *e) { mEngine = e; return *(this); }

  template<typename T>
  MessageBuilder & operator<<(const T & as)
  {
    return (*this) << repr(as, engine());
  }

  Message build() const;
  operator Message() const;

  Engine *mEngine;
  Severity mSeverity;
  Code mCode;
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
