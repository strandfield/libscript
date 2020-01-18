// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_TEMPLATE_ARGUMENT_H
#define LIBSCRIPT_TEMPLATE_ARGUMENT_H

#include "script/types.h"

#include "script/ast/forwards.h"

#include <memory>
#include <vector>

namespace script
{

class TemplateArgumentPack;

class LIBSCRIPT_API TemplateArgument
{
public:
  enum Kind {
    UnspecifiedArgument = 0,
    TypeArgument,
    IntegerArgument,
    BoolArgument,
    PackArgument,
  };

public:
  Kind kind;
  Type type;
  int integer;
  bool boolean;
  std::shared_ptr<TemplateArgumentPack> pack;

public:
  TemplateArgument();
  explicit TemplateArgument(const Type & t);
  explicit TemplateArgument(const Type::BuiltInType & t);
  explicit TemplateArgument(int n);
  explicit TemplateArgument(bool b);
  explicit TemplateArgument(std::vector<TemplateArgument> && args);
};

LIBSCRIPT_API bool operator==(const TemplateArgument & lhs, const TemplateArgument & rhs);
inline bool operator!=(const TemplateArgument & lhs, const TemplateArgument & rhs) { return !(lhs == rhs); }

class LIBSCRIPT_API TemplateArgumentPack
{
private:
  std::vector<TemplateArgument> arguments_;

public:
  TemplateArgumentPack() = default;
  TemplateArgumentPack(const TemplateArgumentPack &) = default;
  TemplateArgumentPack(TemplateArgumentPack &&) = default;
  ~TemplateArgumentPack() = default;

  explicit TemplateArgumentPack(std::vector<TemplateArgument> && args) : arguments_(std::move(args)) { }

  inline const std::vector<TemplateArgument> & args() const { return arguments_; }
  inline size_t size() const { return arguments_.size(); }
  inline const TemplateArgument & at(size_t i) const { return arguments_.at(i); }
  inline std::vector<TemplateArgument>::const_iterator begin() const { return arguments_.begin(); }
  inline std::vector<TemplateArgument>::const_iterator end() const { return arguments_.end(); }

  TemplateArgumentPack & operator=(const TemplateArgumentPack &) = default;
};


// implements operator< for template arguments
struct TemplateArgumentComparison
{
  static int compare(const TemplateArgument & a, const TemplateArgument & b);
  bool operator()(const TemplateArgument & a, const TemplateArgument & b) const;
  bool operator()(const std::vector<TemplateArgument> & a, const std::vector<TemplateArgument> & b) const;
};

} // namespace script

#endif // LIBSCRIPT_TEMPLATE_ARGUMENT_H
