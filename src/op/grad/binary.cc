/*!
 * Copyright (c) 2019 by Contributors
 * \file src/op/grad/binary.cc
 * \brief Declaration of gradients
 */
#include "./grad_utils.h"

namespace mnm {
namespace op {
namespace grad {

using namespace mnm::ir;

Array<Expr> AddGrad(const Expr& orig_call, const Var& y, const Expr& dy) {
  // schema for relu is:
  //    x1, x2
  // schema for binary_dx is:
  //    x1, x2, y, dy
  const CallNode* call = orig_call.as<CallNode>();
  CHECK_GE(call->args.size(), 2);
  const Expr& x1 = call->args[0];
  const Expr& x2 = call->args[1];

  auto f = [&dy](const Expr& x) {
    static auto collapse_axis = Op::Get("mnm.op.get_reduce_axis");
    static auto collapse_keep = Op::Get("mnm.op.get_kept_dims");
    static auto sum = Op::Get("mnm.op.sum");
    Call axes = Call(collapse_axis, {dy, x});
    Call keep = Call(collapse_keep, {dy, x});
    return Call(sum, {dy, axes, keep});
  };

  return {f(x1), f(x2)};
}

MNM_OP_GRAD("mnm.op.add", AddGrad);

Array<Expr> MulGrad(const Expr& orig_call, const Var& y, const Expr& dy) {
  static auto op_multiply = Op::Get("mnm.op.multiply");
  const CallNode* call = orig_call.as<CallNode>();
  CHECK_GE(call->args.size(), 2);
  const Expr& x1 = call->args[0];
  const Expr& x2 = call->args[1];

  auto f = [](const Expr& dx, const Expr& x) {
    static auto collapse_axis = Op::Get("mnm.op.get_reduce_axis");
    static auto collapse_keep = Op::Get("mnm.op.get_kept_dims");
    static auto sum = Op::Get("mnm.op.sum");
    Call axes = Call(collapse_axis, {dx, x});
    Call keep = Call(collapse_keep, {dx, x});
    return Call(sum, {dx, axes, keep});
  };

  return {f(Call(op_multiply, {dy, x2}), x1), f(Call(op_multiply, {dy, x1}), x2)};
}

MNM_OP_GRAD("mnm.op.multiply", MulGrad);

Array<Expr> DivGrad(const Expr& orig_call, const Var& y, const Expr& dy) {
  static auto op_divide = Op::Get("mnm.op.divide");
  static auto op_multiply = Op::Get("mnm.op.multiply");
  static auto op_negative = Op::Get("mnm.op.negative");
  const CallNode* call = orig_call.as<CallNode>();
  CHECK_GE(call->args.size(), 2);
  const Expr& x1 = call->args[0];
  const Expr& x2 = call->args[1];
  Call x2_square = Call(op_multiply, {x2, x2});
  Call dx1 = Call(op_divide, {x2, x2_square});
  Call dx2 = Call(op_divide, {Call(op_negative, {x1}), x2_square});

  auto f = [](const Expr& dx, const Expr& x) {
    static auto collapse_axis = Op::Get("mnm.op.get_reduce_axis");
    static auto collapse_keep = Op::Get("mnm.op.get_kept_dims");
    static auto sum = Op::Get("mnm.op.sum");
    Call axes = Call(collapse_axis, {dx, x});
    Call keep = Call(collapse_keep, {dx, x});
    return Call(sum, {dx, axes, keep});
  };

  return {f(Call(op_multiply, {dy, dx1}), x1), f(Call(op_multiply, {dy, dx2}), x2)};
}

MNM_OP_GRAD("mnm.op.divide", DivGrad);

}  // namespace grad
}  // namespace op
}  // namespace mnm
