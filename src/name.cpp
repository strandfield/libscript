// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/name.h"

namespace script
{


Name::Storage::Storage()
{

}

Name::Storage::~Storage()
{

}

Name::Name()
  : kind_(InvalidName)
{

}

Name::Name(const Name & other)
  : kind_(other.kind())
{
  switch (kind_)
  {
  case Name::InvalidName:
    break;
  case Name::StringName:
  case Name::LiteralOperatorName:
    new (&data_.string) std::string{ other.data_.string };
    break;
  case Name::OperatorName:
    data_.operation = other.data_.operation;
    break;
  case Name::CastName:
    data_.type = other.data_.type;
    break;
  }
}

Name::Name(Name && other)
  : kind_(other.kind())
{
  switch (kind_)
  {
  case Name::InvalidName:
    break;
  case Name::StringName:
  case Name::LiteralOperatorName:
    new (&data_.string) std::string{ std::move(other.data_.string) };
    other.data_.string.~basic_string();
    break;
  case Name::OperatorName:
    data_.operation = other.data_.operation;
    break;
  case Name::CastName:
    data_.type = other.data_.type;
    break;
  }

  other.kind_ = Name::InvalidName;
}

Name::~Name()
{
  switch (kind_)
  {
  case Name::InvalidName:
  case Name::OperatorName:
  case Name::CastName:
    break;
  case Name::StringName:
  case Name::LiteralOperatorName:
    data_.string.~basic_string();
    break;
  }

  kind_ = InvalidName;
}

Name::Name(const std::string & str)
  : kind_(Name::StringName)
{
  new (&data_.string) std::string{ str };
}

Name::Name(Operator::BuiltInOperator op)
  : kind_(Name::OperatorName)
{
  data_.operation = op;
}


Name::Name(LiteralOperatorTag, const std::string & suffix)
  : kind_(Name::LiteralOperatorName)
{
  new (&data_.string) std::string{ suffix };
}

Name::Name(CastTag, const Type & t)
  : kind_(Name::CastName)
{
  data_.type = t;
}

Name & Name::operator=(const Name & other)
{
  if (kind_ == Name::StringName || kind_ == Name::LiteralOperatorName)
  {
    if (other.kind_ == Name::StringName || other.kind_ == Name::LiteralOperatorName)
    {
      data_.string = other.data_.string;
    }
    else
    {
      data_.string.~basic_string();

      if (other.kind_ == Name::OperatorName)
        data_.operation = other.data_.operation;
      else
        data_.type = other.data_.type;
    }
  }
  else
  {
    if (other.kind_ == Name::StringName || other.kind_ == Name::LiteralOperatorName)
    {
      new (&data_.string) std::string{ other.data_.string };
    }
    else
    {
      if (other.kind_ == Name::OperatorName)
        data_.operation = other.data_.operation;
      else
        data_.type = other.data_.type;
    }
  }

  kind_ = other.kind_;

  return (*this);
}

Name & Name::operator=(Name && other)
{
  if (kind_ == Name::StringName || kind_ == Name::LiteralOperatorName)
  {
    if (other.kind_ == Name::StringName || other.kind_ == Name::LiteralOperatorName)
    {
      data_.string = std::move(other.data_.string);
      other.data_.string.~basic_string();
    }
    else
    {
      data_.string.~basic_string();

      if (other.kind_ == Name::OperatorName)
        data_.operation = other.data_.operation;
      else
        data_.type = other.data_.type;
    }
  }
  else
  {
    if (other.kind_ == Name::StringName || other.kind_ == Name::LiteralOperatorName)
    {
      new (&data_.string) std::string{ std::move(other.data_.string) };
      other.data_.string.~basic_string();
    }
    else
    {
      if (other.kind_ == Name::OperatorName)
        data_.operation = other.data_.operation;
      else
        data_.type = other.data_.type;
    }
  }

  kind_ = other.kind_;
  other.kind_ = Name::InvalidName;

  return (*this);
}

Name & Name::operator=(std::string && str)
{
  if (kind_ == Name::StringName || kind_ == Name::LiteralOperatorName)
    data_.string = std::move(str);
  else
    new (&data_.string) std::string{ str };

  kind_ = Name::StringName;

  return (*this);
}


bool operator==(const Name & lhs, const Name & rhs)
{
  if (lhs.kind() != rhs.kind())
    return false;

  if (lhs.kind() == Name::InvalidName)
    return true;

  if (lhs.kind() == Name::OperatorName)
    return lhs.data_.operation == rhs.data_.operation;
  else if (lhs.kind() == Name::CastName)
    return lhs.data_.type == rhs.data_.type;

  return lhs.data_.string == rhs.data_.string;
}

} // namespace script
