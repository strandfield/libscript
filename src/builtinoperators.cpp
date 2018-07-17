// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/private/builtinoperators.h"

#include "script/functionbuilder.h"
#include "script/operator.h"
#include "script/private/operator_p.h"
#include "script/value.h"
#include "script/private/value_p.h"

#include "script/interpreter/executioncontext.h"

namespace script
{

namespace callbacks
{

namespace operators
{

/////////////////////////////// bool //////////////////////////////////


// bool & operator=(bool & a, const bool & b)
Value bool_assign(FunctionCall *c)
{
  c->arg(0).impl()->set_bool(c->arg(1).toBool());
  return c->arg(0);
}

// bool operator==(const bool & a, const bool & b)
Value bool_equal(FunctionCall *c)
{
  return c->engine()->newBool(c->arg(0).toBool() == c->arg(1).toBool());
}

// bool operator!=(const bool & a, const bool & b)
Value bool_inequal(FunctionCall *c)
{
  return c->engine()->newBool(c->arg(0).toBool() != c->arg(1).toBool());
}

// bool operator!(const bool & a)
Value bool_negate(FunctionCall *c)
{
  return c->engine()->newBool(!c->arg(0).toBool());
}

// bool operator&&(const bool & a, const bool & b)
Value bool_logical_and(FunctionCall *c)
{
  return c->engine()->newBool(c->arg(0).toBool() && c->arg(1).toBool());
}

// bool operator||(const bool & a, const bool & b)
Value bool_logical_or(FunctionCall *c)
{
  return c->engine()->newBool(c->arg(0).toBool() || c->arg(1).toBool());
}




/////////////////////////////// char //////////////////////////////////


// char & operator=(char & a, const char & b)
Value char_assign(FunctionCall *c)
{
  c->arg(0).impl()->set_char(c->arg(1).toChar());
  return c->arg(0);
}

// char & operator+=(char & a, const char & b)
Value char_add_assign(FunctionCall *c)
{
  c->arg(0).impl()->set_char(c->arg(0).toChar() + c->arg(1).toChar());
  return c->arg(0);
}

// char & operator-=(char & a, const char & b)
Value char_sub_assign(FunctionCall *c)
{
  c->arg(0).impl()->set_char(c->arg(0).toChar() - c->arg(1).toChar());
  return c->arg(0);
}

// char & operator*=(char & a, const char & b)
Value char_mul_assign(FunctionCall *c)
{
  c->arg(0).impl()->set_char(c->arg(0).toChar() * c->arg(1).toChar());
  return c->arg(0);
}

// char & operator/=(char & a, const char & b)
Value char_div_assign(FunctionCall *c)
{
  c->arg(0).impl()->set_char(c->arg(0).toChar() / c->arg(1).toChar());
  return c->arg(0);
}

// char & operator%=(char & a, const char & b)
Value char_mod_assign(FunctionCall *c)
{
  c->arg(0).impl()->set_char(c->arg(0).toChar() + c->arg(1).toChar());
  return c->arg(0);
}

// char & operator<<=(char & a, const char & b)
Value char_leftshift_assign(FunctionCall *c)
{
  c->arg(0).impl()->set_char(c->arg(0).toChar() << c->arg(1).toChar());
  return c->arg(0);
}

// char & operator>>=(char & a, const char & b)
Value char_rightshift_assign(FunctionCall *c)
{
  c->arg(0).impl()->set_char(c->arg(0).toChar() >> c->arg(1).toChar());
  return c->arg(0);
}

// const char operator+(const char & a, const char & b)
Value char_add(FunctionCall *c)
{
  return c->engine()->newChar(c->arg(0).toChar() + c->arg(1).toChar());
}

// const char operator-(const char & a, const char & b)
Value char_sub(FunctionCall *c)
{
  return c->engine()->newChar(c->arg(0).toChar() - c->arg(1).toChar());
}

// const char operator*(const char & a, const char & b)
Value char_mul(FunctionCall *c)
{
  return c->engine()->newChar(c->arg(0).toChar() * c->arg(1).toChar());
}

// const char operator/(const char & a, const char & b)
Value char_div(FunctionCall *c)
{
  return c->engine()->newChar(c->arg(0).toChar() / c->arg(1).toChar());
}

// const char operator%(const char & a, const char & b)
Value char_mod(FunctionCall *c)
{
  return c->engine()->newChar(c->arg(0).toChar() % c->arg(1).toChar());
}

// const bool operator==(const char & a, const char & b);
Value char_equal(FunctionCall *c)
{
  return c->engine()->newBool(c->arg(0).toChar() == c->arg(1).toChar());
}

// const bool operator!=(const char & a, const char & b);
Value char_inequal(FunctionCall *c)
{
  return c->engine()->newBool(c->arg(0).toChar() != c->arg(1).toChar());
}

// const bool operator>(const char & a, const char & b);
Value char_greater(FunctionCall *c)
{
  return c->engine()->newBool(c->arg(0).toChar() > c->arg(1).toChar());
}

// const bool operator<(const char & a, const char & b);
Value char_less(FunctionCall *c)
{
  return c->engine()->newBool(c->arg(0).toChar() < c->arg(1).toChar());
}

// const bool operator>=(const char & a, const char & b);
Value char_geq(FunctionCall *c)
{
  return c->engine()->newBool(c->arg(0).toChar() >= c->arg(1).toChar());
}

// const bool operator<=(const char & a, const char & b);
Value char_leq(FunctionCall *c)
{
  return c->engine()->newBool(c->arg(0).toChar() <= c->arg(1).toChar());
}

// const char operator<<(const char & a, const char & b);
Value char_shiftleft(FunctionCall *c)
{
  return c->engine()->newChar(c->arg(0).toChar() << c->arg(1).toChar());
}

Value char_shiftright(FunctionCall *c)
{
  return c->engine()->newChar(c->arg(0).toChar() >> c->arg(1).toChar());
}

// char & operator++(char & a);
Value char_preincrement(FunctionCall *c)
{
  c->arg(0).impl()->set_char(c->arg(0).toChar() + 1);
  return c->arg(0);
}

// char & operator--(char & a);
Value char_predecrement(FunctionCall *c)
{
  c->arg(0).impl()->set_char(c->arg(0).toChar() - 1);
  return c->arg(0);
}

// const char operator++(char & a, char);
Value char_postincrement(FunctionCall *c)
{
  Value ret = c->engine()->newChar(c->arg(0).toChar());
  c->arg(0).impl()->set_char(c->arg(0).toChar() + 1);
  return ret;
}

// const char operator--(char & a, char);
Value char_postdecrement(FunctionCall *c)
{
  Value ret = c->engine()->newChar(c->arg(0).toChar());
  c->arg(0).impl()->set_char(c->arg(0).toChar() - 1);
  return ret;
}

// const char operator+(const char & a);
Value char_unary_plus(FunctionCall *c)
{
  return c->engine()->newChar(c->arg(0).toChar());
}

// const char operator-(const char & a);
Value char_unary_minus(FunctionCall *c)
{
  return c->engine()->newChar(-1 * c->arg(0).toChar());
}

// const char operator&(const char & a, const char & b);
Value char_bitand(FunctionCall *c)
{
  return c->engine()->newChar(c->arg(0).toChar() & c->arg(1).toChar());
}

// const char operator|(const char & a, const char & b);
Value char_bitor(FunctionCall *c)
{
  return c->engine()->newChar(c->arg(0).toChar() | c->arg(1).toChar());
}

// const char operator^(const char & a, const char & b);
Value char_bitxor(FunctionCall *c)
{
  return c->engine()->newChar(c->arg(0).toChar() ^ c->arg(1).toChar());
}

// const char operator~(const char & a);
Value char_bitnot(FunctionCall *c)
{
  return c->engine()->newChar(~c->arg(0).toChar());
}

// char & operator&=(char & a, const char & b);
Value char_bitand_assign(FunctionCall *c)
{
  c->arg(0).impl()->set_char(c->arg(0).toChar() & c->arg(1).toChar());
  return c->arg(0);
}

// char & operator|=(char & a, const char & b);
Value char_bitor_assign(FunctionCall *c)
{
  c->arg(0).impl()->set_char(c->arg(0).toChar() | c->arg(1).toChar());
  return c->arg(0);
}

// char & operator^=(char & a, const char & b);
Value char_bitxor_assign(FunctionCall *c)
{
  c->arg(0).impl()->set_char(c->arg(0).toChar() ^ c->arg(1).toChar());
  return c->arg(0);
}


/////////////////////////////// int //////////////////////////////////


// int & operator=(int & a, const int & b)
Value int_assign(FunctionCall *c)
{
  c->arg(0).impl()->set_int(c->arg(1).toInt());
  return c->arg(0);
}

// int & operator+=(int & a, const int & b)
Value int_add_assign(FunctionCall *c)
{
  c->arg(0).impl()->set_int(c->arg(0).toInt() + c->arg(1).toInt());
  return c->arg(0);
}

// int & operator-=(int & a, const int & b)
Value int_sub_assign(FunctionCall *c)
{
  c->arg(0).impl()->set_int(c->arg(0).toInt() - c->arg(1).toInt());
  return c->arg(0);
}

// int & operator*=(int & a, const int & b)
Value int_mul_assign(FunctionCall *c)
{
  c->arg(0).impl()->set_int(c->arg(0).toInt() * c->arg(1).toInt());
  return c->arg(0);
}

// int & operator/=(int & a, const int & b)
Value int_div_assign(FunctionCall *c)
{
  c->arg(0).impl()->set_int(c->arg(0).toInt() / c->arg(1).toInt());
  return c->arg(0);
}

// int & operator%=(int & a, const int & b)
Value int_mod_assign(FunctionCall *c)
{
  c->arg(0).impl()->set_int(c->arg(0).toInt() + c->arg(1).toInt());
  return c->arg(0);
}

// int & operator<<=(int & a, const int & b)
Value int_leftshift_assign(FunctionCall *c)
{
  c->arg(0).impl()->set_int(c->arg(0).toInt() << c->arg(1).toInt());
  return c->arg(0);
}

// int & operator>>=(int & a, const int & b)
Value int_rightshift_assign(FunctionCall *c)
{
  c->arg(0).impl()->set_int(c->arg(0).toInt() >> c->arg(1).toInt());
  return c->arg(0);
}

// const int operator+(const int & a, const int & b)
Value int_add(FunctionCall *c)
{
  return c->engine()->newInt(c->arg(0).toInt() + c->arg(1).toInt());
}

// const int operator-(const int & a, const int & b)
Value int_sub(FunctionCall *c)
{
  return c->engine()->newInt(c->arg(0).toInt() - c->arg(1).toInt());
}

// const int operator*(const int & a, const int & b)
Value int_mul(FunctionCall *c)
{
  return c->engine()->newInt(c->arg(0).toInt() * c->arg(1).toInt());
}

// const int operator/(const int & a, const int & b)
Value int_div(FunctionCall *c)
{
  return c->engine()->newInt(c->arg(0).toInt() / c->arg(1).toInt());
}

// const int operator%(const int & a, const int & b)
Value int_mod(FunctionCall *c)
{
  return c->engine()->newInt(c->arg(0).toInt() % c->arg(1).toInt());
}

// const bool operator==(const int & a, const int & b);
Value int_equal(FunctionCall *c)
{
  return c->engine()->newBool(c->arg(0).toInt() == c->arg(1).toInt());
}

// const bool operator!=(const int & a, const int & b);
Value int_inequal(FunctionCall *c)
{
  return c->engine()->newBool(c->arg(0).toInt() != c->arg(1).toInt());
}

// const bool operator>(const int & a, const int & b);
Value int_greater(FunctionCall *c)
{
  return c->engine()->newBool(c->arg(0).toInt() > c->arg(1).toInt());
}

// const bool operator<(const int & a, const int & b);
Value int_less(FunctionCall *c)
{
  return c->engine()->newBool(c->arg(0).toInt() < c->arg(1).toInt());
}

// const bool operator>=(const int & a, const int & b);
Value int_geq(FunctionCall *c)
{
  return c->engine()->newBool(c->arg(0).toInt() >= c->arg(1).toInt());
}

// const bool operator<=(const int & a, const int & b);
Value int_leq(FunctionCall *c)
{
  return c->engine()->newBool(c->arg(0).toInt() <= c->arg(1).toInt());
}

// const int operator<<(const int & a, const int & b);
Value int_shiftleft(FunctionCall *c)
{
  return c->engine()->newInt(c->arg(0).toInt() << c->arg(1).toInt());
}

Value int_shiftright(FunctionCall *c)
{
  return c->engine()->newInt(c->arg(0).toInt() >> c->arg(1).toInt());
}

// int & operator++(int & a);
Value int_preincrement(FunctionCall *c)
{
  c->arg(0).impl()->set_int(c->arg(0).toInt() + 1);
  return c->arg(0);
}

// int & operator--(int & a);
Value int_predecrement(FunctionCall *c)
{
  c->arg(0).impl()->set_int(c->arg(0).toInt() - 1);
  return c->arg(0);
}

// const int operator++(int & a, int);
Value int_postincrement(FunctionCall *c)
{
  Value ret = c->engine()->newInt(c->arg(0).toInt());
  c->arg(0).impl()->set_int(c->arg(0).toInt() + 1);
  return ret;
}

// const int operator--(int & a, int);
Value int_postdecrement(FunctionCall *c)
{
  Value ret = c->engine()->newInt(c->arg(0).toInt());
  c->arg(0).impl()->set_int(c->arg(0).toInt() - 1);
  return ret;
}

// const int operator+(const int & a);
Value int_unary_plus(FunctionCall *c)
{
  return c->engine()->newInt(c->arg(0).toInt());
}

// const int operator-(const int & a);
Value int_unary_minus(FunctionCall *c)
{
  return c->engine()->newInt(-1 * c->arg(0).toInt());
}

// const int operator&(const int & a, const int & b);
Value int_bitand(FunctionCall *c)
{
  return c->engine()->newInt(c->arg(0).toInt() & c->arg(1).toInt());
}

// const int operator|(const int & a, const int & b);
Value int_bitor(FunctionCall *c)
{
  return c->engine()->newInt(c->arg(0).toInt() | c->arg(1).toInt());
}

// const int operator^(const int & a, const int & b);
Value int_bitxor(FunctionCall *c)
{
  return c->engine()->newInt(c->arg(0).toInt() ^ c->arg(1).toInt());
}

// const int operator~(const int & a);
Value int_bitnot(FunctionCall *c)
{
  return c->engine()->newInt(~c->arg(0).toInt());
}


// int & operator&=(int & a, const int & b);
Value int_bitand_assign(FunctionCall *c)
{
  c->arg(0).impl()->set_int(c->arg(0).toInt() & c->arg(1).toInt());
  return c->arg(0);
}

// int & operator|=(int & a, const int & b);
Value int_bitor_assign(FunctionCall *c)
{
  c->arg(0).impl()->set_int(c->arg(0).toInt() | c->arg(1).toInt());
  return c->arg(0);
}

// int & operator^=(int & a, const int & b);
Value int_bitxor_assign(FunctionCall *c)
{
  c->arg(0).impl()->set_int(c->arg(0).toInt() ^ c->arg(1).toInt());
  return c->arg(0);
}


/////////////////////////////// float //////////////////////////////////


// float & operator=(float & a, const float & b)
Value float_assign(FunctionCall *c)
{
  c->arg(0).impl()->set_float(c->arg(1).toFloat());
  return c->arg(0);
}

// float & operator+=(float & a, const float & b)
Value float_add_assign(FunctionCall *c)
{
  c->arg(0).impl()->set_float(c->arg(0).toFloat() + c->arg(1).toFloat());
  return c->arg(0);
}

// float & operator-=(float & a, const float & b)
Value float_sub_assign(FunctionCall *c)
{
  c->arg(0).impl()->set_float(c->arg(0).toFloat() - c->arg(1).toFloat());
  return c->arg(0);
}

// float & operator*=(float & a, const float & b)
Value float_mul_assign(FunctionCall *c)
{
  c->arg(0).impl()->set_float(c->arg(0).toFloat() * c->arg(1).toFloat());
  return c->arg(0);
}

// float & operator/=(float & a, const float & b)
Value float_div_assign(FunctionCall *c)
{
  c->arg(0).impl()->set_float(c->arg(0).toFloat() / c->arg(1).toFloat());
  return c->arg(0);
}

// float & operator%=(float & a, const float & b)
Value float_mod_assign(FunctionCall *c)
{
  c->arg(0).impl()->set_float(c->arg(0).toFloat() + c->arg(1).toFloat());
  return c->arg(0);
}

// const float operator+(const float & a, const float & b)
Value float_add(FunctionCall *c)
{
  return c->engine()->newFloat(c->arg(0).toFloat() + c->arg(1).toFloat());
}

// const float operator-(const float & a, const float & b)
Value float_sub(FunctionCall *c)
{
  return c->engine()->newFloat(c->arg(0).toFloat() - c->arg(1).toFloat());
}

// const float operator*(const float & a, const float & b)
Value float_mul(FunctionCall *c)
{
  return c->engine()->newFloat(c->arg(0).toFloat() * c->arg(1).toFloat());
}

// const float operator/(const float & a, const float & b)
Value float_div(FunctionCall *c)
{
  return c->engine()->newFloat(c->arg(0).toFloat() / c->arg(1).toFloat());
}

// const bool operator==(const float & a, const float & b);
Value float_equal(FunctionCall *c)
{
  return c->engine()->newBool(c->arg(0).toFloat() == c->arg(1).toFloat());
}

// const bool operator!=(const float & a, const float & b);
Value float_inequal(FunctionCall *c)
{
  return c->engine()->newBool(c->arg(0).toFloat() != c->arg(1).toFloat());
}

// const bool operator>(const float & a, const float & b);
Value float_greater(FunctionCall *c)
{
  return c->engine()->newBool(c->arg(0).toFloat() > c->arg(1).toFloat());
}

// const bool operator<(const float & a, const float & b);
Value float_less(FunctionCall *c)
{
  return c->engine()->newBool(c->arg(0).toFloat() < c->arg(1).toFloat());
}

// const bool operator>=(const float & a, const float & b);
Value float_geq(FunctionCall *c)
{
  return c->engine()->newBool(c->arg(0).toFloat() >= c->arg(1).toFloat());
}

// const bool operator<=(const float & a, const float & b);
Value float_leq(FunctionCall *c)
{
  return c->engine()->newBool(c->arg(0).toFloat() <= c->arg(1).toFloat());
}

// float & operator++(float & a);
Value float_preincrement(FunctionCall *c)
{
  c->arg(0).impl()->set_float(c->arg(0).toFloat() + 1);
  return c->arg(0);
}

// float & operator--(float & a);
Value float_predecrement(FunctionCall *c)
{
  c->arg(0).impl()->set_float(c->arg(0).toFloat() - 1);
  return c->arg(0);
}

// const float operator++(float & a, float);
Value float_postincrement(FunctionCall *c)
{
  Value ret = c->engine()->newFloat(c->arg(0).toFloat());
  c->arg(0).impl()->set_float(c->arg(0).toFloat() + 1);
  return ret;
}

// const float operator--(float & a, float);
Value float_postdecrement(FunctionCall *c)
{
  Value ret = c->engine()->newFloat(c->arg(0).toFloat());
  c->arg(0).impl()->set_float(c->arg(0).toFloat() - 1);
  return ret;
}

// const float operator+(const float & a);
Value float_unary_plus(FunctionCall *c)
{
  return c->engine()->newFloat(c->arg(0).toFloat());
}

// const float operator-(const float & a);
Value float_unary_minus(FunctionCall *c)
{
  return c->engine()->newFloat(-1 * c->arg(0).toFloat());
}



/////////////////////////////// double //////////////////////////////////


// double & operator=(double & a, const double & b)
Value double_assign(FunctionCall *c)
{
  c->arg(0).impl()->set_double(c->arg(1).toDouble());
  return c->arg(0);
}

// double & operator+=(double & a, const double & b)
Value double_add_assign(FunctionCall *c)
{
  c->arg(0).impl()->set_double(c->arg(0).toDouble() + c->arg(1).toDouble());
  return c->arg(0);
}

// double & operator-=(double & a, const double & b)
Value double_sub_assign(FunctionCall *c)
{
  c->arg(0).impl()->set_double(c->arg(0).toDouble() - c->arg(1).toDouble());
  return c->arg(0);
}

// double & operator*=(double & a, const double & b)
Value double_mul_assign(FunctionCall *c)
{
  c->arg(0).impl()->set_double(c->arg(0).toDouble() * c->arg(1).toDouble());
  return c->arg(0);
}

// double & operator/=(double & a, const double & b)
Value double_div_assign(FunctionCall *c)
{
  c->arg(0).impl()->set_double(c->arg(0).toDouble() / c->arg(1).toDouble());
  return c->arg(0);
}

// double & operator%=(double & a, const double & b)
Value double_mod_assign(FunctionCall *c)
{
  c->arg(0).impl()->set_double(c->arg(0).toDouble() + c->arg(1).toDouble());
  return c->arg(0);
}

// const double operator+(const double & a, const double & b)
Value double_add(FunctionCall *c)
{
  return c->engine()->newDouble(c->arg(0).toDouble() + c->arg(1).toDouble());
}

// const double operator-(const double & a, const double & b)
Value double_sub(FunctionCall *c)
{
  return c->engine()->newDouble(c->arg(0).toDouble() - c->arg(1).toDouble());
}

// const double operator*(const double & a, const double & b)
Value double_mul(FunctionCall *c)
{
  return c->engine()->newDouble(c->arg(0).toDouble() * c->arg(1).toDouble());
}

// const double operator/(const double & a, const double & b)
Value double_div(FunctionCall *c)
{
  return c->engine()->newDouble(c->arg(0).toDouble() / c->arg(1).toDouble());
}

// const bool operator==(const double & a, const double & b);
Value double_equal(FunctionCall *c)
{
  return c->engine()->newBool(c->arg(0).toDouble() == c->arg(1).toDouble());
}

// const bool operator!=(const double & a, const double & b);
Value double_inequal(FunctionCall *c)
{
  return c->engine()->newBool(c->arg(0).toDouble() != c->arg(1).toDouble());
}

// const bool operator>(const double & a, const double & b);
Value double_greater(FunctionCall *c)
{
  return c->engine()->newBool(c->arg(0).toDouble() > c->arg(1).toDouble());
}

// const bool operator<(const double & a, const double & b);
Value double_less(FunctionCall *c)
{
  return c->engine()->newBool(c->arg(0).toDouble() < c->arg(1).toDouble());
}

// const bool operator>=(const double & a, const double & b);
Value double_geq(FunctionCall *c)
{
  return c->engine()->newBool(c->arg(0).toDouble() >= c->arg(1).toDouble());
}

// const bool operator<=(const double & a, const double & b);
Value double_leq(FunctionCall *c)
{
  return c->engine()->newBool(c->arg(0).toDouble() <= c->arg(1).toDouble());
}

// double & operator++(double & a);
Value double_preincrement(FunctionCall *c)
{
  c->arg(0).impl()->set_double(c->arg(0).toDouble() + 1);
  return c->arg(0);
}

// double & operator--(double & a);
Value double_predecrement(FunctionCall *c)
{
  c->arg(0).impl()->set_double(c->arg(0).toDouble() - 1);
  return c->arg(0);
}

// const double operator++(double & a, double);
Value double_postincrement(FunctionCall *c)
{
  Value ret = c->engine()->newDouble(c->arg(0).toDouble());
  c->arg(0).impl()->set_double(c->arg(0).toDouble() + 1);
  return ret;
}

// const double operator--(double & a, double);
Value double_postdecrement(FunctionCall *c)
{
  Value ret = c->engine()->newDouble(c->arg(0).toDouble());
  c->arg(0).impl()->set_double(c->arg(0).toDouble() - 1);
  return ret;
}

// const double operator+(const double & a);
Value double_unary_plus(FunctionCall *c)
{
  return c->engine()->newDouble(c->arg(0).toDouble());
}

// const double operator-(const double & a);
Value double_unary_minus(FunctionCall *c)
{
  return c->engine()->newDouble(-1 * c->arg(0).toDouble());
}


} // namespace operators


namespace conversions
{

Value char_to_bool(FunctionCall *c)
{
  return c->engine()->newBool(static_cast<bool>(c->arg(0).toChar()));
}

Value int_to_bool(FunctionCall *c)
{
  return c->engine()->newBool(static_cast<bool>(c->arg(0).toInt()));
}

Value int_to_float(FunctionCall *c)
{
  return c->engine()->newFloat(static_cast<float>(c->arg(0).toInt()));
}

Value int_to_double(FunctionCall *c)
{
  return c->engine()->newDouble(static_cast<double>(c->arg(0).toInt()));
}

Value float_to_int(FunctionCall *c)
{
  return c->engine()->newInt(static_cast<int>(c->arg(0).toFloat()));
}

Value float_to_double(FunctionCall *c)
{
  return c->engine()->newDouble(static_cast<double>(c->arg(0).toFloat()));
}

Value double_to_int(FunctionCall *c)
{
  return c->engine()->newInt(static_cast<int>(c->arg(0).toDouble()));
}

Value double_to_float(FunctionCall *c)
{
  return c->engine()->newFloat(static_cast<float>(c->arg(0).toDouble()));
}

} // namespace conversions


} // namespace callbacks




template<typename T>
struct script_base_type;

template<> struct script_base_type<bool> : public std::integral_constant<int, Type::Boolean> {};
template<> struct script_base_type<char> : public std::integral_constant<int, Type::Char> {};
template<> struct script_base_type<int> : public std::integral_constant<int, Type::Int> {};
template<> struct script_base_type<float> : public std::integral_constant<int, Type::Float> {};
template<> struct script_base_type<double> : public std::integral_constant<int, Type::Double> {};

template<typename T>
Type script_type()
{
  Type ret{ script_base_type<typename std::decay<T>::type>::value };
  if (std::is_reference<T>::value)
    ret = ret.withFlag(Type::ReferenceFlag);
  if (std::is_const<typename std::remove_reference<T>::type>::value)
    ret = ret.withFlag(Type::ConstFlag);
  return ret;
}

template<typename...Args>
Prototype proto()
{
  return Prototype{ script_type<Args>()... };
}


void register_builtin_operators(Namespace root)
{
  using namespace callbacks;
  using namespace operators;

  /// TODO: do not use addOperator()

  struct OperatorBuilder
  {
    Engine *engine;
    OperatorName operation;

    inline Operator operator()(const Prototype & p, NativeFunctionSignature impl)
    {
      auto ret = std::make_shared<BuiltInOperatorImpl>(operation, p, engine);
      ret->implementation.callback = impl;
      return Operator{ ret };
    }
  };

  OperatorBuilder gen{ root.engine(), PreIncrementOperator};
  root.addOperator(gen(proto<char&, char&>(), char_preincrement));
  root.addOperator(gen(proto<int&, int&>(), int_preincrement));
  root.addOperator(gen(proto<float&, float&>(), float_preincrement));
  root.addOperator(gen(proto<double&, double&>(), double_preincrement));

  gen.operation = PreDecrementOperator;
  root.addOperator(gen(proto<char&, char&>(), char_predecrement));
  root.addOperator(gen(proto<int&, int&>(), int_predecrement));
  root.addOperator(gen(proto<float&, float&>(), float_predecrement));
  root.addOperator(gen(proto<double&, double&>(), double_predecrement));

  gen.operation = PostIncrementOperator;
  root.addOperator(gen(proto<char, char&>(), char_postincrement));
  root.addOperator(gen(proto<int, int&>(), int_postincrement));
  root.addOperator(gen(proto<float, float&>(), float_postincrement));
  root.addOperator(gen(proto<double, double&>(), double_postincrement));

  gen.operation = PostDecrementOperator;
  root.addOperator(gen(proto<char, char&>(), char_postdecrement));
  root.addOperator(gen(proto<int, int&>(), int_postdecrement));
  root.addOperator(gen(proto<float, float&>(), float_postdecrement));
  root.addOperator(gen(proto<double, double&>(), double_postdecrement));

  gen.operation = UnaryPlusOperator;
  root.addOperator(gen(proto<char, const char &>(), char_unary_plus));
  root.addOperator(gen(proto<int, const int &>(), int_unary_plus));
  root.addOperator(gen(proto<float, const float &>(), float_unary_plus));
  root.addOperator(gen(proto<double, const double &>(), double_unary_plus));

  gen.operation = UnaryMinusOperator;
  root.addOperator(gen(proto<char, const char &>(), char_unary_minus));
  root.addOperator(gen(proto<int, const int &>(), int_unary_minus));
  root.addOperator(gen(proto<float, const float &>(), float_unary_minus));
  root.addOperator(gen(proto<double, const double &>(), double_unary_minus));

  gen.operation = AssignmentOperator;
  root.addOperator(gen(proto<bool&, bool&, const bool&>(), bool_assign));
  root.addOperator(gen(proto<char&, char&, const char&>(), char_assign));
  root.addOperator(gen(proto<int&, int&, const int&>(), int_assign));
  root.addOperator(gen(proto<float&, float&, const float&>(), float_assign));
  root.addOperator(gen(proto<double&, double&, const double&>(), double_assign));

  gen.operation = EqualOperator;
  root.addOperator(gen(proto<bool, const bool&, const bool&>(), bool_equal));
  root.addOperator(gen(proto<bool, const char&, const char&>(), char_equal));
  root.addOperator(gen(proto<bool, const int&, const int&>(), int_equal));
  root.addOperator(gen(proto<bool, const float&, const float&>(), float_equal));
  root.addOperator(gen(proto<bool, const double&, const double&>(), double_equal));

  gen.operation = InequalOperator;
  root.addOperator(gen(proto<bool, const bool&, const bool&>(), bool_inequal));
  root.addOperator(gen(proto<bool, const char&, const char&>(), char_inequal));
  root.addOperator(gen(proto<bool, const int&, const int&>(), int_inequal));
  root.addOperator(gen(proto<bool, const float&, const float&>(), float_inequal));
  root.addOperator(gen(proto<bool, const double&, const double&>(), double_inequal));

  gen.operation = LessOperator;
  root.addOperator(gen(proto<bool, const char&, const char&>(), char_less));
  root.addOperator(gen(proto<bool, const int&, const int&>(), int_less));
  root.addOperator(gen(proto<bool, const float&, const float&>(), float_less));
  root.addOperator(gen(proto<bool, const double&, const double&>(), double_less));

  gen.operation = GreaterOperator;
  root.addOperator(gen(proto<bool, const char&, const char&>(), char_greater));
  root.addOperator(gen(proto<bool, const int&, const int&>(), int_greater));
  root.addOperator(gen(proto<bool, const float&, const float&>(), float_greater));
  root.addOperator(gen(proto<bool, const double&, const double&>(), double_greater));

  gen.operation = LessEqualOperator;
  root.addOperator(gen(proto<bool, const char&, const char&>(), char_leq));
  root.addOperator(gen(proto<bool, const int&, const int&>(), int_leq));
  root.addOperator(gen(proto<bool, const float&, const float&>(), float_leq));
  root.addOperator(gen(proto<bool, const double&, const double&>(), double_leq));

  gen.operation = GreaterEqualOperator;
  root.addOperator(gen(proto<bool, const char&, const char&>(), char_geq));
  root.addOperator(gen(proto<bool, const int&, const int&>(), int_geq));
  root.addOperator(gen(proto<bool, const float&, const float&>(), float_geq));
  root.addOperator(gen(proto<bool, const double&, const double&>(), double_geq));

  gen.operation = AdditionOperator;
  root.addOperator(gen(proto<char, const char&, const char&>(), char_add));
  root.addOperator(gen(proto<int, const int&, const int&>(), int_add));
  root.addOperator(gen(proto<float, const float&, const float&>(), float_add));
  root.addOperator(gen(proto<double, const double&, const double&>(), double_add));

  gen.operation = SubstractionOperator;
  root.addOperator(gen(proto<char, const char&, const char&>(), char_sub));
  root.addOperator(gen(proto<int, const int&, const int&>(), int_sub));
  root.addOperator(gen(proto<float, const float&, const float&>(), float_sub));
  root.addOperator(gen(proto<double, const double&, const double&>(), double_sub));

  gen.operation = MultiplicationOperator;
  root.addOperator(gen(proto<char, const char&, const char&>(), char_mul));
  root.addOperator(gen(proto<int, const int&, const int&>(), int_mul));
  root.addOperator(gen(proto<float, const float&, const float&>(), float_mul));
  root.addOperator(gen(proto<double, const double&, const double&>(), double_mul));

  gen.operation = DivisionOperator;
  root.addOperator(gen(proto<char, const char&, const char&>(), char_div));
  root.addOperator(gen(proto<int, const int&, const int&>(), int_div));
  root.addOperator(gen(proto<float, const float&, const float&>(), float_div));
  root.addOperator(gen(proto<double, const double&, const double&>(), double_div));

  gen.operation = RemainderOperator;
  root.addOperator(gen(proto<char, const char&, const char&>(), char_mod));
  root.addOperator(gen(proto<int, const int&, const int&>(), int_mod));

  gen.operation = AdditionAssignmentOperator;
  root.addOperator(gen(proto<char&, char&, const char&>(), char_add_assign));
  root.addOperator(gen(proto<int&, int&, const int&>(), int_add_assign));
  root.addOperator(gen(proto<float&, float&, const float&>(), float_add_assign));
  root.addOperator(gen(proto<double&, double&, const double&>(), double_add_assign));

  gen.operation = SubstractionAssignmentOperator;
  root.addOperator(gen(proto<char&, char&, const char&>(), char_sub_assign));
  root.addOperator(gen(proto<int&, int&, const int&>(), int_sub_assign));
  root.addOperator(gen(proto<float&, float&, const float&>(), float_sub_assign));
  root.addOperator(gen(proto<double&, double&, const double&>(), double_sub_assign));

  gen.operation = MultiplicationAssignmentOperator;
  root.addOperator(gen(proto<char&, char&, const char&>(), char_mul_assign));
  root.addOperator(gen(proto<int&, int&, const int&>(), int_mul_assign));
  root.addOperator(gen(proto<float&, float&, const float&>(), float_mul_assign));
  root.addOperator(gen(proto<double&, double&, const double&>(), double_mul_assign));

  gen.operation = DivisionAssignmentOperator;
  root.addOperator(gen(proto<char&, char&, const char&>(), char_div_assign));
  root.addOperator(gen(proto<int&, int&, const int&>(), int_div_assign));
  root.addOperator(gen(proto<float&, float&, const float&>(), float_div_assign));
  root.addOperator(gen(proto<double&, double&, const double&>(), double_div_assign));

  gen.operation = RemainderAssignmentOperator;
  root.addOperator(gen(proto<char&, char&, const char&>(), char_mod_assign));
  root.addOperator(gen(proto<int&, int&, const int&>(), int_mod_assign));

  gen.operation = LeftShiftAssignmentOperator;
  root.addOperator(gen(proto<char&, char&, const char&>(), char_leftshift_assign));
  root.addOperator(gen(proto<int&, int&, const int&>(), int_leftshift_assign));

  gen.operation = RightShiftAssignmentOperator;
  root.addOperator(gen(proto<char&, char&, const char&>(), char_rightshift_assign));
  root.addOperator(gen(proto<int&, int&, const int&>(), int_rightshift_assign));

  gen.operation = LeftShiftOperator;
  root.addOperator(gen(proto<char, const char&, const char&>(), char_shiftleft));
  root.addOperator(gen(proto<int,  const int&, const int&>(), int_shiftleft));

  gen.operation = RightShiftOperator;
  root.addOperator(gen(proto<char, const char&, const char&>(), char_shiftright));
  root.addOperator(gen(proto<int, const int&, const int&>(), int_shiftright));

  gen.operation = LogicalNotOperator;
  root.addOperator(gen(proto<bool, const bool&>(), bool_negate));

  gen.operation = LogicalAndOperator;
  root.addOperator(gen(proto<bool, const bool &, const bool&>(), bool_logical_and));

  gen.operation = LogicalOrOperator;
  root.addOperator(gen(proto<bool, const bool &, const bool&>(), bool_logical_or));

  gen.operation = BitwiseAndOperator;
  root.addOperator(gen(proto<char, const char &, const char&>(), char_bitand));
  root.addOperator(gen(proto<int, const int &, const int&>(), int_bitand));

  gen.operation = BitwiseOrOperator;
  root.addOperator(gen(proto<char, const char &, const char&>(), char_bitor));
  root.addOperator(gen(proto<int, const int &, const int&>(), int_bitor));

  gen.operation = BitwiseXorOperator;
  root.addOperator(gen(proto<char, const char &, const char&>(), char_bitxor));
  root.addOperator(gen(proto<int, const int &, const int&>(), int_bitxor));

  gen.operation = BitwiseNot;
  root.addOperator(gen(proto<char, const char &, const char&>(), char_bitnot));
  root.addOperator(gen(proto<int, const int &, const int&>(), int_bitnot));

  gen.operation = BitwiseAndAssignmentOperator;
  root.addOperator(gen(proto<char&, char &, const char&>(), char_bitand_assign));
  root.addOperator(gen(proto<int&, int &, const int&>(), int_bitand_assign));

  gen.operation = BitwiseOrAssignmentOperator;
  root.addOperator(gen(proto<char&, char &, const char&>(), char_bitor_assign));
  root.addOperator(gen(proto<int&, int &, const int&>(), int_bitor_assign));

  gen.operation = BitwiseXorAssignmentOperator;
  root.addOperator(gen(proto<char&, char &, const char&>(), char_bitxor_assign));
  root.addOperator(gen(proto<int&, int &, const int&>(), int_bitxor_assign));
}

} // namespace script
