// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_DESTRUCTORBUILDER_H
#define LIBSCRIPT_DESTRUCTORBUILDER_H

#include "script/functionbuilder.h"

namespace script
{

class Function;

/*!
 * \class DestructorBuilder
 * \brief The DestructorBuilder class is an utility class used to build destructors.
 *
 * See \t GenericFunctionBuilder for a description of builder classes.
 */

//class LIBSCRIPT_API DestructorBuilder : public GenericFunctionBuilder<DestructorBuilder>
//{
//public:
//  typedef DestructorPrototype prototype_t;
//
//public:
//  prototype_t proto_;
//
//public:
//  explicit DestructorBuilder(const Class& cla);
//  DestructorBuilder(const Symbol & s);
//  
//  DestructorBuilder & setDefaulted();
//  DestructorBuilder & setVirtual();
//
//  DestructorBuilder & setReturnType(const Type & t);
//  DestructorBuilder & addParam(const Type & t);
//
//  DestructorBuilder & compile();
//
//  void create();
//  script::Function get();
//};

} // namespace script

#endif // LIBSCRIPT_DESTRUCTORBUILDER_H
