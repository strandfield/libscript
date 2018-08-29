// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/conversions.h"

#include "script/engine.h"
#include "script/cast.h"
#include "script/class.h"
#include "script/templateargument.h"

#include "script/program/expression.h"

namespace script
{

static StandardConversion same_type_conversion(const Type & src, const Type & dest, Engine *engine)
{
  if (dest.isReference())
  {
    if (dest.isConst())
    {
      if (!src.isConst())
        return StandardConversion::ReferenceInitialization().with(ConstQualification);
      else
        return StandardConversion::ReferenceInitialization();
    }

    /// TODO : is this correct ? I believe no
    //if (!(src.isReference() || src.isRefRef()))
    //  return StandardConversion::NotConvertible();
    //else if (src.isConst() && !dest.isConst())
    //  return StandardConversion::NotConvertible();
    /// should work better : a reference can be initialized from a non-reference 
    if (src.isConst() && !dest.isConst())
      return StandardConversion::NotConvertible();

    return StandardConversion::ReferenceInitialization();
  }
  else if (dest.isRefRef())
  {
    if (!src.isRefRef())
      return StandardConversion::NotConvertible();

    if (src.isConst() && !dest.isConst())
      return StandardConversion::NotConvertible();
    else if (dest.isConst() && !src.isConst())
      return StandardConversion::ReferenceInitialization().with(ConstQualification);

    return StandardConversion::ReferenceInitialization();
  }

  if (src.isFundamentalType() || src.isEnumType() || src.isClosureType() || src.isFunctionType())
    return StandardConversion{};
  else if (src.isObjectType())
  {
    Class c = engine->getClass(src);
    if (!c.isCopyConstructible())
      return StandardConversion::NotConvertible();
    return StandardConversion{};
  }

  return StandardConversion::NotConvertible();
}



static StandardConversion fundamental_conversion(const Type & srcType, const Type & destType, Engine *engine)
{
  static_assert(Type::Void == 1, "Fundamental types are expected to match a predefined numbering");
  static_assert(Type::Boolean == 2, "Fundamental types are expected to match a predefined numbering");
  static_assert(Type::Char == 3, "Fundamental types are expected to match a predefined numbering");
  static_assert(Type::Int == 4, "Fundamental types are expected to match a predefined numbering");
  static_assert(Type::Float == 5, "Fundamental types are expected to match a predefined numbering");
  static_assert(Type::Double == 6, "Fundamental types are expected to match a predefined numbering");

  const auto & ExactMatch = StandardConversion{};
  const auto & NotConvertible = StandardConversion::NotConvertible();

  // usage : ranks[src][dest]
  static const StandardConversion ranks[6][6] = {
    { ExactMatch, NotConvertible, NotConvertible, NotConvertible, NotConvertible, NotConvertible },
  { NotConvertible, ExactMatch, IntegralPromotion, IntegralPromotion, FloatingPointPromotion, FloatingPointPromotion },
  { NotConvertible, BooleanConversion, ExactMatch, IntegralPromotion, FloatingPointPromotion, FloatingPointPromotion },
  { NotConvertible, BooleanConversion, IntegralConversion, ExactMatch, FloatingPointPromotion, FloatingPointPromotion },
  { NotConvertible, BooleanConversion, IntegralConversion, IntegralConversion, ExactMatch, FloatingPointPromotion },
  { NotConvertible, BooleanConversion, IntegralConversion, IntegralConversion, FloatingPointConversion, ExactMatch },
  };

  assert(srcType.baseType() != destType.baseType()); // this case is handled before

  if (destType.isReference() && !destType.isConst()) // what about destType.isRefRef() ?
    return NotConvertible;

  const QualificationAdjustment constQual = destType.isConst() ? QualificationAdjustment::ConstQualification : QualificationAdjustment::NoQualificationAdjustment;
  auto ret = ranks[srcType.baseType().data() - 1][destType.baseType().data() - 1].with(constQual);
  if (destType.isConstRef())
    return StandardConversion::CopyRefInitialization(ret);
  return ret;
}

static ConversionSequence select_converting_constructor(const Type & src, const std::vector<Function> & ctors, const Type & dest, Engine *engine)
{
  if (dest.isReference() && !dest.isConst() && src.isConst())
    return ConversionSequence::NotConvertible();

  // We store the two best conversion sequences to detect ambiguity.
  StandardConversion best_conv = StandardConversion::NotConvertible();
  Function best_ctor;
  StandardConversion ambiguous_conv = StandardConversion::NotConvertible();

  for (const auto & c : ctors)
  {
    if (c.prototype().count() != 1)
      continue;

    if (c.isExplicit())
      continue;

    StandardConversion first_conversion = StandardConversion::compute(src, c.prototype().at(0), engine);
    if (first_conversion == StandardConversion::NotConvertible())
      continue;

    bool comp1 = first_conversion < best_conv;
    bool comp2 = best_conv < first_conversion;

    if (comp1 && !comp2)
    {
      best_conv = first_conversion;
      best_ctor = c;
      ambiguous_conv = StandardConversion::NotConvertible();
    } 
    else if (comp2 && !comp1)
    {
      // nothing to do
    }
    else
    {
      ambiguous_conv = first_conversion;
    }
  }

  if(!(best_conv < ambiguous_conv))
    return ConversionSequence::NotConvertible();

  /// TODO: correctly compute second conversion, which can be
  // - a const-qualification
  // - a derived to base conversion
  //StandardConversion second_conversion = StandardConversion::compute(Type::rref(dest.baseType()), dest, engine);
  /// TODO : not sure this is correct
  /*if (second_conversion == StandardConversion::NotConvertible())
  continue;*/
  //second_conversion = StandardConversion::None();

  return ConversionSequence{ best_conv, best_ctor, StandardConversion::None() };
}

static ConversionSequence select_cast(const Type & src, const std::vector<Cast> & casts, const Type & dest, Engine *engine)
{
  // TODO : before returning, check if better candidates can be found ?
  // this would result in an ambiguous conversion sequence I believe
  for (const auto & c : casts)
  {
    if (c.isExplicit())
      continue;

    StandardConversion first_conversion = StandardConversion::compute(src, c.sourceType(), engine);
    if (first_conversion == StandardConversion::NotConvertible())
      continue;

    StandardConversion second_conversion = StandardConversion::compute(c.destType(), dest, engine);
    if (second_conversion == StandardConversion::NotConvertible())
      continue;

    return ConversionSequence{ first_conversion, c, second_conversion };
  }

  return ConversionSequence::NotConvertible();
}

static ConversionSequence compute_initializer_list_conv(const std::shared_ptr<program::Expression> & expr, const Type & dest, Engine *engine)
{
  if (!expr->is<program::InitializerList>())
    return ConversionSequence::NotConvertible();
  const program::InitializerList & init_list = dynamic_cast<const program::InitializerList &>(*expr);


  Class initializer_list_type = engine->getClass(dest);
  Type T = initializer_list_type.arguments().front().type;

  std::vector<ConversionSequence> conversions;
  for (const auto & e : init_list.elements)
  {
    ConversionSequence conv = ConversionSequence::compute(e, T, engine);
    if (conv == ConversionSequence::NotConvertible())
      return conv;

    conversions.push_back(conv);
  }

  auto result = std::make_shared<ListInitializationSequence>(ListInitializationSequence::InitializerListCreation, initializer_list_type.id());
  result->conversions() = std::move(conversions);
  return ConversionSequence{ result };
}

static ConversionSequence compute_initializer_list_conv(const std::shared_ptr<program::Expression> & expr, const Function & ctor, Engine *engine)
{
  assert(expr->is<program::InitializerList>());
  assert(engine->isInitializerListType(ctor.parameter(0)));

  const program::InitializerList & init_list = dynamic_cast<const program::InitializerList &>(*expr);

  Class initializer_list_type = engine->getClass(ctor.parameter(0));
  Type T = initializer_list_type.arguments().front().type;

  std::vector<ConversionSequence> conversions;
  for (const auto & e : init_list.elements)
  {
    ConversionSequence conv = ConversionSequence::compute(e, T, engine);
    if (conv == ConversionSequence::NotConvertible())
      return conv;

    conversions.push_back(conv);
  }

  auto result = std::make_shared<ListInitializationSequence>(ListInitializationSequence::InitializerListInitialization, ctor);
  result->conversions() = std::move(conversions);
  return ConversionSequence{ result };
}

// The format used should allow easy comparison : 
// highest bit is set to 1 if no conversion is possible
// bit 0 is set to 1 for qualifcation adjustement
// bit 1 is used to indicate copy of one type to the same
// bit 2-3 are used to encode numeric promotion
// bit 4-6 are used to encode numeric conversion
// bit 8-16 are used to encode derived to base conversion

static int NumPromoMask = (3 << 1);
static int NumConvMask = (7 << 3);
static int DerivToBaseMask = (0xFF << DerivedToBaseConversion::Shift);

StandardConversion::StandardConversion()
  : d(static_cast<int>(CopyInitializationFlag))
{

}

StandardConversion::StandardConversion(NumericConversion conv, QualificationAdjustment qualadjust)
{
  d = static_cast<int>(CopyInitializationFlag) | static_cast<int>(conv) | static_cast<int>(qualadjust);
}

StandardConversion::StandardConversion(NumericPromotion promo, QualificationAdjustment qualadjust)
{
  d = static_cast<int>(CopyInitializationFlag) | static_cast<int>(promo) | static_cast<int>(qualadjust);
}

StandardConversion::StandardConversion(DerivedToBaseConversion dbconv, QualificationAdjustment qualadjust)
{
  d = static_cast<int>(CopyInitializationFlag) | static_cast<int>(dbconv) | static_cast<int>(qualadjust);

}

StandardConversion::StandardConversion(QualificationAdjustment qualadjust)
{
  d = static_cast<int>(qualadjust);
}


bool StandardConversion::isNone() const
{
  return d == 0;
}

StandardConversion StandardConversion::None()
{
  StandardConversion ret;
  ret.d = 0;
  return ret;
}

bool StandardConversion::isNarrowing() const
{
  return (d & NumConvMask) != 0;
}

StandardConversion::Rank StandardConversion::rank() const
{
  if ((d & NumConvMask) || (d & DerivToBaseMask))
    return Rank::Conversion;
  else if (d & NumPromoMask)
    return Rank::Promotion;
  else if (d & NotConvertibleFlag)
    return Rank::NotConvertible;
  return Rank::ExactMatch;
}

bool StandardConversion::isCopyInitialization() const
{
  return d & CopyInitializationFlag;
}

bool StandardConversion::isReferenceInitialization() const
{
  return d & ReferenceInitializationFlag;
}

bool StandardConversion::isNumericPromotion() const
{
  return d & NumPromoMask;
}

NumericPromotion StandardConversion::numericPromotion() const
{
  return static_cast<NumericPromotion>(d & NumPromoMask);
}


bool StandardConversion::isNumericConversion() const
{
  return d & NumConvMask;
}

NumericConversion StandardConversion::numericConversion() const
{
  return static_cast<NumericConversion>(d & NumConvMask);
}


bool StandardConversion::hasQualificationConversion() const
{
  return d & 1;
}

QualificationAdjustment StandardConversion::qualificationConversion()
{
  if (d & 1)
    return ConstQualification;
  return QualificationAdjustment::NoQualificationAdjustment;
}


bool StandardConversion::isDerivedToBaseConversion() const
{
  return (d & DerivToBaseMask);
}

int StandardConversion::derivedToBaseConversionDepth() const
{
  return (d & DerivToBaseMask) >> DerivedToBaseConversion::Shift;
}

StandardConversion StandardConversion::with(QualificationAdjustment adjust) const
{
  StandardConversion conv{ *this };
  conv.d |= static_cast<int>(adjust);
  return conv;
}


StandardConversion StandardConversion::NotConvertible()
{
  StandardConversion seq;
  seq.d = static_cast<int>(NotConvertibleFlag);
  return seq;
}

StandardConversion StandardConversion::ReferenceInitialization()
{
  StandardConversion seq;
  seq.d = static_cast<int>(ReferenceInitializationFlag);
  return seq;
}

StandardConversion StandardConversion::ReferenceInitialization(DerivedToBaseConversion dbconv, QualificationAdjustment qualadjust)
{
  StandardConversion seq;
  seq.d = static_cast<int>(ReferenceInitializationFlag) | static_cast<int>(dbconv) | static_cast<int>(qualadjust);
  return seq;
}

StandardConversion StandardConversion::ReferenceInitialization(QualificationAdjustment qualadjust)
{
  StandardConversion seq;
  seq.d = static_cast<int>(ReferenceInitializationFlag) | static_cast<int>(qualadjust);
  return seq;
}

StandardConversion StandardConversion::CopyRefInitialization(StandardConversion conv)
{
  if (conv.d & NotConvertibleFlag)
    return conv;
  conv.d |= static_cast<int>(CopyRefInitializationFlag);
  return conv;
}

StandardConversion StandardConversion::compute(const Type & src, const Type & dest, Engine *e)
{
  if (dest.isReference() && !dest.isConst() && src.isConst())
    return StandardConversion::NotConvertible();

  if (src.baseType() == dest.baseType())
    return same_type_conversion(src, dest, e);
  else if (src.isFundamentalType() && dest.isFundamentalType())
    return fundamental_conversion(src, dest, e);
  else if (src.isObjectType() && dest.isObjectType())
  {
    const Class src_class = e->getClass(src), dest_class = e->getClass(dest);
    const int inheritance_depth = src_class.inheritanceLevel(dest_class);
    if (inheritance_depth < 0)
      return StandardConversion::NotConvertible();

    const QualificationAdjustment adjust = (dest.isConst() && !src.isConst()) ? ConstQualification : NoQualificationAdjustment;

    if (dest.isReference())
      return StandardConversion::ReferenceInitialization(DerivedToBaseConversion{ inheritance_depth }, adjust);

    if (!dest_class.isCopyConstructible())
      return StandardConversion::NotConvertible();
    return StandardConversion{ DerivedToBaseConversion{inheritance_depth}, adjust };
  }
  else if (src.isEnumType() && dest.baseType() == Type::Int)
  {
    if (dest.isConstRef())
      return StandardConversion::CopyRefInitialization(StandardConversion{ IntegralConversion });

    if (dest.isReference())
      return StandardConversion::NotConvertible();

    return StandardConversion{ IntegralConversion };
  }
  else if (src.isFunctionType() && dest.isFunctionType())
  {
    /// TODO : check if dest type is const-compatible with src type
    // eg. int(int, int) can be converted to int(const int, const int)
    return StandardConversion::NotConvertible();
  }

  return StandardConversion::NotConvertible();
}

bool StandardConversion::operator==(const StandardConversion & other) const
{
  return d == other.d;
}

bool StandardConversion::operator<(const StandardConversion & other) const
{
  auto this_rank = rank();
  auto other_rank = other.rank();
  if (this_rank < other_rank)
    return true;
  else if (this_rank > other_rank)
    return false;

  if (other.isDerivedToBaseConversion() && this->isDerivedToBaseConversion())
  {
    if (this->derivedToBaseConversionDepth() < other.derivedToBaseConversionDepth())
      return true;
    else if (this->derivedToBaseConversionDepth() > other.derivedToBaseConversionDepth())
      return false;
  }

  if (this->isCopyInitialization() && !other.isCopyInitialization())
    return false;
  else if (!this->isCopyInitialization() && other.isCopyInitialization())
    return true;

  if (this->isReferenceInitialization() && other.isReferenceInitialization())
  {
    if (other.hasQualificationConversion() && !this->hasQualificationConversion())
      return true;
  }

  return false;
}


ConversionSequence::ConversionSequence(const StandardConversion & c1, const Function & userdefinedConversion, const StandardConversion & c2)
  : conv1(c1)
  , function(userdefinedConversion)
  , conv3(c2)
{

}

ConversionSequence::ConversionSequence(const std::shared_ptr<ListInitializationSequence> & listinit)
  : listInitialization(listinit)
{

}

ConversionSequence::Rank ConversionSequence::rank() const
{
  if (conv1 == StandardConversion::NotConvertible())
    return Rank::NotConvertible;

  if (!function.isNull())
    return Rank::UserDefinedConversion;

  return static_cast<ConversionSequence::Rank>(conv1.rank());
}

ConversionSequence::Rank ConversionSequence::globalRank(const std::vector<ConversionSequence> & convs)
{
  if (convs.empty())
    return Rank::ExactMatch;

  Rank r = convs.front().rank();
  for (size_t i(1); i < convs.size(); ++i)
  {
    Rank r2 = convs.at(i).rank();
    r = r2 > r ? r2 : r;
  }
  return r;
}

bool ConversionSequence::isInvalid() const
{
  return conv1.rank() == StandardConversion::Rank::NotConvertible;
}

bool ConversionSequence::isNarrowing() const
{
  return conv1.isNarrowing() || conv3.isNarrowing();
}

bool ConversionSequence::isUserDefinedConversion() const
{
  return !this->function.isNull();
}

bool ConversionSequence::isListInitialization() const
{
  return this->listInitialization != nullptr;
}

ConversionSequence ConversionSequence::NotConvertible()
{
  return ConversionSequence{ StandardConversion::NotConvertible() };
}

ConversionSequence ConversionSequence::compute(const Type & src, const Type & dest, Engine *engine)
{
  StandardConversion stdconv = StandardConversion::compute(src, dest, engine);
  if (stdconv != StandardConversion::NotConvertible())
    return stdconv;

  if (!src.isObjectType() && !dest.isObjectType())
    return ConversionSequence::NotConvertible();

  assert(src.isObjectType() || dest.isObjectType());

  if (dest.isObjectType())
  {
    const auto & ctors = engine->getClass(dest).constructors();
    ConversionSequence userdefconv = select_converting_constructor(src, ctors, dest, engine);
    if (userdefconv != ConversionSequence::NotConvertible())
      return userdefconv;
  }

  if (src.isObjectType())
  {
    const auto & casts = engine->getClass(src).casts();
    ConversionSequence userdefconv = select_cast(src, casts, dest, engine);
    if (userdefconv != ConversionSequence::NotConvertible())
      return userdefconv;
  }

  // TODO : implement other forms of conversion

  return ConversionSequence::NotConvertible();
}

ConversionSequence ConversionSequence::compute(const std::shared_ptr<program::Expression> & expr, const Type & dest, Engine *engine)
{
  if (expr->type() != Type::InitializerList)
    return compute(expr->type(), dest, engine);

  if (engine->isInitializerListType(dest))
    return compute_initializer_list_conv(expr, dest, engine);

  if (dest.isReference() && !dest.isConst())
    return ConversionSequence::NotConvertible();

  const program::InitializerList & init_list = dynamic_cast<const program::InitializerList &>(*expr);

  if (init_list.elements.empty())
  {
    if (dest.isFundamentalType())
      return ConversionSequence{ std::make_shared<ListInitializationSequence>(dest) };
    else if(dest.isObjectType() && engine->getClass(dest).isDefaultConstructible())
      return ConversionSequence{ std::make_shared<ListInitializationSequence>(dest) };
    else
      return ConversionSequence::NotConvertible();
  }

  if (!dest.isObjectType())
    return ConversionSequence::NotConvertible();

  const Class dest_class = engine->getClass(dest);

  std::vector<ConversionSequence> conversions;
  conversions.resize(init_list.elements.size());

  /// TODO: shouldn't we perform overload resolution instead ?
  for (const auto & c : dest_class.constructors())
  {
    if(c.prototype().count() == 1 && engine->isInitializerListType(c.parameter(0)))
      return compute_initializer_list_conv(expr, c, engine);

    if (c.prototype().count() != init_list.elements.size())
      continue;

    conversions.clear();
    for (size_t i(0); i < init_list.elements.size(); ++i)
    {
      ConversionSequence conv = ConversionSequence::compute(init_list.elements.at(i), c.prototype().at(i), engine);
      conversions.push_back(conv);
      if (conv == ConversionSequence::NotConvertible())
        break;
    }

    if (conversions.back() != ConversionSequence::NotConvertible())
    {
      auto result = std::make_shared<ListInitializationSequence>(ListInitializationSequence::ConstructorListInitialization, c);
      result->conversions() = std::move(conversions);
      return ConversionSequence{ result };
    }
  }

  // TODO : implement aggregate and initializer_list initialization

  return ConversionSequence::NotConvertible();
}

int ConversionSequence::comp(const ConversionSequence & a, const ConversionSequence & b)
{
  if ((a.listInitialization == nullptr) != (b.listInitialization == nullptr))
    throw std::runtime_error{ "ConversionSequence::comp() : the two sequences are not comparable" };

  if (a.listInitialization != nullptr)
    return ListInitializationSequence::comp(*a.listInitialization, *b.listInitialization);

  // 1) A standard conversion sequence is always better than a user defined conversion sequence.
  if (a.function.isNull() && !b.function.isNull())
    return -1;
  else if (b.function.isNull() && !a.function.isNull())
    return 1;

  if (a.function.isNull() && b.function.isNull())
  {
    if (a.conv1 < b.conv1)
      return -1;
    else if (b.conv1 < a.conv1)
      return 1;
    return 0;
  }

  assert(a.function.isNull() == false && b.function.isNull() == false);

  if (a.conv3 < b.conv3)
    return -1;
  else if (b.conv3 < a.conv3)
    return 1;
  return 0;
}

bool ConversionSequence::operator==(const ConversionSequence & other) const
{
  // TODO : add comparison when it is a brace initialization
  return conv1 == other.conv1 && conv3 == other.conv3 && other.function == other.function;
}

ListInitializationSequence::ListInitializationSequence(Kind k, Type t)
  : mKind(k)
  , mType(t)
{

}

ListInitializationSequence::ListInitializationSequence(Kind k, script::Function ctor)
  : mKind(k)
  , mConstructor(ctor)
{
  mType = ctor.memberOf().id();
}

std::vector<ConversionSequence> & ListInitializationSequence::conversions()
{
  return mConversions;
}

const std::vector<ConversionSequence> & ListInitializationSequence::conversions() const
{
  return mConversions;
}

int ListInitializationSequence::comp(const ListInitializationSequence & a, const ListInitializationSequence & b)
{
  if (a.mKind == InitializerListInitialization && b.mKind != InitializerListInitialization)
    return -1;
  else if (b.mKind == InitializerListInitialization && a.mKind != InitializerListInitialization)
    return 1;

  // TODO : we can do better, we should store in the ListInitializationSequence the worst ConversionSequence
  return 0;
}


static const int stdconv_table[] = {
               /* bool */ /* char */ /* int */ /* float */ /* double */
  /* bool */         0,         2,        3,          4,           5,
  /* char */         6,         0,        8,          9,          10,
  /* int */         11,        12,        0,         14,          15,
  /* float */       16,        17,       18,          0,          20,
  /* double */      21,        22,       23,         24,           0,
};

static const Type::BuiltInType stdconv_srctype_table[] = {
  Type::Auto,
  Type::Boolean, 
  Type::Boolean,
  Type::Boolean,
  Type::Boolean,
  Type::Boolean,
  Type::Char,
  Type::Char,
  Type::Char,
  Type::Char,
  Type::Char,
  Type::Int,
  Type::Int,
  Type::Int,
  Type::Int,
  Type::Int,
  Type::Float,
  Type::Float,
  Type::Float,
  Type::Float,
  Type::Float,
  Type::Double,
  Type::Double,
  Type::Double,
  Type::Double,
  Type::Double,
  Type::Auto, // enum to int
  Type::Auto, // derived to base
  Type::Null, // not convertible
};

static const Type::BuiltInType stdconv_desttype_table[] = {
  Type::Auto,
  Type::Boolean,
  Type::Char,
  Type::Int,
  Type::Float,
  Type::Double,
  Type::Boolean,
  Type::Char,
  Type::Int,
  Type::Float,
  Type::Double,
  Type::Boolean,
  Type::Char,
  Type::Int,
  Type::Float,
  Type::Double,
  Type::Boolean,
  Type::Char,
  Type::Int,
  Type::Float,
  Type::Double,
  Type::Boolean,
  Type::Char,
  Type::Int,
  Type::Float,
  Type::Double,
  Type::Int, // enum to int
  Type::Auto, // derived to base
  Type::Null, // not convertible
};

static const int conversion_categories[] = {
  0, // copy
                                     0,   NumericPromotion::IntegralPromotion,   NumericPromotion::IntegralPromotion,   NumericPromotion::FloatingPointPromotion, NumericPromotion::FloatingPointPromotion,
  NumericConversion::BooleanConversion,                                     0,   NumericPromotion::IntegralPromotion,   NumericPromotion::FloatingPointPromotion, NumericPromotion::FloatingPointPromotion,
  NumericConversion::BooleanConversion, NumericConversion::IntegralConversion,                                     0,   NumericPromotion::FloatingPointPromotion, NumericPromotion::FloatingPointPromotion,
  NumericConversion::BooleanConversion, NumericConversion::IntegralConversion, NumericConversion::IntegralConversion,                                          0, NumericPromotion::FloatingPointPromotion,
  NumericConversion::BooleanConversion, NumericConversion::IntegralConversion, NumericConversion::IntegralConversion, NumericConversion::FloatingPointConversion,                                        0,
  NumericConversion::IntegralConversion, // enum-to-int
  0, // derived-to-base
  0, // not-convertible
};

static const ConversionRank conversion_ranks[] = {
  ConversionRank::ExactMatch, // copy
  ConversionRank::ExactMatch,  ConversionRank::Promotion,  ConversionRank::Promotion,  ConversionRank::Promotion,  ConversionRank::Promotion,
  ConversionRank::Conversion, ConversionRank::ExactMatch,  ConversionRank::Promotion,  ConversionRank::Promotion,  ConversionRank::Promotion,
  ConversionRank::Conversion, ConversionRank::Conversion, ConversionRank::ExactMatch,  ConversionRank::Promotion,  ConversionRank::Promotion,
  ConversionRank::Conversion, ConversionRank::Conversion, ConversionRank::Conversion, ConversionRank::ExactMatch,  ConversionRank::Promotion,
  ConversionRank::Conversion, ConversionRank::Conversion, ConversionRank::Conversion, ConversionRank::Conversion, ConversionRank::ExactMatch,
  ConversionRank::Conversion, // enum-to-int
  ConversionRank::Conversion, // derived-to-base
  ConversionRank::NotConvertible, //not-convertible
};

// 5 bits to store conv between fundamental types and "not convertible"
// 1 bit to store ref-init, 1 bit to store qual adjust
static const int gEnumToIntConversion = 26;
static const int gDerivedToBaseConv = 27;
static const int gNotConvertibleStdConv = 28;
static const int gConstQualAdjustStdConv = (1 << 5);
static const int gCopyConvStdConv = (1 << 6);
static const int gRefConvStdConv = (1 << 6);
static const int gDerivedToBaseConvOffset = 8;
static const int gConvIdMask = (1 << 5) - 1;
static const int g8bitsMask = 255;


inline static bool checkNotConvertible(const Type & src, const Type & dest)
{
  return
    src == Type::Void ||
    dest == Type::Void ||
    (dest.isReference() && src.baseType() != dest.baseType()) ||
    (dest.isReference() && src.isConst() && !dest.isConst());
}

StandardConversion2::StandardConversion2()
{
  d = gRefConvStdConv;
}

StandardConversion2::StandardConversion2(const Type & src, const Type & dest)
{
  if (!src.isFundamentalType() || !dest.isFundamentalType())
    throw std::runtime_error{ "Types must be fundamental types" };

  if (checkNotConvertible(src, dest))
  {
    d = gNotConvertibleStdConv;
    return;
  }
  
  d = stdconv_table[(src.baseType().data() - 2)*5 + (dest.baseType().data() - 2)];
  if (dest.isReference())
    d |= gRefConvStdConv;
  if (dest.isConst() && !src.isConst())
    d |= gConstQualAdjustStdConv;
}

StandardConversion2::StandardConversion2(QualificationAdjustment qualadjust)
{
  d = gRefConvStdConv | (gConstQualAdjustStdConv * qualadjust);
}


bool StandardConversion2::isNone() const
{
  return d == gRefConvStdConv;
}

StandardConversion2 StandardConversion2::None()
{
  StandardConversion2 ret;
  ret.d = gRefConvStdConv;
  return ret;
}

bool StandardConversion2::isNarrowing() const
{
  return isNumericConversion();
}

ConversionRank StandardConversion2::rank() const
{
  if (isDerivedToBaseConversion())
    return ConversionRank::Conversion;
  else if (d == gNotConvertibleStdConv)
    return ConversionRank::NotConvertible;

  return conversion_ranks[d & gConvIdMask];
}

bool StandardConversion2::isReferenceConversion() const
{
  return d & gRefConvStdConv;
}

bool StandardConversion2::isNumericPromotion() const
{
  return numericPromotion() != NumericPromotion::NoNumericPromotion;
}

NumericPromotion StandardConversion2::numericPromotion() const
{
  return static_cast<NumericPromotion>(conversion_categories[d & gConvIdMask] & (NumericPromotion::IntegralPromotion | NumericPromotion::FloatingPointPromotion));
}

bool StandardConversion2::isNumericConversion() const
{
  return numericConversion() != NumericConversion::NoNumericConversion;
}

NumericConversion StandardConversion2::numericConversion() const
{
  return static_cast<NumericConversion>(conversion_categories[d & gConvIdMask] & (NumericConversion::IntegralConversion | NumericConversion::FloatingPointConversion | NumericConversion::BooleanConversion));
}

bool StandardConversion2::hasQualificationAdjustment() const
{
  return d & gConstQualAdjustStdConv;
}

bool StandardConversion2::isDerivedToBaseConversion() const
{
  return (d & gConvIdMask) == gDerivedToBaseConv;
  // Alternative:
  //return ((d >> gDerivedToBaseConvOffset) & g8bitsMask) != 0;
}

int StandardConversion2::derivedToBaseConversionDepth() const
{
  return (d >> gDerivedToBaseConvOffset) & g8bitsMask;
}

Type StandardConversion2::srcType() const
{
  return stdconv_srctype_table[d & gConvIdMask];
}

Type StandardConversion2::destType() const
{
  /// TODO : should we add const-qual and ref-specifiers ?
  return stdconv_desttype_table[d & gConvIdMask];
}

StandardConversion2 StandardConversion2::with(QualificationAdjustment adjust) const
{
  StandardConversion2 conv{ *this };
  conv.d |= static_cast<int>(adjust) * gConstQualAdjustStdConv;
  return conv;
}

StandardConversion2 StandardConversion2::Copy()
{
  return StandardConversion2{ 0 };
}

StandardConversion2 StandardConversion2::EnumToInt()
{
  return StandardConversion2{ gEnumToIntConversion };
}

StandardConversion2 StandardConversion2::DerivedToBaseConversion(int depth, bool is_ref_conv, QualificationAdjustment adjust)
{
  return StandardConversion2{ gDerivedToBaseConv |(depth << gDerivedToBaseConvOffset) | (is_ref_conv ? gRefConvStdConv : 0) | (adjust ? gConstQualAdjustStdConv : 0) };
}

StandardConversion2 StandardConversion2::NotConvertible()
{
  StandardConversion2 seq;
  seq.d = gNotConvertibleStdConv;
  return seq;
}


StandardConversion2 StandardConversion2::compute(const Type & src, const Type & dest, Engine *e)
{
  if (dest.isReference() && src.isConst() && !dest.isConst())
    return StandardConversion2::NotConvertible();

  if (dest.isFundamentalType() && src.isFundamentalType())
    return StandardConversion2{ src, dest };

  if (src.isObjectType() && dest.isObjectType())
  {
    const Class src_class = e->getClass(src), dest_class = e->getClass(dest);
    const int inheritance_depth = src_class.inheritanceLevel(dest_class);

    if (inheritance_depth < 0)
      return StandardConversion2::NotConvertible();

    const QualificationAdjustment adjust = (dest.isConst() && !src.isConst()) ? ConstQualification : NoQualificationAdjustment;


    if (inheritance_depth == 0)
    {
      if (dest.isReference())
        return StandardConversion2{};

      if (!dest_class.isCopyConstructible())
        return StandardConversion2::NotConvertible();
      return StandardConversion2::Copy().with(adjust);
    }
    else
    {
      if (dest.isReference())
        return StandardConversion2::DerivedToBaseConversion(inheritance_depth, dest.isReference(), adjust);

      if (!dest_class.isCopyConstructible())
        return StandardConversion2::NotConvertible();
      return StandardConversion2::DerivedToBaseConversion(inheritance_depth, dest.isReference(), adjust);
    }
  }
  else if (src.baseType() == dest.baseType())
  {
    if (dest.isReference())
      return StandardConversion2{};

    if (dest.isEnumType() || dest.isClosureType() || dest.isFunctionType())
      return StandardConversion2::Copy();
  }
  else if (src.isEnumType() && dest.baseType() == Type::Int)
  {
    if (dest.isReference())
      return StandardConversion2::NotConvertible();

    return StandardConversion2::EnumToInt();
  }

  return StandardConversion2::NotConvertible();
}

bool StandardConversion2::operator==(const StandardConversion2 & other) const
{
  return d == other.d;
}

bool StandardConversion2::operator<(const StandardConversion2 & other) const
{
  auto this_rank = rank();
  auto other_rank = other.rank();
  if (this_rank < other_rank)
    return true;
  else if (this_rank > other_rank)
    return false;

  if (other.isDerivedToBaseConversion() && this->isDerivedToBaseConversion())
  {
    if (this->derivedToBaseConversionDepth() < other.derivedToBaseConversionDepth())
      return true;
    else if (this->derivedToBaseConversionDepth() > other.derivedToBaseConversionDepth())
      return false;
  }

  if (!this->isReferenceConversion() && other.isReferenceConversion())
    return false;
  else if (this->isReferenceConversion() && !other.isReferenceConversion())
    return true;

  if (other.hasQualificationAdjustment() && !this->hasQualificationAdjustment())
    return true;

  return false;
}


static Conversion select_converting_constructor2(const Type & src, const std::vector<Function> & ctors, const Type & dest, Engine *engine, Conversion::ConversionPolicy policy)
{
  if (dest.isReference() && !dest.isConst() && src.isConst())
    return Conversion::NotConvertible();

  // We store the two best conversion sequences to detect ambiguity.
  StandardConversion2 best_conv = StandardConversion2::NotConvertible();
  Function best_ctor;
  StandardConversion2 ambiguous_conv = StandardConversion2::NotConvertible();

  for (const auto & c : ctors)
  {
    if (c.prototype().count() != 1)
      continue;

    if (c.isExplicit() && policy == Conversion::NoExplicitConversions)
      continue;

    StandardConversion2 first_conversion = StandardConversion2::compute(src, c.prototype().at(0), engine);
    if (first_conversion == StandardConversion2::NotConvertible())
      continue;

    bool comp1 = first_conversion < best_conv;
    bool comp2 = best_conv < first_conversion;

    if (comp1 && !comp2)
    {
      best_conv = first_conversion;
      best_ctor = c;
      ambiguous_conv = StandardConversion2::NotConvertible();
    }
    else if (comp2 && !comp1)
    {
      // nothing to do
    }
    else
    {
      ambiguous_conv = first_conversion;
    }
  }

  if (!(best_conv < ambiguous_conv))
    return Conversion::NotConvertible();

  /// TODO: correctly compute second conversion, which can be
  // - a const-qualification
  // - a derived to base conversion
  //StandardConversion second_conversion = StandardConversion::compute(Type::rref(dest.baseType()), dest, engine);
  /// TODO : not sure this is correct
  /*if (second_conversion == StandardConversion::NotConvertible())
  continue;*/
  //second_conversion = StandardConversion::None();

  return Conversion{ best_conv, best_ctor, StandardConversion2::None() };
}

static Conversion select_cast2(const Type & src, const std::vector<Cast> & casts, const Type & dest, Engine *engine, Conversion::ConversionPolicy policy)
{
  // TODO : before returning, check if better candidates can be found ?
  // this would result in an ambiguous conversion sequence I believe
  for (const auto & c : casts)
  {
    if (c.isExplicit() && policy == Conversion::NoExplicitConversions)
      continue;

    StandardConversion2 first_conversion = StandardConversion2::compute(src, c.sourceType(), engine);
    if (first_conversion == StandardConversion2::NotConvertible())
      continue;

    StandardConversion2 second_conversion = StandardConversion2::compute(c.destType(), dest, engine);
    if (second_conversion == StandardConversion2::NotConvertible())
      continue;
    // Avoid additonal useless copy
    if (second_conversion == StandardConversion2::Copy())
      second_conversion = StandardConversion2{};

    return Conversion{ first_conversion, c, second_conversion };
  }

  return Conversion::NotConvertible();
}


Conversion::Conversion(const StandardConversion2 & c1, const Function & userdefinedConversion, const StandardConversion2 & c2)
  : conv1(c1)
  , function(userdefinedConversion)
  , conv3(c2)
{

}

ConversionRank Conversion::rank() const
{
  if (conv1 == StandardConversion2::NotConvertible())
    return ConversionRank::NotConvertible;

  if (!function.isNull())
    return ConversionRank::UserDefinedConversion;

  return conv1.rank();
}

bool Conversion::isInvalid() const
{
  return conv1.rank() == ConversionRank::NotConvertible;
}

bool Conversion::isNarrowing() const
{
  return conv1.isNarrowing() || conv3.isNarrowing();
}

bool Conversion::isUserDefinedConversion() const
{
  return !this->function.isNull();
}

Type Conversion::srcType() const
{
  if (function.isNull())
    return conv1.srcType();
  else if (function.isConstructor())
    return function.parameter(0).baseType();

  assert(function.isCast());
  if (!conv3.isNone())
    return conv3.destType();
  return function.parameter(0).baseType();
}

Type Conversion::destType() const
{
  if (function.isNull())
    return conv1.destType();
  else if (function.isConstructor())
    return function.memberOf().id();

  assert(function.isCast());
  return function.returnType().baseType();
}

Conversion Conversion::NotConvertible()
{
  return Conversion{ StandardConversion2::NotConvertible() };
}

Conversion Conversion::compute(const Type & src, const Type & dest, Engine *engine, ConversionPolicy policy)
{
  StandardConversion2 stdconv = StandardConversion2::compute(src, dest, engine);
  if (stdconv != StandardConversion2::NotConvertible())
    return stdconv;

  if (!src.isObjectType() && !dest.isObjectType())
    return Conversion::NotConvertible();

  assert(src.isObjectType() || dest.isObjectType());

  if (dest.isObjectType())
  {
    const auto & ctors = engine->getClass(dest).constructors();
    Conversion userdefconv = select_converting_constructor2(src, ctors, dest, engine, policy);
    if (userdefconv != Conversion::NotConvertible())
      return userdefconv;
  }

  if (src.isObjectType())
  {
    const auto & casts = engine->getClass(src).casts();
    Conversion userdefconv = select_cast2(src, casts, dest, engine, policy);
    if (userdefconv != Conversion::NotConvertible())
      return userdefconv;
  }

  /// TODO : implement other forms of conversion

  return Conversion::NotConvertible();
}

Conversion Conversion::compute(const std::shared_ptr<program::Expression> & expr, const Type & dest, Engine *engine)
{
  if (expr->type() == Type::InitializerList)
    throw std::runtime_error{ "Conversion does not support brace-expressions" };

  return compute(expr->type(), dest, engine);
}

int Conversion::comp(const Conversion & a, const Conversion & b)
{
  // 1) A standard conversion sequence is always better than a user defined conversion sequence.
  if (a.function.isNull() && !b.function.isNull())
    return -1;
  else if (b.function.isNull() && !a.function.isNull())
    return 1;

  if (a.function.isNull() && b.function.isNull())
  {
    if (a.conv1 < b.conv1)
      return -1;
    else if (b.conv1 < a.conv1)
      return 1;
    return 0;
  }

  assert(a.function.isNull() == false && b.function.isNull() == false);

  if (a.conv3 < b.conv3)
    return -1;
  else if (b.conv3 < a.conv3)
    return 1;
  return 0;
}

bool Conversion::operator==(const Conversion & other) const
{
  return conv1 == other.conv1 && conv3 == other.conv3 && other.function == other.function;
}


} // namespace script