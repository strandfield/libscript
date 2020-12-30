// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_NAME_H
#define LIBSCRIPT_NAME_H

#include "script/operators.h"
#include "script/types.h"

namespace script
{

class LIBSCRIPT_API Name
{
public:

  enum Kind {
    InvalidName = 0,
    StringName = 1,
    OperatorName = 2,
    CastName = 3,
    LiteralOperatorName = 4,
  };

  Name();
  Name(const Name & other);
  Name(Name && other);
  ~Name();

  Name(const std::string & str);
  Name(script::OperatorName op);
  struct LiteralOperatorTag {};
  Name(LiteralOperatorTag, const std::string & suffix);
  struct CastTag {};
  Name(CastTag, const Type & t);

  inline Kind kind() const { return kind_; }

  inline const std::string & string() const { return data_.string; }
  inline const script::OperatorName operatorName() const { return data_.operation; }
  inline const Type asType() const { return data_.type; }

  Name & operator=(const Name & other);
  Name & operator=(Name && other);

  Name & operator=(std::string && str);

  friend LIBSCRIPT_API bool operator==(const Name & lhs, const Name & rhs);

private:
  Kind kind_;
  union Storage {
    Storage();
    ~Storage();
    std::string string;
    script::OperatorName operation;
    Type type;
  };
  Storage data_;
};

LIBSCRIPT_API bool operator==(const Name & lhs, const Name & rhs);
inline bool operator!=(const Name & lhs, const Name & rhs) { return !(lhs == rhs); }

} // namespace script

#endif // LIBSCRIPT_NAME_H
