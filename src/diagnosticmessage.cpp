// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/diagnosticmessage.h"

namespace script
{

namespace diagnostic
{

inline static bool is_digit(const char c)
{
  return c >= '0' && c <= '9';
}

struct placemarker_t
{
  size_t pos;
  size_t length;
  int value;
};

placemarker_t read_placemarker(const std::string & str, size_t pos)
{
  size_t num_start = pos + 1;
  size_t end = num_start;
  while (end < str.size() && is_digit(str[end]))
    ++end;

  if (end == num_start)
    return placemarker_t{ std::string::npos, 0, -1 };

  return placemarker_t{ pos, end - pos, std::stoi(str.substr(num_start, end - num_start)) };
}

placemarker_t find_lowest_placemarker(const std::string & str)
{
  std::size_t pos = str.find('%');
  if (pos == std::string::npos)
    return placemarker_t{ pos, 0, -1 };

  placemarker_t pm = read_placemarker(str, pos);
  pos = str.find('%', pos + 1);
  while (pos != std::string::npos)
  {
    placemarker_t new_pm = read_placemarker(str, pos);
    if (new_pm.value != -1 && new_pm.value < pm.value)
      pm = new_pm;
    pos = str.find('%', pos + 1);
  }

  return pm;
}

std::string format(std::string str)
{
  return str;
}

std::string format(std::string str, const std::string & a)
{
  placemarker_t pm = find_lowest_placemarker(str);
  if (pm.value == -1)
    return str;

  str.replace(str.begin() + pm.pos, str.begin() + pm.pos + pm.length, a);
  return str;
}



Message::Message(const std::string & str)
  : mMessage(str)
{

}

Message::Message(std::string && str)
  : mMessage(std::move(str))
{

}

Severity Message::severity() const
{
  if (mMessage.find("[warning]") == 0)
    return Warning;
  else if (mMessage.find("[error]") == 0)
    return Error;
  return Info;
}

const std::string & Message::message() const
{
  return mMessage;
}

int Message::line() const
{
  size_t offset = mMessage.find("[warning]") == 0 ? 9 : 0;
  offset += mMessage.find("[error]") == 0 ? 7 : 0;
  offset += mMessage.find("[info]") == 0 ? 6 : 0;

  size_t end = offset;
  while (end < mMessage.size() && is_digit(mMessage[end]))
    ++end;

  if (end > offset)
    return std::stoi(mMessage.substr(offset, end - offset));

  return -1;
}

int Message::column() const
{
  size_t offset = mMessage.find("[warning]") == 0 ? 9 : 0;
  offset += mMessage.find("[error]") == 0 ? 7 : 0;
  offset += mMessage.find("[info]") == 0 ? 6 : 0;

  size_t end = offset;
  while (end < mMessage.size() && is_digit(mMessage[end]))
    ++end;

  if (end == mMessage.size() || mMessage[end] != ':')
    return -1;

  offset = end + 1;
  end = offset;

  while (end < mMessage.size() && is_digit(mMessage[end]))
    ++end;

  if (end > offset)
    return std::stoi(mMessage.substr(offset, end - offset));

  return -1;
}

Message & Message::operator=(Message && other)
{
  this->mMessage = std::move(other.mMessage);
  return *(this);
}

line_t line(int l)
{
  return line_t{ l };
}

pos_t pos(int l, int col)
{
  return pos_t{ l, col };
}

MessageBuilder::MessageBuilder(Severity s)
  : mSeverity(s)
  , mLine(-1)
  , mColumn(-1)
{

}

MessageBuilder & MessageBuilder::operator<<(int n)
{
  mBuffer += std::to_string(n);
  return *(this);
}

MessageBuilder & MessageBuilder::operator<<(line_t l)
{
  mLine = l.line;
  return *(this);
}

MessageBuilder & MessageBuilder::operator<<(pos_t p)
{
  mLine = p.line;
  mColumn = p.column;
  return *(this);
}

MessageBuilder & MessageBuilder::operator<<(const std::string & str)
{
  mBuffer += std::move(str);
  return *(this);
}

MessageBuilder & MessageBuilder::operator<<(std::string && str)
{
  mBuffer += std::move(str);
  return *(this);
}

Message MessageBuilder::build() const
{
  std::string mssg;
  switch (mSeverity)
  {
  case Info:
    mssg += "[info]";
    break;
  case Warning:
    mssg += "[warning]";
    break;
  case Error:
    mssg += "[error]";
    break;
  }

  if (mLine >= 0)
  {
    mssg += std::to_string(mLine);
    mssg += std::string{ ":" };
    if (mColumn >= 0)
      mssg += std::to_string(mColumn) + std::string{ ":" };
  }

  mssg += " ";

  mssg += mBuffer;

  return Message{ mssg };
}

MessageBuilder::operator Message() const
{
  return build();
}


MessageBuilder info()
{
  return MessageBuilder{ Info };
}

MessageBuilder warning()
{
  return MessageBuilder{ Warning };
}

MessageBuilder error()
{
  return MessageBuilder{ Error };
}

} // namespace diagnostic

} // namespace script
