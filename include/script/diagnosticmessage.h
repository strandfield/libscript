// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_DIAGNOSTIC_MESSAGE_H
#define LIBSCRIPT_DIAGNOSTIC_MESSAGE_H

#include "libscriptdefs.h"

#include <string>

namespace script
{

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

class LIBSCRIPT_API Message
{
public:
  Message() = default;
  Message(const Message &) = default;

  Message(const std::string & str);
  Message(std::string && str);
  ~Message() = default;

  Severity severity() const;
  const std::string & message() const;
  inline const std::string & to_string() const { return message(); }
  std::string content() const;

  int line() const;
  int column() const;

  Message & operator=(const Message & other) = default;
  Message & operator=(Message && other);

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
  MessageBuilder(Severity s);
  MessageBuilder(const MessageBuilder &) = default;
  ~MessageBuilder() = default;

  MessageBuilder & operator<<(int n);
  MessageBuilder & operator<<(line_t l);
  MessageBuilder & operator<<(pos_t p);
  MessageBuilder & operator<<(const std::string & str);
  MessageBuilder & operator<<(std::string && str);

  Message build() const;
  operator Message() const;

  Severity mSeverity;
  std::string mBuffer;
  int mLine;
  int mColumn;
};

MessageBuilder info();
MessageBuilder warning();
MessageBuilder error();

} // namespace diagnostic

} // namespace script

#endif // LIBSCRIPT_DIAGNOSTIC_MESSAGE_H
