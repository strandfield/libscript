// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/compiler/expressioncompiler.h"

#include "script/compiler/compilererrors.h"
#include "script/compiler/compilesession.h"
#include "script/compiler/conversionprocessor.h"
#include "script/compiler/diagnostichelper.h"
#include "script/compiler/lambdacompiler.h"
#include "script/compiler/literalprocessor.h"
#include "script/compiler/valueconstructor.h"

#include "script/ast/ast_p.h"
#include "script/ast/node.h"

#include "script/program/expression.h"

#include "script/arraytemplate.h"
#include "script/datamember.h"
#include "script/private/engine_p.h"
#include "script/functiontype.h"
#include "script/initialization.h"
#include "script/lambda.h"
#include "script/literals.h"
#include "script/namelookup.h"
#include "script/private/namelookup_p.h"
#include "script/overloadresolution.h"
#include "script/staticdatamember.h"
#include "script/typesystem.h"

namespace script
{

namespace compiler
{

LambdaProcessor::LambdaProcessor(Compiler* c)
  : Component(c)
{

}

void LambdaProcessor::setStack(Stack* s)
{
  stack_ = s;
}

std::shared_ptr<program::LambdaExpression> LambdaProcessor::generate(ExpressionCompiler & ec, const std::shared_ptr<ast::LambdaExpression> & le)
{
  if (allowCaptures())
  {
    CompileLambdaTask task;
    task.lexpr = le;
    task.scope = ec.scope();

    const int first_capturable = 1;
    LambdaCompiler::preprocess(task, &ec, *stack_, first_capturable);

    LambdaCompiler lambda_compiler{ compiler() };

    LambdaCompilationResult result = lambda_compiler.compile(task);

    return result.expression;
  }
  else
  {
    const auto& p = le->pos();

    if (le->captures.size() > 0)
      throw CompilationFailure{ CompilerError::LambdaMustBeCaptureless };

    CompileLambdaTask task;
    task.lexpr = le;
    task.scope = script::Scope{ engine()->rootNamespace() }; /// TODO : make this customizable !

    LambdaCompiler lambda_compiler{ compiler() };

    LambdaCompilationResult result = lambda_compiler.compile(task);

    return result.expression;
  }
}

ExpressionCompiler::ExpressionCompiler(Compiler* c)
  : Component(c)
{

}

ExpressionCompiler::ExpressionCompiler(Compiler* c, const Scope & scp)
  : Component(c),
    scope_(scp)
{

}

void ExpressionCompiler::setScope(const Scope& scp)
{
  scope_ = scp;
}

void ExpressionCompiler::setCaller(const Function & func)
{
  caller_ = func;

  /// TODO : is this correct in the body of a Lambda ?
  /// TODO : add correct const-qualification
  if (caller_.isNull() || !caller_.isNonStaticMemberFunction())
    implicit_object_ = nullptr;
  else
    implicit_object_ = program::StackValue::New(1, Type::ref(caller_.memberOf().id()));
}

void ExpressionCompiler::setStack(Stack* s)
{
  stack_ = s;
  variables_.setStack(s);
}

std::vector<Function> ExpressionCompiler::getBinaryOperators(OperatorName op, Type a, Type b)
{
  return NameLookup::resolve(op, a, b, scope());
}

std::vector<Function> ExpressionCompiler::getUnaryOperators(OperatorName op, Type a)
{
  return NameLookup::resolve(op, a, scope());
}

std::vector<Function> ExpressionCompiler::getLiteralOperators(const std::string & suffix)
{
  /// TODO : improve this impl
  std::vector<Function> ret;

  auto insert_if = [&ret, &suffix](const LiteralOperator & lop) -> void {
    if (lop.suffix() == suffix)
      ret.push_back(lop);
  };

  Scope s = scope();
  while (!s.isNull())
  {
    const auto & lops = s.literalOperators();
    for (const auto & lop : lops)
      insert_if(lop);
    s = s.parent();
  }

  return ret;
}

std::vector<Function> ExpressionCompiler::getCallOperator(const Type & functor_type)
{
  std::vector<Function> result;

  if (functor_type.isObjectType())
  {
    Class cla = engine()->typeSystem()->getClass(functor_type);
    const auto & operators = cla.operators();
    for (const auto & op : operators)
    {
      if (op.operatorId() == FunctionCallOperator)
        result.push_back(op);
    }

    if (!result.empty())
      return result;

    if (!cla.parent().isNull())
      return getCallOperator(cla.parent().id());
  }
  else if (functor_type.isClosureType())
  {
    ClosureType closure = engine()->typeSystem()->getLambda(functor_type);
    return { closure.function() };
  }

  return result;
}

std::vector<std::shared_ptr<program::Expression>> ExpressionCompiler::generateExpressions(const std::vector<std::shared_ptr<ast::Expression>> & expressions)
{
  std::vector<std::shared_ptr<program::Expression>> ret;
  generateExpressions(expressions, ret);
  return ret;
}

void ExpressionCompiler::generateExpressions(const std::vector<std::shared_ptr<ast::Expression>> & in, std::vector<std::shared_ptr<program::Expression>> & out)
{
  for (const auto & e : in)
    out.push_back(generateExpression(e));
}

std::shared_ptr<program::Expression> ExpressionCompiler::generateArrayConstruction(const std::shared_ptr<ast::ArrayExpression> & array_expr)
{
  std::vector<std::shared_ptr<program::Expression>> args = generateExpressions(array_expr->elements);

  if (args.size() == 0)
    throw NotImplemented{ "ExpressionCompiler::generateArrayConstruction() : array of size 0" };

  const Type element_type = args.front()->type().baseType();
  if (element_type == Type::InitializerList)
    throw CompilationFailure{ CompilerError::InitializerListAsFirstArrayElement };

  std::vector<Conversion> conversions;
  conversions.reserve(args.size());
  for (const auto & arg : args)
  {
    auto conv = Conversion::compute(arg, element_type, engine());

    if (conv == Conversion::NotConvertible())
      throw CompilationFailure{ CompilerError::ArrayElementNotConvertible };

    conversions.push_back(conv);
  }

  auto array_template = ClassTemplate::get<ArrayTemplate>(engine());
  Class array_class = array_template.getInstance({ TemplateArgument{element_type} });

  for (size_t i(0); i < args.size(); ++i)
    args[i] = ConversionProcessor::convert(engine(), args.at(i), conversions.at(i));

  return program::ArrayExpression::New(array_class.id(), std::move(args));
}

std::shared_ptr<program::Expression> ExpressionCompiler::generateBraceConstruction(const std::shared_ptr<ast::BraceConstruction> & bc)
{
  using diagnostic::dstr;

  NameLookup lookup = resolve(bc->temporary_type);

  if (lookup.typeResult().isNull())
    throw CompilationFailure{ CompilerError::UnknownTypeInBraceInitialization, errors::InvalidName{dstr(bc->temporary_type)} };

  const Type & type = lookup.typeResult();

  if (!type.isObjectType() && bc->arguments.size() != 1)
    throw CompilationFailure{ CompilerError::TooManyArgumentInVariableInitialization };

  std::vector<std::shared_ptr<program::Expression>> args = generateExpressions(bc->arguments);

  return ValueConstructor::brace_construct(engine(), type, std::move(args));
}

std::shared_ptr<program::Expression> ExpressionCompiler::generateConstructorCall(const std::shared_ptr<ast::FunctionCall> & fc, const Type & type, std::vector<std::shared_ptr<program::Expression>> && args)
{
  return ValueConstructor::construct(engine(), type, std::move(args));
}

std::shared_ptr<program::Expression> ExpressionCompiler::generateListExpression(const std::shared_ptr<ast::ListExpression> & list_expr)
{
  auto elements = generateExpressions(list_expr->elements);
  return program::InitializerList::New(std::move(elements));
}


std::shared_ptr<program::Expression> ExpressionCompiler::generateArraySubscript(const std::shared_ptr<ast::ArraySubscript> & as)
{
  std::shared_ptr<program::Expression> obj = generateExpression(as->array);
  std::shared_ptr<program::Expression> index = generateExpression(as->index);

  const Type & objType = obj->type();
  if (!objType.isObjectType())
    throw CompilationFailure{ CompilerError::ArraySubscriptOnNonObject };

  const Type & argType = index->type();

  std::vector<Function> candidates = this->getBinaryOperators(SubscriptOperator, objType, argType);
  if (candidates.empty())
    throw CompilationFailure{ CompilerError::CouldNotFindValidSubscriptOperator };

  OverloadResolution resol = OverloadResolution::New(engine());
  if (!resol.process(candidates, std::vector<Type>{objType, argType}))
    throw CompilationFailure{ CompilerError::CouldNotFindValidSubscriptOperator };

  Function selected = resol.selectedOverload();

  std::vector<std::shared_ptr<program::Expression>> args;
  args.push_back(obj);
  args.push_back(index);

  ValueConstructor::prepare(engine(), args, selected.prototype(), resol.initializations());

  return program::FunctionCall::New(selected, std::move(args));
}

std::shared_ptr<program::Expression> ExpressionCompiler::generateCall(const std::shared_ptr<ast::FunctionCall> & call)
{
  std::vector<std::shared_ptr<program::Expression>> args = generateExpressions(call->arguments);
  const auto & callee = call->callee;

  if (callee->is<ast::Identifier>())
  {
    const std::shared_ptr<ast::Identifier> callee_name = std::static_pointer_cast<ast::Identifier>(callee);

    NameLookup lookup = NameLookup::resolve(callee_name, scope());

    if (lookup.resultType() == NameLookup::VariableName || lookup.resultType() == NameLookup::GlobalName
      || lookup.resultType() == NameLookup::DataMemberName || lookup.resultType() == NameLookup::LocalName)
    {
      auto functor = generateVariableAccess(callee_name, lookup);
      return generateFunctorCall(call, functor, std::move(args));
    }
    else if (lookup.resultType() == NameLookup::TypeName)
    {
      return generateConstructorCall(call, lookup.typeResult(), std::move(args));
    }

    return generateCall(call, callee_name, implicit_object(), args, lookup);
  }
  else if (callee->is<ast::Operation>() && callee->as<ast::Operation>().operatorToken == parser::Token::Dot)
  {
    auto member_access = std::static_pointer_cast<ast::Operation>(callee);
    assert(member_access->arg2->is<ast::Identifier>());

    auto object = generateExpression(member_access->arg1);

    const std::shared_ptr<ast::Identifier> callee_name = std::static_pointer_cast<ast::Identifier>(member_access->arg2);
    /// TODO: add an overload to NameLookup to pass the identifier directly
    std::string member_name = callee_name->is<ast::SimpleIdentifier>() ?
      callee_name->as<ast::SimpleIdentifier>().getName() : callee_name->as<ast::TemplateIdentifier>().getName();
    NameLookup lookup = NameLookup::member(member_name, engine()->typeSystem()->getClass(object->type()));
    if (lookup.resultType() == NameLookup::DataMemberName)
    {
      auto functor = VariableAccessor::generateMemberAccess(*this, object, lookup.dataMemberIndex());
      return generateFunctorCall(call, functor, std::move(args));
    }

    return generateCall(call, callee_name, object, args, lookup);
  }
  else if (callee->is<ast::Expression>())
  {
    auto functor = generateExpression(std::static_pointer_cast<ast::Expression>(callee));
    return generateFunctorCall(call, functor, std::move(args));
  }
  else
    throw std::runtime_error{ "Invalid callee / implementation error" }; /// TODO : which one

  throw std::runtime_error{ "Not implemented" };
}

std::shared_ptr<program::Expression> ExpressionCompiler::generateCall(const std::shared_ptr<ast::FunctionCall> & call, const std::shared_ptr<ast::Identifier> callee, const std::shared_ptr<program::Expression> & object, std::vector<std::shared_ptr<program::Expression>> & args, const NameLookup & lookup)
{
  using diagnostic::dstr;

  if (!lookup.impl()->functionTemplateResult.empty())
  {
    FunctionTemplateProcessor::remove_duplicates(lookup.impl()->functionTemplateResult);

    const std::vector<Type> types = getTypes(args);

    const auto& ast_targs = getTemplateArgs(callee);
    const std::vector<TemplateArgument> targs = TemplateArgumentProcessor::arguments(scope(), ast_targs);

    templates_.complete(lookup.impl()->functions, lookup.impl()->functionTemplateResult, targs, types);
  }

  assert(lookup.resultType() == NameLookup::FunctionName || lookup.resultType() == NameLookup::UnknownName || lookup.resultType() == NameLookup::TemplateName);

  if (lookup.resultType() == NameLookup::UnknownName)
    throw CompilationFailure{ CompilerError::NoSuchCallee };

  OverloadResolution resol = OverloadResolution::New(engine());
  if (!resol.process(lookup.functions(), args, object))
    throw CompilationFailure{ CompilerError::CouldNotFindValidMemberFunction };

  Function selected = resol.selectedOverload();

  if (selected.isDeleted())
    throw CompilationFailure{ CompilerError::CallToDeletedFunction };
  else if (!Accessibility::check(caller(), selected))
    throw CompilationFailure{ CompilerError::InaccessibleMember, errors::InaccessibleMember{dstr(callee), selected.accessibility()} };

  if (selected.isTemplateInstance() && (selected.native_callback() == nullptr && selected.program() == nullptr))
  {
    /// TODO: catch instantiation errors (e.g. TemplateInstantiationError)
    templates_.instantiate(selected);
  }

  if (selected.hasImplicitObject() && object != nullptr)
    args.insert(args.begin(), object);

  const auto & inits = resol.initializations();
  ValueConstructor::prepare(engine(), args, selected.prototype(), inits);
  complete(selected, args);
  if (selected.isVirtual() && callee->type() == ast::NodeType::SimpleIdentifier)
    return generateVirtualCall(call, selected, std::move(args));
  return program::FunctionCall::New(selected, std::move(args));
}

std::shared_ptr<program::Expression> ExpressionCompiler::generateVirtualCall(const std::shared_ptr<ast::FunctionCall> & call, const Function & f, std::vector<std::shared_ptr<program::Expression>> && args)
{
  assert(f.isVirtual());

  Class c = f.memberOf();
  const auto & vtable = c.vtable();
  auto it = std::find(vtable.begin(), vtable.end(), f);
  if (it == vtable.end())
    throw NotImplemented{ "Implementation error when calling virtual member" };

  auto object = args.front();
  args.erase(args.begin());
  return program::VirtualCall::New(object, std::distance(vtable.begin(), it), f.returnType(), std::move(args));
}

std::shared_ptr<program::Expression> ExpressionCompiler::generateFunctorCall(const std::shared_ptr<ast::FunctionCall> & call, const std::shared_ptr<program::Expression> & functor, std::vector<std::shared_ptr<program::Expression>> && args)
{
  if (functor->type().isFunctionType())
    return generateFunctionVariableCall(call, functor, std::move(args));
  
  std::vector<Function> functions = getCallOperator(functor->type());
  OverloadResolution resol = OverloadResolution::New(engine());

  if (!resol.process(functions, args, functor))
    throw CompilationFailure{ CompilerError::CouldNotFindValidCallOperator };

  Function selected = resol.selectedOverload();

  if (selected.isDeleted())
    throw CompilationFailure{ CompilerError::CallToDeletedFunction };
  else if (!Accessibility::check(caller(), selected))
    throw CompilationFailure{ CompilerError::InaccessibleMember, errors::InaccessibleMember{"operator()", selected.accessibility()} };

  assert(selected.isMemberFunction());
  args.insert(args.begin(), functor);
  const auto & inits = resol.initializations();
  ValueConstructor::prepare(engine(), args, selected.prototype(), inits);
  complete(selected, args);
  return program::FunctionCall::New(selected, std::move(args));
}

std::shared_ptr<program::Expression> ExpressionCompiler::generateFunctionVariableCall(const std::shared_ptr<ast::FunctionCall> & call, const std::shared_ptr<program::Expression> & functor, std::vector<std::shared_ptr<program::Expression>> && args)
{
  auto function_type = engine()->typeSystem()->getFunctionType(functor->type());
  const Prototype & proto = function_type.prototype();

  std::vector<Initialization> inits;
  for (size_t i(0); i < args.size(); ++i)
  {
    const auto & a = args.at(i);
    Initialization init = Initialization::compute(proto.at(i), a, engine());

    if (!init.isValid())
      throw CompilationFailure{ CompilerError::CouldNotConvert, errors::ConversionFailure{a->type(), proto.at(i)} };

    inits.push_back(init);
  }

  ValueConstructor::prepare(engine(), args, proto, inits);
  return program::FunctionVariableCall::New(functor, proto.returnType(), std::move(args));
}

std::shared_ptr<program::Expression> ExpressionCompiler::generateExpression(const std::shared_ptr<ast::Expression> & expr)
{
  TranslationTarget target{ this, expr };

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

std::shared_ptr<program::Expression> ExpressionCompiler::generateUserDefinedLiteral(const std::shared_ptr<ast::UserDefinedLiteral> & udl)
{
  std::string str = udl->toString();

  // suffix extraction
  std::string suffix = LiteralProcessor::take_suffix(str);
  Value val = LiteralProcessor::generate(engine(), str);
  engine()->manage(val);
  auto lit = program::Literal::New(val);
  std::vector<std::shared_ptr<program::Expression>> args{ lit };

  const auto & lops = getLiteralOperators(suffix);
  OverloadResolution resol = OverloadResolution::New(engine());

  if (!resol.process(lops, args))
    throw CompilationFailure{ CompilerError::CouldNotFindValidLiteralOperator };

  Function selected = resol.selectedOverload();
  const auto & inits = resol.initializations();
  ValueConstructor::prepare(engine(), args, selected.prototype(), inits);

  return program::FunctionCall::New(selected, std::move(args));
}

std::shared_ptr<program::LambdaExpression> ExpressionCompiler::generateLambdaExpression(const std::shared_ptr<ast::LambdaExpression> & lambda_expr)
{
  LambdaProcessor lambda{ compiler() };
  lambda.setStack(stack_);
  return lambda.generate(*this, lambda_expr);
}

std::shared_ptr<program::Expression> ExpressionCompiler::generateLiteral(const std::shared_ptr<ast::Literal> & literalExpr)
{
  if (literalExpr->is<ast::UserDefinedLiteral>())
    return generateUserDefinedLiteral(std::static_pointer_cast<ast::UserDefinedLiteral>(literalExpr));

  Value val = LiteralProcessor::generate(engine(), literalExpr);
  engine()->manage(val);
  return program::Literal::New(val);
}




NameLookup ExpressionCompiler::resolve(const std::shared_ptr<ast::Identifier> & identifier)
{
  return NameLookup::resolve(identifier, scope());
}

std::vector<Type> ExpressionCompiler::getTypes(const std::vector<std::shared_ptr<program::Expression>>& exprs)
{
  std::vector<Type> ret;
  ret.reserve(exprs.size());

  for (const auto& e : exprs)
  {
    ret.push_back(e->type());
  }

  return ret;
}

const std::vector<std::shared_ptr<ast::Node>>& ExpressionCompiler::getTemplateArgs(const std::shared_ptr<ast::Identifier>& id)
{
  static const std::vector<std::shared_ptr<ast::Node>> static_instance = {};

  if (id->is<ast::TemplateIdentifier>())
  {
    return id->as<ast::TemplateIdentifier>().arguments;
  }
  else if (id->is<ast::ScopedIdentifier>())
  {
    return getTemplateArgs(id->as<ast::ScopedIdentifier>().rhs);
  }

  return static_instance;
}

void ExpressionCompiler::complete(const Function & f, std::vector<std::shared_ptr<program::Expression>> & args)
{
  size_t diff = size_t(f.prototype().count()) - args.size();

  if (diff == 0)
    return;

  const auto & defaults = f.defaultArguments();
  while (diff-- > 0)
  {
    args.push_back(defaults.at(diff));
  }
}

std::shared_ptr<program::Expression> ExpressionCompiler::generateOperation(const std::shared_ptr<ast::Expression> & in_op)
{
  auto operation = std::dynamic_pointer_cast<ast::Operation>(in_op);

  if (operation->operatorToken == parser::Token::Dot)
    return generateMemberAccess(operation);
  else if (operation->arg2 == nullptr)
    return generateUnaryOperation(operation);

  return generateBinaryOperation(operation);
}

std::shared_ptr<program::Expression> ExpressionCompiler::generateMemberAccess(const std::shared_ptr<ast::Operation> & operation)
{
  assert(operation->operatorToken == parser::Token::Dot);

  auto object = generateExpression(operation->arg1);

  if (!object->type().isObjectType())
    throw CompilationFailure{ CompilerError::CannotAccessMemberOfNonObject };

  Class cla = engine()->typeSystem()->getClass(object->type());
  const std::string mname = operation->arg2->as<ast::SimpleIdentifier>().getName();
  const int attr_index = cla.attributeIndex(mname);

  if (attr_index == -1)
    throw CompilationFailure{ CompilerError::NoSuchMember, errors::DataMemberName{mname} };

  return VariableAccessor::generateMemberAccess(*this, object, attr_index);
}


std::shared_ptr<program::Expression> ExpressionCompiler::generateBinaryOperation(const std::shared_ptr<ast::Operation> & operation)
{
  assert(operation->arg2 != nullptr);
  assert(operation->operatorToken != parser::Token::Dot);

  auto lhs = generateExpression(operation->arg1);
  auto rhs = generateExpression(operation->arg2);

  OperatorName op = ast::OperatorName::getOperatorId(operation->operatorToken, ast::OperatorName::BuiltInOpResol::InfixOp);

  const std::vector<Function> operators = getBinaryOperators(op, lhs->type(), rhs->type());

  OverloadResolution resol = OverloadResolution::New(engine());
  if (!resol.process(operators, std::vector<Type>{lhs->type(), rhs->type()}))
    throw CompilationFailure{ CompilerError::CouldNotFindValidOperator };

  Operator selected = resol.selectedOverload().toOperator();
  std::vector<std::shared_ptr<program::Expression>> args{ lhs, rhs };
  const auto & inits = resol.initializations();
  ValueConstructor::prepare(engine(), args, selected.prototype(), inits);
  return program::FunctionCall::New(selected, std::move(args));
}

std::shared_ptr<program::Expression> ExpressionCompiler::generateUnaryOperation(const std::shared_ptr<ast::Operation> & operation)
{
  assert(operation->arg2 == nullptr);

  auto operand = generateExpression(operation->arg1);

  const bool postfix = operation->arg1->pos().pos < operation->operatorToken.pos;
  const auto opts = postfix ? ast::OperatorName::BuiltInOpResol::PostFixOp : ast::OperatorName::BuiltInOpResol::PrefixOp;
  OperatorName op = ast::OperatorName::getOperatorId(operation->operatorToken, opts);

  const std::vector<Function> operators = getUnaryOperators(op, operand->type());

  OverloadResolution resol = OverloadResolution::New(engine());
  if (!resol.process(operators, std::vector<Type>{operand->type()}))
    throw CompilationFailure{ CompilerError::CouldNotFindValidOperator };

  Operator selected = resol.selectedOverload().toOperator();

  if (selected.isDeleted())
    throw CompilationFailure{ CompilerError::CallToDeletedFunction };
  else if (!Accessibility::check(caller(), selected))
    throw CompilationFailure{ CompilerError::InaccessibleMember, errors::InaccessibleMember{Operator::getFullName(selected.operatorId()), selected.accessibility()} };

  std::vector<std::shared_ptr<program::Expression>> args{ operand };
  const auto & inits = resol.initializations();
  ValueConstructor::prepare(engine(), args, selected.prototype(), inits);
  return program::FunctionCall::New(selected, std::move(args));
}

std::shared_ptr<program::Expression> ExpressionCompiler::generateConditionalExpression(const std::shared_ptr<ast::ConditionalExpression> & ce)
{
  auto con = generateExpression(ce->condition);
  if (con->type().baseType() != Type::Boolean)
    throw NotImplemented{ "Condition of ?: must be of type bool" };

  auto tru = generateExpression(ce->onTrue);
  auto fal = generateExpression(ce->onFalse);
  script::Type common_type = ConversionProcessor::common_type(engine(), tru, fal);

  if (common_type.isNull())
    throw CompilationFailure{ CompilerError::CouldNotFindCommonType, errors::NoCommonType{tru->type(), fal->type()} };

  tru = ConversionProcessor::convert(engine(), tru, Conversion::compute(tru, common_type, engine()));
  fal = ConversionProcessor::convert(engine(), fal, Conversion::compute(fal, common_type, engine()));

  return program::ConditionalExpression::New(con, tru, fal);
}

std::shared_ptr<program::Expression> ExpressionCompiler::generateVariableAccess(const std::shared_ptr<ast::Identifier> & identifier)
{
  return generateVariableAccess(identifier, resolve(identifier));
}

std::shared_ptr<program::Expression> ExpressionCompiler::generateVariableAccess(const std::shared_ptr<ast::Identifier> & identifier, const NameLookup & lookup)
{
  switch (lookup.resultType())
  {
  case NameLookup::FunctionName:
    return generateFunctionAccess(identifier, lookup);
  case NameLookup::TemplateName:
    throw CompilationFailure{ CompilerError::TemplateNamesAreNotExpressions };
  case NameLookup::TypeName:
    throw CompilationFailure{ CompilerError::TypeNameInExpression };
  case NameLookup::VariableName:
    return program::VariableAccess::New(lookup.variable());
  case NameLookup::StaticDataMemberName:
    return generateStaticDataMemberAccess(identifier, lookup);
  case NameLookup::DataMemberName:
    return variables_.accessDataMember(*this, lookup.dataMemberIndex());
  case NameLookup::GlobalName:
    return variables_.accessGlobal(*this, lookup.globalIndex());
  case NameLookup::LocalName:
    return variables_.accessLocal(*this, lookup.localIndex());
  case NameLookup::CaptureName:
    return variables_.accessCapture(*this, lookup.captureIndex());
  case NameLookup::EnumValueName:
    return program::Literal::New(Value::fromEnumerator(lookup.enumeratorResult()));
  case NameLookup::NamespaceName:
    throw CompilationFailure{ CompilerError::NamespaceNameInExpression };
  default:
    break;
  }

  throw NotImplemented{ "ExpressionCompiler::generateVariableAccess() : kind of variable not implemented" };
}

std::shared_ptr<program::Expression> ExpressionCompiler::generateFunctionAccess(const std::shared_ptr<ast::Identifier> & identifier, const NameLookup & lookup)
{
  if (lookup.functions().size() != 1)
    throw CompilationFailure{ CompilerError::AmbiguousFunctionName };

  Function f = lookup.functions().front();
  FunctionType ft = engine()->typeSystem()->getFunctionType(f.prototype());
  Value val = Value::fromFunction(f, ft.type());
  engine()->manage(val);
  return program::Literal::New(val); /// TODO : perhaps a program::VariableAccess would be better ?
}

std::shared_ptr<program::Expression> ExpressionCompiler::generateStaticDataMemberAccess(const std::shared_ptr<ast::Identifier> & id, const NameLookup & lookup)
{
  const Class c = lookup.memberOf();
  const Class::StaticDataMember & sdm = lookup.staticDataMemberResult();

  if (!Accessibility::check(caller(), c, sdm.accessibility()))
    throw CompilationFailure{ CompilerError::InaccessibleMember, errors::InaccessibleMember{ sdm.name, sdm.accessibility()} };

  return program::VariableAccess::New(sdm.value);
}

} // namespace compiler

} // namespace script

