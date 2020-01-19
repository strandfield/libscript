// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/diagnosticmessage.h"

#include "script/accessspecifier.h"
#include "script/engine.h"
#include "script/initialization.h"
#include "script/operator.h"
#include "script/overloadresolution.h"
#include "script/typesystem.h"

#include "script/parser/token.h"
#include "script/parser/parsererrors.h"
#include "script/compiler/compilererrors.h"

#include <cstring>
#include <initializer_list>
#include <sstream>

namespace script
{

namespace diagnostic
{

inline static bool is_digit(const char c)
{
  return c >= '0' && c <= '9';
}

inline static bool is_letter(const char c)
{
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

inline static bool is_alphanumeric(const char c)
{
  return is_letter(c) || is_digit(c);
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

const std::string & format(const std::string & str)
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

std::string format(std::string str, const std::string & arg1, const std::string & arg2)
{
  return format(format(str, arg1), arg2);
}

std::string format(std::string str, const std::string & arg1, const std::string & arg2, const std::string & arg3)
{
  return format(format(str, arg1, arg2), arg3);
}

std::string format(std::string str, const std::string & arg1, const std::string & arg2, const std::string & arg3, const std::string & arg4)
{
  return format(format(str, arg1, arg2, arg3), arg3, arg4);
}

DiagnosticMessage::DiagnosticMessage(Severity s)
  : mSeverity(s)
{

}

DiagnosticMessage::DiagnosticMessage(Severity s, std::error_code ec, SourceLocation loc, std::string text)
  : mSeverity(s),
    mCode(ec),
    mLocation(loc),
    mContent(std::move(text))
{

}

DiagnosticMessage::DiagnosticMessage(Severity s, std::error_code ec, std::string text)
  : mSeverity(s),
    mCode(ec),
    mContent(std::move(text))
{

}

void DiagnosticMessage::setSeverity(Severity sev)
{
  mSeverity = sev;
}

std::string DiagnosticMessage::message() const
{
  std::string result;

  switch (mSeverity)
  {
  case Info:
    result += "[info]";
    break;
  case Warning:
    result += "[warning]";
    break;
  case Error:
    result += "[error]";
    break;
  }

  if (!mCode)
  {
    /// TODO: do something with that ?
  }


  if (line() != std::numeric_limits<uint16_t>::max())
  {
    result += std::to_string(line());
    result += std::string{ ":" };
    if (column() != std::numeric_limits<uint16_t>::max())
      result += std::to_string(column()) + std::string{ ":" };
  }

  result += " ";

  result += mContent;

  return result;
}

const std::string & DiagnosticMessage::content() const
{
  return mContent;
}

void DiagnosticMessage::setContent(std::string str)
{
  mContent = std::move(str);
}

void DiagnosticMessage::setCode(std::error_code ec)
{
  mCode = ec;
}

void DiagnosticMessage::setLocation(const SourceLocation& loc)
{
  mLocation = loc;
}

MessageBuilder::MessageBuilder(Engine* e)
  : mEngine(e)
{

}

MessageBuilder::~MessageBuilder()
{

}

Verbosity MessageBuilder::verbosity() const
{
  return mVerbosity;
}

void MessageBuilder::setVerbosity(Verbosity ver)
{
  mVerbosity = ver;
}

void MessageBuilder::build(DiagnosticMessage& mssg, const parser::SyntaxError& ex)
{
  mssg.setCode(ex.errorCode());
  mssg.setLocation(ex.location);
  mssg.setContent(ex.errorCode().message());
}

void MessageBuilder::build(DiagnosticMessage& mssg, const compiler::CompilationFailure& ex)
{
  mssg.setCode(ex.errorCode());
  mssg.setLocation(ex.location);
  mssg.setContent(ex.errorCode().message());
}

std::string MessageBuilder::produce(const OverloadResolution& resol) const
{
  if (resol.success())
    return "Overload resolution succeeded";

  std::stringstream ss;

  if (!resol.ambiguousOverload().isNull())
  {
    ss << "Overload resolution failed because at least two candidates are not comparable or indistinguishable \n";
    ss << engine()->toString(resol.selectedOverload()) << "\n";
    ss << engine()->toString(resol.ambiguousOverload()) << "\n";
    return ss.str();
  }

  ss << "Overload resolution failed because no viable overload could be found \n";
  std::vector<Initialization> paraminits;
  for (const auto& f : resol.candidates())
  {
    auto status = resol.getViabilityStatus(f, &paraminits);
    ss << engine()->toString(f);
    if (status == OverloadResolution::IncorrectParameterCount)
      ss << "\n" << "Incorrect argument count, expects " << f.prototype().count() << " but " << resol.inputSize() << " were provided";
    else if (status == OverloadResolution::CouldNotConvertArgument)
      ss << "\n" << "Could not convert argument " << paraminits.size();
    ss << "\n";
  }
  return ss.str();
}

} // namespace diagnostic

} // namespace script
