// Copyright (C) 2020 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_STRINGVIEW_H
#define LIBSCRIPT_STRINGVIEW_H

#include <cstring>
#include <string>

namespace script
{

namespace utils
{

class StringView
{
private:
  const char* m_data;
  size_t m_size;

public:
  StringView()
    : m_data(""), m_size(0)
  {

  }

  StringView(const StringView&) = default;

  StringView(const char* str, size_t s)
    : m_data(str), m_size(s)
  {

  }

  StringView(const char* str)
    : m_data(str), m_size(std::strlen(str))
  {

  }

  size_t size() const { return m_size; }
  const char* data() const { return m_data; }

  char at(size_t index) const { return m_data[index]; }

  bool starts_with(const char* str) const
  {
    size_t offset = 0;

    for (; offset < size() && *str != '\0'; ++offset, ++str)
    {
      if (*str != at(offset))
        return false;
    }

    return *str == '\0';
  }

  std::string toString() const { return std::string(data(), data() + size()); }

  StringView& operator=(const StringView&) = default;
};

inline bool operator==(const StringView& lhs, const StringView& rhs)
{
  return lhs.size() == rhs.size() &&
    (lhs.data() == rhs.data() || std::memcmp(lhs.data(), rhs.data(), lhs.size()) == 0);
}

inline bool operator!=(const StringView& lhs, const StringView& rhs)
{
  return !(lhs == rhs);
}

inline bool operator==(const StringView& lhs, const char* rhs)
{
  size_t offset = 0;

  for (; offset < lhs.size() && *rhs != '\0'; ++offset, ++rhs)
  {
    if (*rhs != lhs.data()[offset])
      return false;
  }

  return offset == lhs.size() && *rhs == '\0';
}

inline bool operator!=(const StringView& lhs, const char* rhs)
{
  return !(lhs == rhs);
}

inline bool operator==(const StringView& lhs, const std::string& str)
{
  return lhs == StringView(str.data(), str.size());
}

inline bool operator!=(const StringView& lhs, const std::string& rhs)
{
  return !(lhs == rhs);
}

} // namespace utils

} // namespace script


#endif // LIBSCRIPT_STRINGVIEW_H
