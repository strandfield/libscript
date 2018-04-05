// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/compiler/expressioncompiler.h"

#include "script/compiler/compilererrors.h"
#include "script/compiler/lambdacompiler.h"

#include "script/ast/ast.h"
#include "script/ast/node.h"

#include "script/program/expression.h"
#include "script/program/statements.h"

#include "script/cast.h"
#include "../engine_p.h"
#include "script/functiontype.h"
#include "script/lambda.h"
#include "script/literals.h"
#include "script/namelookup.h"
#include "../namelookup_p.h"
#include "script/overloadresolution.h"

namespace script
{

namespace compiler
{

AbstractExpressionCompiler::AbstractExpressionCompiler(Compiler *c, CompileSession *s)
  : CompilerComponent(c, s)
{

}

TemplateArgument AbstractExpressionCompiler::generateTemplateArgument(const std::shared_ptr<ast::Node> & arg)
{
  if (arg->is<ast::Identifier>())
  {
    auto name = std::dynamic_pointer_cast<ast::Identifier>(arg);
    NameLookup lookup = resolve(name);
    if (lookup.resultType() == NameLookup::TypeName)
      return TemplateArgument::make(lookup.typeResult());
    else
    {
      std::shared_ptr<program::Expression> expr = generateVariableAccess(name, lookup);
      return generateTemplateArgument(expr, arg);
    }
  }
  else if (arg->is<ast::Literal>())
  {
    const ast::Literal & l = arg->as<ast::Literal>();
    if (l.is<ast::BoolLiteral>())
      return TemplateArgument::make(l.token == parser::Token::True);
    else if (l.is<ast::IntegerLiteral>())
      return TemplateArgument::make(generateIntegerLiteral(std::dynamic_pointer_cast<ast::IntegerLiteral>(arg)));
    else
      throw InvalidLiteralTemplateArgument{ dpos(arg) };
  }
  else if (arg->is<ast::TypeNode>())
  {
    auto type = std::dynamic_pointer_cast<ast::TypeNode>(arg);
    return TemplateArgument::make(resolve(type->value));
  }
  else if (arg->is<ast::Expression>())
  {
    auto expr = generateExpression(std::dynamic_pointer_cast<ast::Expression>(arg));
    return generateTemplateArgument(expr, arg);
  }

  throw InvalidTemplateArgument{ dpos(arg) };
}

TemplateArgument AbstractExpressionCompiler::generateTemplateArgument(const std::shared_ptr<program::Expression> & e, const std::shared_ptr<ast::Node> & src)
{
  if (!isConstExpr(e))
    throw NonConstExprTemplateArgument{ dpos(src) };

  Value val = evalConstExpr(e);

  if (val.isBool())
    return TemplateArgument::make(val.toBool());
  else if (val.isChar())
    return TemplateArgument::make(val.toChar());
  else if (val.isInt())
    return TemplateArgument::make(val.toInt());

  throw InvalidTemplateArgumentType{ dpos(src) };
}

std::vector<TemplateArgument> AbstractExpressionCompiler::generateTemplateArguments(const std::vector<std::shared_ptr<ast::Node>> & args)
{
  std::vector<TemplateArgument> ret;
  for (const auto & a : args)
    ret.push_back(generateTemplateArgument(a));
  return ret;
}

bool AbstractExpressionCompiler::isConstExpr(const std::shared_ptr<program::Expression> & e)
{
  if (e->is<program::Literal>())
    return true;
  else if (e->is<program::FunctionCall>())
  {
    const program::FunctionCall & fc = dynamic_cast<const program::FunctionCall &>(*e);
    if (!fc.callee.isOperator())
      return false;

    if (!fc.callee.toOperator().isBuiltin())
      return false;
      
    if (!fc.callee.prototype().argv(0).isConst())
      return false;

    if (fc.args.size() == 1)
      return isConstExpr(fc.args.front());
    return isConstExpr(fc.args.front()) && isConstExpr(fc.args.back());
  }

  return false;
}

Value AbstractExpressionCompiler::evalConstExpr(const std::shared_ptr<program::Expression> & expr)
{
  /// TODO : check this implementation !!
  // this might be a little too simple...
  return engine()->implementation()->interpreter->eval(expr);
}

Type AbstractExpressionCompiler::resolveFunctionType(const ast::QualifiedType & qt)
{
  Prototype proto;
  proto.setReturnType(resolve(qt.functionType->returnType));

  for (const auto & p : qt.functionType->params)
    proto.addArgument(resolve(p));

  auto ft = engine()->getFunctionType(proto);
  Type t = ft.type();
  if (qt.constQualifier.isValid())
    t = t.withFlag(Type::ConstFlag);
  if (qt.reference == parser::Token::Ref)
    t = t.withFlag(Type::ReferenceFlag);
  else if (qt.reference == parser::Token::RefRef)
    t = t.withFlag(Type::ForwardReferenceFlag);
  return t;
}

Type AbstractExpressionCompiler::resolve(const ast::QualifiedType & qt)
{
  if (qt.functionType != nullptr)
    return resolveFunctionType(qt);

  NameLookup lookup = resolve(qt.type);
  if (lookup.resultType() != NameLookup::TypeName)
    throw InvalidTypeName{ dpos(qt.type), repr(qt.type) };

  Type t = lookup.typeResult();
  if (qt.constQualifier.isValid())
    t.setFlag(Type::ConstFlag);
  if (qt.isRef())
    t.setFlag(Type::ReferenceFlag);
  else if (qt.isRefRef())
    t.setFlag(Type::ForwardReferenceFlag);

  return t;
}

std::vector<Function> & AbstractExpressionCompiler::removeDuplicates(std::vector<Function> & list)
{
  /// TODO

  return list;
}


std::vector<Function> AbstractExpressionCompiler::getScopeOperators(Operator::BuiltInOperator op, const script::Scope & scp, int lookup_policy)
{
  std::vector<Function> ret = scp.operators(op);
  if ((lookup_policy & OperatorLookupPolicy::FetchParentOperators) && !scp.parent().isNull())
  {
    const auto & ops = getScopeOperators(op, scp.parent(), OperatorLookupPolicy::FetchParentOperators);
    ret.insert(ret.end(), ops.begin(), ops.end());
  }

  if (lookup_policy & OperatorLookupPolicy::RemoveDuplicates)
    return removeDuplicates(ret);

  return ret;
}

std::vector<Function> AbstractExpressionCompiler::getBinaryOperators(Operator::BuiltInOperator op, Type a, Type b)
{
  std::vector<Function> result = getOperators(op, a, OperatorLookupPolicy::ConsiderCurrentScope | OperatorLookupPolicy::FetchParentOperators);
  const std::vector<Function> & others = getOperators(op, b, OperatorLookupPolicy::FetchParentOperators);
  result.insert(result.end(), others.begin(), others.end());
  return removeDuplicates(result);
}

std::vector<Function> AbstractExpressionCompiler::getUnaryOperators(Operator::BuiltInOperator op, Type a)
{
  std::vector<Function> result = getOperators(op, a, OperatorLookupPolicy::ConsiderCurrentScope | OperatorLookupPolicy::FetchParentOperators);
  return removeDuplicates(result);
}

std::vector<Function> AbstractExpressionCompiler::getLiteralOperators(const std::string & suffix)
{
  /// TODO : improve this impl
  std::vector<Function> ret;

  auto insert_if = [&ret, &suffix](const LiteralOperator & lop) -> void {
    if (lop.suffix() == suffix)
      ret.push_back(lop);
  };

  Scope s = currentScope();
  while (!s.isNull())
  {
    const auto & lops = s.literalOperators();
    for (const auto & lop : lops)
      insert_if(lop);
    s = s.parent();
  }

  return ret;
}

std::vector<Function> AbstractExpressionCompiler::getCallOperator(const Type & functor_type)
{
  std::vector<Function> result;

  if (functor_type.isObjectType())
  {
    Class cla = engine()->getClass(functor_type);
    const auto & operators = cla.operators();
    for (const auto & op : operators)
    {
      if (op.operatorId() == Operator::FunctionCallOperator)
        result.push_back(op);
    }

    if (!result.empty())
      return result;

    if (!cla.parent().isNull())
      return getCallOperator(cla.parent().id());
  }
  else if (functor_type.isClosureType())
  {
    Lambda closure = engine()->getLambda(functor_type);
    return { closure.function() };
  }

  return result;
}


std::vector<std::shared_ptr<program::Expression>> AbstractExpressionCompiler::generateExpressions(const std::vector<std::shared_ptr<ast::Expression>> & expressions)
{
  std::vector<std::shared_ptr<program::Expression>> ret;
  generateExpressions(expressions, ret);
  return ret;
}

void AbstractExpressionCompiler::generateExpressions(const std::vector<std::shared_ptr<ast::Expression>> & in, std::vector<std::shared_ptr<program::Expression>> & out)
{
  for (const auto & e : in)
    out.push_back(generateExpression(e));
}

std::shared_ptr<program::Expression> AbstractExpressionCompiler::applyStandardConversion(const std::shared_ptr<program::Expression> & arg, const Type & type, const StandardConversion & conv)
{
  if (!conv.isCopyInitialization())
    return arg;

  assert(conv.isCopyInitialization());

  if (conv.isDerivedToBaseConversion() || type.isObjectType())
  {
    Class dest_class = engine()->getClass(type);
    return program::ConstructorCall::New(dest_class.copyConstructor(), { arg });
  }

  if (arg->type().baseType() == type.baseType())
    return program::Copy::New(type, arg);

  return program::FundamentalConversion::New(type, arg);
}

std::shared_ptr<program::Expression> AbstractExpressionCompiler::performListInitialization(const std::shared_ptr<program::Expression> & arg, const Type & type, const std::shared_ptr<ListInitializationSequence> & linit)
{
  throw std::runtime_error{ "Not implemented" };
}

std::shared_ptr<program::Expression> AbstractExpressionCompiler::prepareFunctionArgument(const std::shared_ptr<program::Expression> & arg, const Type & type, const ConversionSequence & conv)
{
  if (conv.isListInitialization())
    return performListInitialization(arg, type, conv.listInitialization);
  else if (!conv.isUserDefinedConversion())
    return applyStandardConversion(arg, type, conv.conv1);

  std::shared_ptr<program::Expression> ret = nullptr;

  if (conv.function.isCast())
  {
    auto cast = conv.function.toCast();
    ret = applyStandardConversion(arg, cast.sourceType(), conv.conv1);
    ret = program::FunctionCall::New(cast, { ret });
  }
  else
  {
    assert(conv.function.isConstructor());

    auto ctor = conv.function;
    ret = applyStandardConversion(arg, ctor.prototype().argv(0), conv.conv1);
    ret = program::ConstructorCall::New(ctor, { ret });
  }

  return applyStandardConversion(ret, type, conv.conv3);
}

void AbstractExpressionCompiler::prepareFunctionArguments(std::vector<std::shared_ptr<program::Expression>> & args, const Prototype & proto, const std::vector<ConversionSequence> & conversions)
{
  for (size_t i(0); i < args.size(); ++i)
  {
    args[i] = prepareFunctionArgument(args.at(i), proto.argv(i), conversions.at(i));
  }
}


std::shared_ptr<program::Expression> AbstractExpressionCompiler::constructFundamentalValue(const Type & t, bool copy)
{
  assert(t.isFundamentalType());

  Value val;
  switch (t.baseType().data())
  {
  case Type::Null:
  case Type::Void:
    throw NotImplementedError{ "Could not construct value of type void" };
  case Type::Boolean:
    val = engine()->newBool(false);
    break;
  case Type::Char:
    val = engine()->newChar('\0');
    break;
  case Type::Int:
    val = engine()->newInt(0);
    break;
  case Type::Float:
    val = engine()->newFloat(0.f);
    break;
  case Type::Double:
    val = engine()->newDouble(0.);
    break;
  default:
    throw NotImplementedError{ "Could not construct value of given fundamental type" };
  }

  engine()->manage(val);

  auto lit = program::Literal::New(val);
  if (copy)
    return program::Copy::New(t, lit);
  return lit;
}

std::shared_ptr<program::Expression> AbstractExpressionCompiler::braceConstructValue(const Type & type, std::vector<std::shared_ptr<program::Expression>> && args, diagnostic::pos_t dp)
{
  if (args.size() == 0)
    return constructValue(type, nullptr, dp);

  if (!type.isObjectType() && args.size() != 1)
    throw TooManyArgumentInInitialization{ dp };

  if ((type.isReference() || type.isRefRef()) && args.size() != 1)
    throw TooManyArgumentInReferenceInitialization{ dp };

  if (type.isFundamentalType() || type.isEnumType() || type.isFunctionType())
  {
    ConversionSequence seq = ConversionSequence::compute(args.front(), type, engine());
    if (seq == ConversionSequence::NotConvertible())
      throw CouldNotConvert{ dp, dstr(args.front()->type()), dstr(type) };

    if (seq.isNarrowing())
      throw NarrowingConversionInBraceInitialization{ dp, dstr(args.front()->type()), dstr(type) };

    return prepareFunctionArgument(args.front(), type, seq);
  }
  else if (type.isObjectType())
  {
    const std::vector<Function> & ctors = engine()->getClass(type).constructors();
    OverloadResolution resol = OverloadResolution::New(engine());
    if (!resol.process(ctors, args))
      throw CouldNotFindValidConstructor{ dp }; /// TODO add a better diagnostic message

    const Function ctor = resol.selectedOverload();
    const auto & conversions = resol.conversionSequence();
    for (std::size_t i(0); i < conversions.size(); ++i)
    {
      const auto & conv = conversions.at(i);
      if (conv.isNarrowing())
        throw NarrowingConversionInBraceInitialization{ dp, dstr(args.at(i)->type()), dstr(ctor.parameter(i)) };
    }

    prepareFunctionArguments(args, ctor.prototype(), conversions);
    return program::ConstructorCall::New(ctor, std::move(args));
  }
  else
    throw std::runtime_error{ "Not implemented" };
}

std::shared_ptr<program::Expression> AbstractExpressionCompiler::constructValue(const Type & type, std::nullptr_t, diagnostic::pos_t dp)
{
  if (type.isReference() || type.isRefRef())
    throw ReferencesMustBeInitialized{ dp };

  if (type.isFundamentalType())
    return constructFundamentalValue(type, true);
  else if (type.isEnumType())
    throw EnumerationsCannotBeDefaultConstructed{ dp };
  else if (type.isFunctionType())
    throw FunctionVariablesMustBeInitialized{ dp };
  else if (type.isObjectType())
  {
    Function default_ctor = engine()->getClass(type).defaultConstructor();
    if (default_ctor.isNull())
      throw VariableCannotBeDefaultConstructed{ dp, dstr(type) };
    else if (default_ctor.isDeleted())
      throw ClassHasDeletedDefaultCtor{ dp, dstr(type) };

    return program::ConstructorCall::New(default_ctor, {});
  }

  throw NotImplementedError{ dp, "AbstractExpressionCompiler::constructValue() : cannot default construct value" };
}

std::shared_ptr<program::Expression> AbstractExpressionCompiler::constructValue(const Type & type, std::vector<std::shared_ptr<program::Expression>> && args, diagnostic::pos_t dp)
{
  if (args.size() == 0)
    return constructValue(type, nullptr, dp);

  if (!type.isObjectType() && args.size() != 1)
    throw TooManyArgumentInInitialization{ dp };

  if ((type.isReference() || type.isRefRef()) && args.size() != 1)
    throw TooManyArgumentInReferenceInitialization{ dp };

  if (type.isFundamentalType() || type.isEnumType() || type.isFunctionType())
  {
    ConversionSequence seq = ConversionSequence::compute(args.front(), type, engine());
    if (seq == ConversionSequence::NotConvertible())
      throw CouldNotConvert{ dp, dstr(args.front()->type()), dstr(type) };

    return prepareFunctionArgument(args.front(), type, seq);
  }
  else if (type.isObjectType())
  {
    const std::vector<Function> & ctors = engine()->getClass(type).constructors();
    OverloadResolution resol = OverloadResolution::New(engine());
    if (!resol.process(ctors, args))
      throw CouldNotFindValidConstructor{ dp }; /// TODO add a better diagnostic message

    const Function ctor = resol.selectedOverload();
    const auto & conversions = resol.conversionSequence();
    prepareFunctionArguments(args, ctor.prototype(), conversions);
    return program::ConstructorCall::New(ctor, std::move(args));
  }
  else
    throw NotImplementedError{ dp, "AbstractExpressionCompiler::constructValue() : type not implemented" };
}

std::shared_ptr<program::Expression> AbstractExpressionCompiler::constructValue(const Type & t, const std::shared_ptr<ast::ConstructorInitialization> & init)
{
  auto args = generateExpressions(init->args);
  return constructValue(t, std::move(args), dpos(init));
}

std::shared_ptr<program::Expression> AbstractExpressionCompiler::constructValue(const Type & t, const std::shared_ptr<ast::BraceInitialization> & init)
{
  auto args = generateExpressions(init->args);
  return braceConstructValue(t, std::move(args), dpos(init));
}


int AbstractExpressionCompiler::generateIntegerLiteral(const std::shared_ptr<ast::IntegerLiteral> & l)
{
  std::string i = l->toString();
  if (i.find("0x") == 0)
    return std::stoi(i.substr(2), nullptr, 16);
  else if(i.find("0b") == 0)
    return std::stoi(i.substr(2), nullptr, 2);
  else if (i.find("0") == 0)
    return std::stoi(i, nullptr, 8);
  return std::stoi(i, nullptr, 10);
}

std::shared_ptr<program::Expression> AbstractExpressionCompiler::generateArrayConstruction(const std::shared_ptr<ast::ArrayExpression> & array_expr)
{
  std::vector<std::shared_ptr<program::Expression>> args = generateExpressions(array_expr->elements);

  if (args.size() == 0)
    throw NotImplementedError{ "AbstractExpressionCompiler::generateArrayConstruction() : array of size 0" };

  const Type element_type = args.front()->type().baseType();
  if (element_type == Type::InitializerList)
    throw InitializerListAsFirstArrayElement{};

  std::vector<ConversionSequence> conversions;
  conversions.reserve(args.size());
  for (const auto & arg : args)
  {
    auto conv = ConversionSequence::compute(arg, element_type, engine());
    if (conv == ConversionSequence::NotConvertible())
      throw ArrayElementNotConvertible{};

    conversions.push_back(conv);
  }

  auto array_template = engine()->getTemplate(Engine::ArrayTemplate);
  Class array_class = array_template.getInstance({ TemplateArgument::make(element_type) });

  for (size_t i(0); i < args.size(); ++i)
    args[i] = prepareFunctionArgument(args.at(i), element_type, conversions.at(i));

  return program::ArrayExpression::New(array_class.id(), std::move(args));
}

std::shared_ptr<program::Expression> AbstractExpressionCompiler::generateBraceConstruction(const std::shared_ptr<ast::BraceConstruction> & bc)
{
  NameLookup lookup = resolve(bc->temporary_type);
  if (lookup.typeResult().isNull())
    throw UnknownTypeInBraceInitialization{ dpos(bc), dstr(bc->temporary_type) };

  /// TODO : refactor this huge duplicate of FunctionCompiler::generateVariableDeclaration()
  const Type & type = lookup.typeResult();

  if (!type.isObjectType() && bc->arguments.size() != 1)
    throw TooManyArgumentInVariableInitialization{ dpos(bc) };

  std::vector<std::shared_ptr<program::Expression>> args = generateExpressions(bc->arguments);

  return braceConstructValue(type, std::move(args), dpos(bc));
}

std::shared_ptr<program::Expression> AbstractExpressionCompiler::generateConstructorCall(const std::shared_ptr<ast::FunctionCall> & fc, const Type & type, std::vector<std::shared_ptr<program::Expression>> && args)
{
  return constructValue(type, std::move(args), dpos(fc));
}

std::shared_ptr<program::Expression> AbstractExpressionCompiler::generateListExpression(const std::shared_ptr<ast::ListExpression> & list_expr)
{
  auto elements = generateExpressions(list_expr->elements);
  return program::InitializerList::New(std::move(elements));
}


std::shared_ptr<program::Expression> AbstractExpressionCompiler::generateArraySubscript(const std::shared_ptr<ast::ArraySubscript> & as)
{
  std::shared_ptr<program::Expression> obj = generateExpression(as->array);
  std::shared_ptr<program::Expression> index = generateExpression(as->index);

  const Type & objType = obj->type();
  if (!objType.isObjectType())
    throw ArraySubscriptOnNonObject{ dpos(as) };

  const Type & argType = index->type();

  std::vector<Function> candidates = this->getBinaryOperators(Operator::SubscriptOperator, objType, argType);
  if (candidates.empty())
    throw CouldNotFindValidSubscriptOperator{ dpos(as) };

  OverloadResolution resol = OverloadResolution::New(engine());
  if (!resol.process(candidates, std::vector<Type>{objType, argType}))
    throw CouldNotFindValidSubscriptOperator{ dpos(as) };

  Function selected = resol.selectedOverload();

  std::vector<std::shared_ptr<program::Expression>> args;
  args.push_back(obj);
  args.push_back(index);

  const auto & conversions = resol.conversionSequence();

  prepareFunctionArguments(args, selected.prototype(), conversions);

  return program::FunctionCall::New(selected, std::move(args));
}

std::shared_ptr<program::Expression> AbstractExpressionCompiler::generateCall(const std::shared_ptr<ast::FunctionCall> & call)
{
  std::vector<std::shared_ptr<program::Expression>> args = generateExpressions(call->arguments);
  const auto & callee = call->callee;

  if (callee->is<ast::Identifier>())
  {
    NameLookup lookup = NameLookup::resolve(std::dynamic_pointer_cast<ast::Identifier>(callee), args, this);

    if (lookup.resultType() == NameLookup::FunctionName)
    {
      OverloadResolution resol = OverloadResolution::New(engine());
      if (!resol.process(lookup.functions(), args))
        throw CouldNotFindValidMemberFunction{ dpos(call) };

      Function selected = resol.selectedOverload();
      if (selected.isDeleted())
        throw CallToDeletedFunction{ dpos(call) };

      const auto & convs = resol.conversionSequence();
      prepareFunctionArguments(args, selected.prototype(), convs);
      if (selected.isConstructor())
        return program::ConstructorCall::New(selected, std::move(args));
      else
        return program::FunctionCall::New(selected, std::move(args));
    }
    else if (lookup.resultType() == NameLookup::VariableName || lookup.resultType() == NameLookup::GlobalName
      || lookup.resultType() == NameLookup::DataMemberName || lookup.resultType() == NameLookup::LocalName)
    {
      auto functor = generateVariableAccess(std::dynamic_pointer_cast<ast::Identifier>(callee), lookup);
      return generateFunctorCall(call, functor, std::move(args));
    }
    else if (lookup.resultType() == NameLookup::TypeName)
    {
      return generateConstructorCall(call, lookup.typeResult(), std::move(args));
    }

    throw std::runtime_error{ "Not implemented" };

  }
  else if (callee->is<ast::Operation>() && callee->as<ast::Operation>().operatorToken == parser::Token::Dot)
  {
    auto member_access = std::dynamic_pointer_cast<ast::Operation>(callee);
    assert(member_access->arg2->is<ast::Identifier>());

    auto object = generateExpression(member_access->arg1);

    NameLookup lookup = NameLookup::member(std::dynamic_pointer_cast<ast::Identifier>(member_access->arg2)->getName(), engine()->getClass(object->type()));
    if (lookup.resultType() == NameLookup::DataMemberName)
    {
      auto functor = generateMemberAccess(object, lookup.dataMemberIndex());
      return generateFunctorCall(call, functor, std::move(args));
    }
    else if (lookup.resultType() == NameLookup::FunctionName)
    {
      args.insert(args.begin(), object);

      OverloadResolution resol = OverloadResolution::New(engine());
      if (!resol.process(lookup.functions(), args))
        throw CouldNotFindValidOverload{ dpos(call) };

      Function selected = resol.selectedOverload();
      const auto & convs = resol.conversionSequence();
      prepareFunctionArguments(args, selected.prototype(), convs);
      assert(!selected.isConstructor()); /// TODO : check that this is not possible
      if (selected.isVirtual() && member_access->arg2->type() == ast::NodeType::SimpleIdentifier)
        return generateVirtualCall(call, selected, std::move(args));
      return program::FunctionCall::New(selected, std::move(args));
    }
    else
      throw std::runtime_error{ "Not implemented" };
  }
  else if (callee->is<ast::Expression>())
  {
    auto functor = generateExpression(std::dynamic_pointer_cast<ast::Expression>(callee));
    return generateFunctorCall(call, functor, std::move(args));
  }
  else
    throw std::runtime_error{ "Invalid callee / implementation error" }; /// TODO : which one

  throw std::runtime_error{ "Not implemented" };
}

std::shared_ptr<program::Expression> AbstractExpressionCompiler::generateVirtualCall(const std::shared_ptr<ast::FunctionCall> & call, const Function & f, std::vector<std::shared_ptr<program::Expression>> && args)
{
  assert(f.isVirtual());

  Class c = f.memberOf();
  const auto & vtable = c.vtable();
  auto it = std::find(vtable.begin(), vtable.end(), f);
  if (it == vtable.end())
    throw NotImplementedError{ "Implementation error when calling virtual member" };

  auto object = args.front();
  args.erase(args.begin());
  return program::VirtualCall::New(object, std::distance(vtable.begin(), it), f.returnType(), std::move(args));
}

std::shared_ptr<program::Expression> AbstractExpressionCompiler::generateFunctorCall(const std::shared_ptr<ast::FunctionCall> & call, const std::shared_ptr<program::Expression> & functor, std::vector<std::shared_ptr<program::Expression>> && args)
{
  if (functor->type().isFunctionType())
    return generateFunctionVariableCall(call, functor, std::move(args));
  
  std::vector<Function> functions = getCallOperator(functor->type());
  OverloadResolution resol = OverloadResolution::New(engine());
  if (!resol.process(functions, args, functor))
    throw CouldNotFindValidCallOperator{ dpos(call) };

  Function selected = resol.selectedOverload();
  assert(selected.isMemberFunction());
  args.insert(args.begin(), functor);
  const auto & convs = resol.conversionSequence();
  prepareFunctionArguments(args, selected.prototype(), convs);
  return program::FunctionCall::New(selected, std::move(args));
}

std::shared_ptr<program::Expression> AbstractExpressionCompiler::generateFunctionVariableCall(const std::shared_ptr<ast::FunctionCall> & call, const std::shared_ptr<program::Expression> & functor, std::vector<std::shared_ptr<program::Expression>> && args)
{
  auto function_type = engine()->getFunctionType(functor->type());
  const Prototype & proto = function_type.prototype();

  std::vector<ConversionSequence> conversions;
  for (size_t i(0); i < args.size(); ++i)
  {
    const auto & a = args.at(i);
    ConversionSequence conv = ConversionSequence::compute(a, proto.argv(i), engine());
    if (conv == ConversionSequence::NotConvertible())
      throw CouldNotConvert{ dpos(call->arguments.at(i)), dstr(a->type()), dstr(proto.argv(i)) };
    conversions.push_back(conv);
  }

  prepareFunctionArguments(args, proto, conversions);
  return program::FunctionVariableCall::New(functor, proto.returnType(), std::move(args));
}

std::string AbstractExpressionCompiler::repr(const std::shared_ptr<ast::Identifier> & id)
{
  if (id->type() == ast::NodeType::SimpleIdentifier)
    return id->getName();
  
  /// TODO : implemented other identifier types

  return id->getName();
}

std::shared_ptr<program::Expression> AbstractExpressionCompiler::generateExpression(const std::shared_ptr<ast::Expression> & expr)
{
  switch (expr->type())
  {
  case ast::NodeType::Operation:
    return generateOperation(std::dynamic_pointer_cast<ast::Operation>(expr));
  case ast::NodeType::SimpleIdentifier:
  case ast::NodeType::QualifiedIdentifier:
  case ast::NodeType::TemplateIdentifier:
    return generateVariableAccess(std::dynamic_pointer_cast<ast::Identifier>(expr));
  case ast::NodeType::FunctionCall:
    return generateCall(std::dynamic_pointer_cast<ast::FunctionCall>(expr));
  case ast::NodeType::BraceConstruction:
    return generateBraceConstruction(std::dynamic_pointer_cast<ast::BraceConstruction>(expr));
  case ast::NodeType::ArraySubscript:
    return generateArraySubscript(std::dynamic_pointer_cast<ast::ArraySubscript>(expr));
  case ast::NodeType::ConditionalExpression:
    return generateConditionalExpression(std::dynamic_pointer_cast<ast::ConditionalExpression>(expr));
  case ast::NodeType::ArrayExpression:
    return generateArrayConstruction(std::dynamic_pointer_cast<ast::ArrayExpression>(expr));
  case ast::NodeType::ListExpression:
    return generateListExpression(std::dynamic_pointer_cast<ast::ListExpression>(expr));
  case ast::NodeType::LambdaExpression:
    return generateLambdaExpression(std::dynamic_pointer_cast<ast::LambdaExpression>(expr));
  case ast::NodeType::BoolLiteral:
  case ast::NodeType::IntegerLiteral:
  case ast::NodeType::FloatingPointLiteral:
  case ast::NodeType::StringLiteral:
  case ast::NodeType::UserDefinedLiteral:
    return generateLiteral(std::dynamic_pointer_cast<ast::Literal>(expr));
  default:
    break;
  }


  throw std::runtime_error{ "Not impelmented" };
}

std::shared_ptr<program::Expression> AbstractExpressionCompiler::generateUserDefinedLiteral(const std::shared_ptr<ast::UserDefinedLiteral> & udl)
{
  std::string str = udl->toString();

  // suffix extraction
  auto it = str.end() - 1;
  while (parser::Lexer::isLetter(*it) || *it == '_')
    --it;
  ++it;
  std::string suffix = std::string(it, str.end());
  str.erase(it, str.end());

  Value val;
  if (str.front() == '\'' || str.front() == '"')
    val = generateStringLiteral(udl, std::move(str));
  else if (str.find('.') != str.npos || str.find('e') != str.npos)
  {
    double dval = std::stod(str);
    val = engine()->newDouble(dval);
  }
  else
  {
    int ival = std::stoi(str);
    val = engine()->newInt(ival);
  }

  engine()->manage(val);
  auto lit = program::Literal::New(val);
  std::vector<std::shared_ptr<program::Expression>> args{ lit };

  const auto & lops = getLiteralOperators(suffix);
  OverloadResolution resol = OverloadResolution::New(engine());
  if (!resol.process(lops, args))
    throw CouldNotFindValidLiteralOperator{ dpos(udl) };

  Function selected = resol.selectedOverload();
  const auto & convs = resol.conversionSequence();
  prepareFunctionArguments(args, selected.prototype(), convs);

  return program::FunctionCall::New(selected, std::move(args));
}

Value AbstractExpressionCompiler::generateStringLiteral(const std::shared_ptr<ast::Literal> & l, std::string && str)
{
  auto it = str.find("\\n");
  while (it != str.npos)
  {
    str.replace(it, it + 2, "\n");
    it = str.find("\\n");
  }
  it = str.find("\\t");
  while (it != str.npos)
  {
    str.replace(it, it + 2, "\t");
    it = str.find("\\t");
  }
  it = str.find("\\r");
  while (it != str.npos)
  {
    str.replace(it, it + 2, "\r");
    it = str.find("\\r");
  }
  it = str.find("\\0");
  while (it != str.npos)
  {
    str.replace(it, it + 2, "\0");
    it = str.find("\\0");
  }

  if (str.front() == '"')
    return engine()->newString(std::string(str.begin() + 1, str.end() - 1));

  if (str.size() != 3)
    throw InvalidCharacterLiteral{ dpos(l) };
  return engine()->newChar(str.at(1));
}

std::shared_ptr<program::Expression> AbstractExpressionCompiler::generateLiteral(const std::shared_ptr<ast::Literal> & literalExpr)
{
  if (literalExpr->is<ast::BoolLiteral>())
  {
    auto val = engine()->newBool(literalExpr->token == parser::Token::True);
    engine()->manage(val);
    return program::Literal::New(val);
  }
  else if (literalExpr->is<ast::IntegerLiteral>())
  {
    const int value = generateIntegerLiteral(std::dynamic_pointer_cast<ast::IntegerLiteral>(literalExpr));
    auto val = engine()->newInt(value);
    engine()->manage(val);
    return program::Literal::New(val);
  }
  else if (literalExpr->is<ast::FloatingPointLiteral>())
  {
    std::string str = literalExpr->toString();
    Value val;
    if (str.back() == 'f')
    {
      str.pop_back();
      float fval = std::stof(str);
      val = engine()->newFloat(fval);
    }
    else
    {
      double dval = std::stod(str);
      val = engine()->newDouble(dval);
    }
    engine()->manage(val);
    return program::Literal::New(val);
  }
  else if (literalExpr->is<ast::StringLiteral>())
  {
    std::string str = literalExpr->toString();
    Value val = generateStringLiteral(literalExpr, std::move(str));
    engine()->manage(val);
    return program::Literal::New(val);
  }
  else if (literalExpr->is<ast::UserDefinedLiteral>())
  {
    return generateUserDefinedLiteral(std::dynamic_pointer_cast<ast::UserDefinedLiteral>(literalExpr));
  }

  throw NotImplementedError{ "AbstractExpressionCompiler::generateLiteral()" };
}


NameLookup AbstractExpressionCompiler::resolve(const std::shared_ptr<ast::Identifier> & identifier)
{
  return NameLookup::resolve(identifier, this);
}

std::vector<Function> AbstractExpressionCompiler::getOperators(Operator::BuiltInOperator op, Type type, int lookup_policy)
{
  std::vector<Function> ret;

  if (type.isClosureType() || type.isFunctionType())
  {
    // these two don't have a definition scope, so we must process them separatly
    if (type.isFunctionType() && op == Operator::AssignmentOperator)
    {
      return {engine()->getFunctionType(type).assignment()};
    }
    else if (type.isClosureType() && op == Operator::FunctionCallOperator)
    {
      return { engine()->getLambda(type).function() };
    }

    return ret;
  }

  script::Scope type_decl_scope = engine()->scope(type);
  if (type.isObjectType())
  {
    script::Scope class_scope = script::Scope{ engine()->getClass(type), type_decl_scope };
    const auto & ops = getScopeOperators(op, class_scope, OperatorLookupPolicy::FetchParentOperators);
    ret.insert(ret.end(), ops.begin(), ops.end());

    Class parent = class_scope.asClass().parent();
    if (!parent.isNull())
    {
      const auto & parent_ops = getOperators(op, parent.id(), OperatorLookupPolicy::FetchParentOperators);
      ret.insert(ret.end(), parent_ops.begin(), parent_ops.end());
    }
  }
  else
  {
    const auto & ops = getScopeOperators(op, type_decl_scope, OperatorLookupPolicy::FetchParentOperators);
    ret.insert(ret.end(), ops.begin(), ops.end());
  }

  if (type.isEnumType() && op == Operator::AssignmentOperator)
    ret.insert(ret.end(), engine()->getEnum(type).getAssignmentOperator());

  if (lookup_policy & OperatorLookupPolicy::RemoveDuplicates)
    return removeDuplicates(ret);

  return ret;
}

std::shared_ptr<program::Expression> AbstractExpressionCompiler::generateOperation(const std::shared_ptr<ast::Expression> & in_op)
{
  auto operation = std::dynamic_pointer_cast<ast::Operation>(in_op);

  if (operation->operatorToken == parser::Token::Dot)
    return generateMemberAccess(operation);
  else if (operation->arg2 == nullptr)
    return generateUnaryOperation(operation);

  return generateBinaryOperation(operation);
}

std::shared_ptr<program::Expression> AbstractExpressionCompiler::generateMemberAccess(const std::shared_ptr<ast::Operation> & operation)
{
  assert(operation->operatorToken == parser::Token::Dot);

  auto object = generateExpression(operation->arg1);

  if (!object->type().isObjectType())
    throw CannotAccessMemberOfNonObject{ dpos(operation) };

  Class cla = engine()->getClass(object->type());
  const int attr_index = cla.attributeIndex(operation->arg2->as<ast::Identifier>().getName());
  if (attr_index == -1)
    throw NoSuchMember{ dpos(operation) };

  int relative_index = attr_index;
  while (relative_index - int(cla.dataMembers().size()) >= 0)
  {
    relative_index = relative_index - cla.dataMembers().size();
    cla = cla.parent();
  }

  const Type member_type = cla.dataMembers().at(relative_index).type;

  return program::MemberAccess::New(member_type, object, attr_index);
}


std::shared_ptr<program::Expression> AbstractExpressionCompiler::generateBinaryOperation(const std::shared_ptr<ast::Operation> & operation)
{
  assert(operation->arg2 != nullptr);
  assert(operation->operatorToken != parser::Token::Dot);

  auto lhs = generateExpression(operation->arg1);
  auto rhs = generateExpression(operation->arg2);

  Operator::BuiltInOperator op = ast::OperatorName::getOperatorId(operation->operatorToken, ast::OperatorName::BuiltInOpResol::InfixOp);

  const std::vector<Function> operators = getBinaryOperators(op, lhs->type(), rhs->type());

  OverloadResolution resol = OverloadResolution::New(engine());
  if (!resol.process(operators, std::vector<Type>{lhs->type(), rhs->type()}))
    throw CouldNotFindValidOperator{ dpos(operation) };

  Operator selected = resol.selectedOverload().toOperator();
  const auto & convs = resol.conversionSequence();
  std::vector<std::shared_ptr<program::Expression>> args{ lhs, rhs };
  prepareFunctionArguments(args, selected.prototype(), convs);
  return program::FunctionCall::New(selected, std::move(args));
}

std::shared_ptr<program::Expression> AbstractExpressionCompiler::generateUnaryOperation(const std::shared_ptr<ast::Operation> & operation)
{
  assert(operation->arg2 == nullptr);

  auto operand = generateExpression(operation->arg1);

  const bool postfix = operation->arg1->pos().pos < operation->operatorToken.pos;
  const auto opts = postfix ? ast::OperatorName::BuiltInOpResol::PostFixOp : ast::OperatorName::BuiltInOpResol::PrefixOp;
  Operator::BuiltInOperator op = ast::OperatorName::getOperatorId(operation->operatorToken, opts);

  const std::vector<Function> operators = getUnaryOperators(op, operand->type());

  OverloadResolution resol = OverloadResolution::New(engine());
  if (!resol.process(operators, std::vector<Type>{operand->type()}))
    throw CouldNotFindValidOperator{ dpos(operation) };

  Operator selected = resol.selectedOverload().toOperator();
  const auto & convs = resol.conversionSequence();
  std::vector<std::shared_ptr<program::Expression>> args{ operand };
  prepareFunctionArguments(args, selected.prototype(), convs);
  return program::FunctionCall::New(selected, std::move(args));
}

std::shared_ptr<program::Expression> AbstractExpressionCompiler::generateConditionalExpression(const std::shared_ptr<ast::ConditionalExpression> & ce)
{
  auto tru = generateExpression(ce->onTrue);
  auto fal = generateExpression(ce->onFalse);
  auto con = generateExpression(ce->condition);
  return program::ConditionalExpression::New(con, tru, fal);
}

std::shared_ptr<program::Expression> AbstractExpressionCompiler::generateVariableAccess(const std::shared_ptr<ast::Identifier> & identifier)
{
  return generateVariableAccess(identifier, resolve(identifier));
}

std::shared_ptr<program::Expression> AbstractExpressionCompiler::generateFunctionAccess(const std::shared_ptr<ast::Identifier> & identifier, const NameLookup & lookup)
{
  if (lookup.functions().size() != 1)
    throw AmbiguousFunctionName{ dpos(identifier) };

  Function f = lookup.functions().front();
  FunctionType ft = engine()->getFunctionType(f.prototype());
  Value val = Value::fromFunction(f, ft.type());
  engine()->manage(val);
  return program::Literal::New(val); /// TODO : perhaps a program::VariableAccess would be better ?
}

std::shared_ptr<program::Expression> AbstractExpressionCompiler::generateMemberAccess(const std::shared_ptr<program::Expression> & object, const int index)
{
  Class cla = engine()->getClass(object->type());
  int relative_index = index;
  while (relative_index - int(cla.dataMembers().size()) >= 0)
  {
    relative_index = relative_index - cla.dataMembers().size();
    cla = cla.parent();
  }

  const Type member_type = cla.dataMembers().at(index).type;
  return program::MemberAccess::New(member_type, object, index);
}



ExpressionCompiler::ExpressionCompiler(Compiler *c, CompileSession *s)
  : AbstractExpressionCompiler(c, s)
{
  mScope = engine()->rootNamespace();
}

void ExpressionCompiler::setScope(const Scope & s)
{
  mScope = s;
}

std::shared_ptr<program::Expression> ExpressionCompiler::compile(const std::shared_ptr<ast::Expression> & expr, const Context & context)
{
  mContext = context;
  mScope = Scope{ std::make_shared<ContextScope>(mContext, mScope.impl()) };
  return generateExpression(expr);
}

Scope ExpressionCompiler::currentScope() const
{
  return mScope;
}

std::vector<Function> ExpressionCompiler::getOperators(Operator::BuiltInOperator op, Type type, int lookup_policy)
{
  return AbstractExpressionCompiler::getOperators(op, type, lookup_policy);
}

std::shared_ptr<program::Expression> ExpressionCompiler::generateOperation(const std::shared_ptr<ast::Expression> & op)
{
  const ast::Operation & oper = op->as<ast::Operation>();
  if (oper.operatorToken == parser::Token::Eq)
  {
    if (oper.arg1->type() == ast::NodeType::SimpleIdentifier)
    {
      std::string name = oper.arg1->as<ast::Identifier>().getName();

      auto value = generateExpression(oper.arg2);
      return program::BindExpression::New(std::move(name), mContext, value);
    }
  }

  return AbstractExpressionCompiler::generateOperation(op);
}

std::shared_ptr<program::Expression> ExpressionCompiler::generateVariableAccess(const std::shared_ptr<ast::Identifier> & identifier, const NameLookup & lookup)
{
  switch (lookup.resultType())
  {
  case NameLookup::FunctionName:
    return generateFunctionAccess(identifier, lookup);
  case NameLookup::TemplateName:
    throw TemplateNamesAreNotExpressions{ dpos(identifier) };
  case NameLookup::TypeName:
    throw TypeNameInExpression{ dpos(identifier) };
  case NameLookup::VariableName:
    return program::Literal::New(lookup.variable()); // perhaps a VariableAccess would be better
  case NameLookup::DataMemberName:
  case NameLookup::GlobalName:
  case NameLookup::LocalName:
    assert(false);
    throw std::runtime_error{ "ExpressionCompiler::generateVariableAccess() : Implementation error" };
  case NameLookup::EnumValueName:
    return program::Literal::New(Value::fromEnumValue(lookup.enumValueResult()));
  case NameLookup::NamespaceName:
    throw NamespaceNameInExpression{ dpos(identifier) };
  default: 
    break;
  }

  throw std::runtime_error{ "Not implemented" };
}



std::shared_ptr<program::LambdaExpression> ExpressionCompiler::generateLambdaExpression(const std::shared_ptr<ast::LambdaExpression> & lambda_expr)
{
  if (lambda_expr->captures.size() > 0)
    throw LambdaMustBeCaptureless{ dpos(lambda_expr) };

  CompileLambdaTask task;
  task.lexpr = lambda_expr;
  task.scope = script::Scope{engine()->rootNamespace()};

  std::shared_ptr<LambdaCompiler> compiler{ getComponent<LambdaCompiler>() };
  LambdaCompilationResult result = compiler->compile(task);

  return result.expression;
}

} // namespace compiler

} // namespace script

