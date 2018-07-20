// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_CLASSBUILDER_H
#define LIBSCRIPT_CLASSBUILDER_H

#include "script/datamember.h"
#include "script/symbol.h"
#include "script/userdata.h"

namespace script
{

class Class;
class ClassTemplate;
class TemplateArgument;

template<typename Derived>
class ClassBuilderBase
{
public:
  Symbol symbol;
  std::string name;
  Type base;
  std::vector<DataMember> dataMembers;
  bool isFinal;
  std::shared_ptr<UserData> userdata;
  int id;

public:
  ClassBuilderBase(const Symbol & s, const std::string & n)
    : symbol(s), name(n), isFinal(false), id(0) { }
  ClassBuilderBase(const Symbol & s, std::string && n)
    : symbol(s), name(std::move(n)), isFinal(false), id(0) { }

  Derived & setBase(const script::Type & b) {
    base = b;
    return *(static_cast<Derived*>(this));
  }

  Derived & setFinal(bool f = true) {
    isFinal = f;
    return *(static_cast<Derived*>(this));
  }

  Derived & add(const DataMember & dm) {
    dataMembers.push_back(dm);
    return *(static_cast<Derived*>(this));
  }

  Derived & add(DataMember && dm) {
    dataMembers.push_back(std::move(dm));
    return *(static_cast<Derived*>(this));
  }

  inline Derived & addMember(const DataMember & dm) { return add(dm); }

  Derived & setData(const std::shared_ptr<UserData> & d) {
    data = d;
    return *(static_cast<Derived*>(this));
  }

  Derived & setId(int n) {
    id = n;
    return *(static_cast<Derived*>(this));
  }

};

class LIBSCRIPT_API ClassBuilder : public ClassBuilderBase<ClassBuilder>
{
private:
  typedef ClassBuilderBase<ClassBuilder> Base;

public:
  ClassBuilder(const Symbol & s, const std::string & n);
  ClassBuilder(const Symbol & s, std::string && n);

  [[deprecated("Use setBase() instead")]] ClassBuilder & setParent(const Class & p);

  ClassBuilder & setBase(const Class & b);

  Class get();
  Class get(const ClassTemplate & ct, std::vector<TemplateArgument> && targs);
  void create();
};

} // namespace script

#endif // LIBSCRIPT_CLASSBUILDER_H
