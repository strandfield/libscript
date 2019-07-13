// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_FUNCTION_TEMPLATE_H
#define LIBSCRIPT_FUNCTION_TEMPLATE_H

#include "script/template.h"

#include "script/function.h"

#include <map>

namespace script
{

class FunctionBuilder;
class FunctionTemplateImpl;
class FunctionTemplateNativeBackend;
struct TemplateArgumentComparison;

namespace program
{
class Expression;
} // namespace program

class LIBSCRIPT_API FunctionTemplate : public Template
{
public:
  FunctionTemplate() = default;
  FunctionTemplate(const FunctionTemplate & other) = default;
  ~FunctionTemplate() = default;

  explicit FunctionTemplate(const std::shared_ptr<FunctionTemplateImpl> & impl);

  FunctionTemplateNativeBackend* backend() const;

  bool hasInstance(const std::vector<TemplateArgument> & args, Function *value = nullptr) const;
  Function getInstance(const std::vector<TemplateArgument> & args);

  Function addSpecialization(const std::vector<TemplateArgument> & args, const FunctionBuilder & opts);

  const std::map<std::vector<TemplateArgument>, Function, TemplateArgumentComparison> & instances() const;

  using Template::get;

  template<typename T>
  static FunctionTemplate get(Engine* e);

  std::shared_ptr<FunctionTemplateImpl> impl() const;

  FunctionTemplate & operator=(const FunctionTemplate & other) = default;
};

template<typename T>
inline static FunctionTemplate FunctionTemplate::get(Engine* e)
{
  static_assert(std::is_base_of<FunctionTemplateNativeBackend, T>::value, "T must be derived from FunctionTemplateNativeBackend");

  const auto& map = get_template_map(e);
  auto it = map.find(std::type_index(typeid(T)));

  if (it == map.end())
  {
    return {};
  }
  else
  {
    return (*it).second.asFunctionTemplate();
  }
}

} // namespace script

#endif // LIBSCRIPT_FUNCTION_TEMPLATE_H
