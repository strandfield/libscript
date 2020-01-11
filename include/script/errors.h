// Copyright (C) 2020 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_ERRORS_H
#define LIBSCRIPT_ERRORS_H

#include "script/diagnosticmessage.h"

namespace script
{

class Info;
class Warning;
class Error;

class LIBSCRIPT_API Error : public diagnostic::Message<diagnostic::Severity::Error>
{
public:
  typedef diagnostic::Message<diagnostic::Severity::Error> Base;
  using Base::Base;

  Error() = default;
  Error(const Error&) = default;
  Error(Error&&) = default;
  ~Error() = default;

  Info toInfo() const;
  Warning toWarning() const;
  const Error& toError() const { return *this; }

  Error& operator=(const Error&) = default;
  Error& operator=(Error&&) = default;
};

class LIBSCRIPT_API Warning : public diagnostic::Message<diagnostic::Severity::Warning>
{
public:
  typedef diagnostic::Message<diagnostic::Severity::Warning> Base;
  using Base::Base;

  Warning() = default;
  Warning(const Warning&) = default;
  Warning(Warning&&) = default;
  ~Warning() = default;

  Info toInfo() const;
  const Warning& toWarning() const { return *this; }
  Error toError() const;

  Warning& operator=(const Warning&) = default;
  Warning& operator=(Warning&&) = default;
};

class LIBSCRIPT_API Info : public diagnostic::Message<diagnostic::Severity::Info>
{
public:
  typedef diagnostic::Message<diagnostic::Severity::Info> Base;
  using Base::Base;

  Info() = default;
  Info(const Info&) = default;
  Info(Info&&) = default;
  ~Info() = default;

  const Info& toInfo() const { return *this; }
  Warning toWarning() const;
  Error toError() const;

  Info& operator=(const Info&) = default;
  Info& operator=(Info&&) = default;
};

} // namespace script

namespace script
{

inline Info Error::toInfo() const
{
  return Info{ errorCode(), location(), content() };
}

inline Warning Error::toWarning() const
{
  return Warning{ errorCode(), location(), content() };
}

inline Info Warning::toInfo() const
{
  return Info{ errorCode(), location(), content() };
}

inline Error Warning::toError() const
{
  return Error{ errorCode(), location(), content() };
}

inline Warning Info::toWarning() const
{
  return Warning{ errorCode(), location(), content() };
}

inline Error Info::toError() const
{
  return Error{ errorCode(), location(), content() };
}

} // namespace script

#endif // LIBSCRIPT_ERRORS_H
