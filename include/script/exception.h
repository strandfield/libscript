// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_EXCEPTION_H
#define LIBSCRIPT_EXCEPTION_H

#include "libscriptdefs.h"

#include "script/errorcodes.h"

#include <memory>
#include <stdexcept>
#include <system_error>
#include <utility>

#include <string>

namespace script
{

class LIBSCRIPT_API ExceptionData
{
public:
  virtual ~ExceptionData() = default;

  template<typename T>
  T& get() noexcept;

  virtual bool test(const std::type_info& info) const noexcept = 0;
};

template<typename T>
struct ExceptionDataWrapper : ExceptionData
{
public:
  T value;

public:
  ExceptionDataWrapper(const ExceptionDataWrapper<T>&) = delete;
  ~ExceptionDataWrapper() = default;

  ExceptionDataWrapper(T&& data) : value(std::move(data)) { }

  bool test(const std::type_info& info) const noexcept override
  {
    return typeid(T) == info;
  }
};

template<typename T>
inline T& ExceptionData::get() noexcept
{
#ifndef NDEBUG
  assert(this->test(typeid(T)));
#endif // !NDEBUG

  return static_cast<ExceptionDataWrapper<T>*>(this)->value;
}

class LIBSCRIPT_API Exceptional : public std::exception
{
protected:
  std::error_code m_error_code;
  std::shared_ptr<ExceptionData> m_data;

public:
  explicit Exceptional(const std::error_code& err)
    : m_error_code{ err }
  {

  }

  template<typename T>
  Exceptional(const std::error_code& err, T&& data)
    : m_error_code{ err },
    m_data{ std::make_shared<ExceptionDataWrapper<T>>(std::forward<T>(data)) }
  {

  }

  Exceptional(const Exceptional&) noexcept = default;
  Exceptional(Exceptional&&) noexcept = default;

  const std::error_code& errorCode() const noexcept { return m_error_code; }

  const char* what() const noexcept override { return "script-engine-exception"; }

  ExceptionData* data() const noexcept { return m_data.get(); }

  Exceptional& operator=(const Exceptional&) = delete;
  Exceptional& operator=(Exceptional&&) = delete;
};

} // namespace script

#endif // LIBSCRIPT_EXCEPTION_H
