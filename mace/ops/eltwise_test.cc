// Copyright 2018 Xiaomi, Inc.  All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "mace/kernels/eltwise.h"
#include "mace/core/operator.h"
#include "mace/ops/ops_test_util.h"

namespace mace {
namespace ops {
namespace test {

class EltwiseOpTest : public OpsTestBase {};

namespace {
template <DeviceType D, typename T>
void SimpleTensorScalar(const kernels::EltwiseType type,
                        const std::vector<index_t> &shape,
                        const std::vector<float> &input,
                        const float x,
                        const std::vector<float> &output) {
  // Construct graph
  OpsTestNet net;

  // Add input data
  net.AddInputFromArray<D, float>("Input", shape, input);

  if (D == DeviceType::CPU) {
    net.TransformDataFormat<D, float>("Input", NHWC, "TInput", NCHW);
    OpDefBuilder("Eltwise", "EltwiseTest")
        .Input("TInput")
        .AddIntArg("type", static_cast<int>(type))
        .AddFloatArg("value", x)
        .Output("TOutput")
        .Finalize(net.NewOperatorDef());
    // Run
    net.RunOp(D);
    net.TransformDataFormat<D, float>("TOutput", NCHW, "Output", NHWC);
  } else {
    BufferToImage<D, T>(&net, "Input", "InputImg",
                        kernels::BufferType::IN_OUT_CHANNEL);
    OpDefBuilder("Eltwise", "EltwiseTest")
        .Input("InputImg")
        .AddIntArg("type", static_cast<int>(type))
        .AddFloatArg("value", x)
        .Output("OutputImg")
        .Finalize(net.NewOperatorDef());

    // Run
    net.RunOp(D);

    ImageToBuffer<D, float>(&net, "OutputImg", "Output",
                            kernels::BufferType::IN_OUT_CHANNEL);
  }

  auto expected = CreateTensor<float>(shape, output);

  ExpectTensorNear<float>(*expected, *net.GetOutput("Output"), 1e-5);
}

template <DeviceType D, typename T>
void SimpleTensorEltwise(const kernels::EltwiseType type,
                         const std::vector<index_t> &shape0,
                         const std::vector<float> &input0,
                         const std::vector<index_t> &shape1,
                         const std::vector<float> &input1,
                         const std::vector<float> &output,
                         const std::vector<float> &coeff = {}) {
  // Construct graph
  OpsTestNet net;

  // Add input data
  net.AddInputFromArray<D, float>("Input0", shape0, input0);
  net.AddInputFromArray<D, float>("Input1", shape1, input1);

  if (D == DeviceType::CPU) {
    net.TransformDataFormat<D, float>("Input0", NHWC, "TInput0", NCHW);
    net.TransformDataFormat<D, float>("Input1", NHWC, "TInput1", NCHW);
    OpDefBuilder("Eltwise", "EltwiseTest")
        .Input("TInput0")
        .Input("TInput1")
        .AddIntArg("type", static_cast<int>(type))
        .AddFloatsArg("coeff", coeff)
        .Output("TOutput")
        .Finalize(net.NewOperatorDef());

    // Run
    net.RunOp(D);
    net.TransformDataFormat<D, float>("TOutput", NCHW, "Output", NHWC);
  } else {
    BufferToImage<D, T>(&net, "Input0", "InputImg0",
                        kernels::BufferType::IN_OUT_CHANNEL);
    BufferToImage<D, T>(&net, "Input1", "InputImg1",
                        kernels::BufferType::IN_OUT_CHANNEL);
    OpDefBuilder("Eltwise", "EltwiseTest")
        .Input("InputImg0")
        .Input("InputImg1")
        .AddIntArg("type", static_cast<int>(type))
        .AddFloatsArg("coeff", coeff)
        .Output("OutputImg")
        .Finalize(net.NewOperatorDef());

    // Run
    net.RunOp(D);

    ImageToBuffer<D, float>(&net, "OutputImg", "Output",
                            kernels::BufferType::IN_OUT_CHANNEL);
  }

  std::vector<index_t> output_shape = shape0;
  if (input0.size() < input1.size()) {
    output_shape = shape1;
  }
  auto expected = CreateTensor<float>(output_shape, output);

  ExpectTensorNear<float>(*expected, *net.GetOutput("Output"), 1e-5);
}
}  // namespace

TEST_F(EltwiseOpTest, CPUSimpleTensorScalar) {
  SimpleTensorScalar<DeviceType::CPU, float>(kernels::EltwiseType::SUM,
                                             {1, 1, 1, 1}, {1}, 1, {2});
  SimpleTensorScalar<DeviceType::CPU, float>(kernels::EltwiseType::SUB,
                                             {1, 1, 2, 3}, {1, 2, 3, 4, 5, 6},
                                             1, {0, 1, 2, 3, 4, 5});
  SimpleTensorScalar<DeviceType::CPU, float>(kernels::EltwiseType::PROD,
                                             {1, 1, 2, 3}, {1, 2, 3, 4, 5, 6},
                                             2, {2, 4, 6, 8, 10, 12});
  SimpleTensorScalar<DeviceType::CPU, float>(kernels::EltwiseType::DIV,
                                             {1, 1, 2, 3}, {2, 4, 6, 8, 10, 12},
                                             2, {1, 2, 3, 4, 5, 6});
  SimpleTensorScalar<DeviceType::CPU, float>(kernels::EltwiseType::MIN,
                                             {1, 1, 2, 3}, {1, 2, 3, 4, 5, 6},
                                             1, {1, 1, 1, 1, 1, 1});
  SimpleTensorScalar<DeviceType::CPU, float>(kernels::EltwiseType::MAX,
                                             {1, 1, 2, 3}, {1, 2, 3, 4, 5, 6},
                                             3, {3, 3, 3, 4, 5, 6});
  SimpleTensorScalar<DeviceType::CPU, float>(kernels::EltwiseType::NEG,
                                             {1, 1, 2, 3}, {1, 2, 3, 4, 5, 6},
                                             3, {-1, -2, -3, -4, -5, -6});
  SimpleTensorScalar<DeviceType::CPU, float>(
      kernels::EltwiseType::ABS, {1, 1, 2, 3}, {-1, -2, -3, -4, -5, -6}, 3,
      {1, 2, 3, 4, 5, 6});
  SimpleTensorScalar<DeviceType::CPU, float>(kernels::EltwiseType::SQR_DIFF,
                                             {1, 1, 2, 3}, {1, 2, 3, 4, 5, 6},
                                             1, {0, 1, 4, 9, 16, 25});
}

TEST_F(EltwiseOpTest, GPUSimpleTensorScalar) {
  SimpleTensorScalar<DeviceType::GPU, float>(kernels::EltwiseType::SUM,
                                             {1, 1, 1, 1}, {1}, 1, {2});
  SimpleTensorScalar<DeviceType::GPU, float>(kernels::EltwiseType::SUB,
                                             {1, 1, 2, 3}, {1, 2, 3, 4, 5, 6},
                                             1, {0, 1, 2, 3, 4, 5});
  SimpleTensorScalar<DeviceType::GPU, float>(kernels::EltwiseType::PROD,
                                             {1, 1, 2, 3}, {1, 2, 3, 4, 5, 6},
                                             2, {2, 4, 6, 8, 10, 12});
  SimpleTensorScalar<DeviceType::GPU, float>(kernels::EltwiseType::DIV,
                                             {1, 1, 2, 3}, {2, 4, 6, 8, 10, 12},
                                             2, {1, 2, 3, 4, 5, 6});
  SimpleTensorScalar<DeviceType::GPU, float>(kernels::EltwiseType::MIN,
                                             {1, 1, 2, 3}, {1, 2, 3, 4, 5, 6},
                                             1, {1, 1, 1, 1, 1, 1});
  SimpleTensorScalar<DeviceType::GPU, float>(kernels::EltwiseType::MAX,
                                             {1, 1, 2, 3}, {1, 2, 3, 4, 5, 6},
                                             3, {3, 3, 3, 4, 5, 6});
  SimpleTensorScalar<DeviceType::GPU, float>(kernels::EltwiseType::NEG,
                                             {1, 1, 2, 3}, {1, 2, 3, 4, 5, 6},
                                             3, {-1, -2, -3, -4, -5, -6});
  SimpleTensorScalar<DeviceType::GPU, float>(
      kernels::EltwiseType::ABS, {1, 1, 2, 3}, {-1, -2, -3, -4, -5, -6}, 3,
      {1, 2, 3, 4, 5, 6});
  SimpleTensorScalar<DeviceType::GPU, float>(kernels::EltwiseType::SQR_DIFF,
                                             {1, 1, 2, 3}, {1, 2, 3, 4, 5, 6},
                                             1, {0, 1, 4, 9, 16, 25});
}

TEST_F(EltwiseOpTest, CPUSimpleTensorVector) {
  SimpleTensorEltwise<DeviceType::CPU, float>(
      kernels::EltwiseType::SUM, {1, 1, 2, 3}, {1, 2, 3, 4, 5, 6}, {1, 1, 1, 3},
      {1, 2, 3}, {2, 4, 6, 5, 7, 9});
  SimpleTensorEltwise<DeviceType::CPU, float>(
      kernels::EltwiseType::SUB, {1, 2, 1, 5}, {1, 2, 3, 4, 5, 6, 7, 8, 9, 10},
      {1, 1, 1, 5}, {1, 2, 3, 4, 5}, {0, 0, 0, 0, 0, 5, 5, 5, 5, 5});
  SimpleTensorEltwise<DeviceType::CPU, float>(
      kernels::EltwiseType::SUB, {1, 1, 1, 5}, {1, 2, 3, 4, 5}, {1, 2, 1, 5},
      {1, 2, 3, 4, 5, 6, 7, 8, 9, 10}, {0, 0, 0, 0, 0, -5, -5, -5, -5, -5});
  SimpleTensorEltwise<DeviceType::CPU, float>(
      kernels::EltwiseType::PROD, {1, 1, 1, 3}, {1, 2, 3}, {1, 2, 1, 3},
      {1, 2, 3, 4, 5, 6}, {1, 4, 9, 4, 10, 18});
  SimpleTensorEltwise<DeviceType::CPU, float>(
      kernels::EltwiseType::DIV, {1, 2, 1, 5}, {1, 2, 3, 4, 5, 6, 7, 8, 9, 10},
      {1, 1, 1, 5}, {1, 1, 1, 1, 5}, {1, 2, 3, 4, 1, 6, 7, 8, 9, 2});
  SimpleTensorEltwise<DeviceType::CPU, float>(
      kernels::EltwiseType::DIV, {1, 1, 1, 5}, {1, 1, 1, 2, 4}, {1, 2, 1, 5},
      {1, 1, 1, 2, 2, 1, 1, 1, 1, 1}, {1, 1, 1, 1, 2, 1, 1, 1, 2, 4});
  SimpleTensorEltwise<DeviceType::CPU, float>(
      kernels::EltwiseType::MIN, {1, 1, 1, 5}, {1, 2, 3, 4, 5}, {1, 2, 1, 5},
      {1, 2, 3, 4, 5, 6, 7, 8, 9, 10}, {1, 2, 3, 4, 5, 1, 2, 3, 4, 5});
  SimpleTensorEltwise<DeviceType::CPU, float>(
      kernels::EltwiseType::MAX, {1, 2, 1, 5}, {1, 2, 3, 4, 5, 6, 7, 8, 9, 10},
      {1, 1, 1, 5}, {1, 2, 3, 4, 5}, {1, 2, 3, 4, 5, 6, 7, 8, 9, 10});
  SimpleTensorEltwise<DeviceType::CPU, float>(
      kernels::EltwiseType::SQR_DIFF, {1, 1, 1, 5}, {1, 2, 3, 4, 5},
      {1, 2, 1, 5}, {1, 2, 3, 4, 5, 6, 7, 8, 9, 10},
      {0, 0, 0, 0, 0, 25, 25, 25, 25, 25});
}

TEST_F(EltwiseOpTest, GPUSimpleTensorVector) {
  SimpleTensorEltwise<DeviceType::GPU, float>(
      kernels::EltwiseType::SUM, {1, 1, 2, 3}, {1, 2, 3, 4, 5, 6}, {1, 1, 1, 3},
      {1, 2, 3}, {2, 4, 6, 5, 7, 9});
  SimpleTensorEltwise<DeviceType::GPU, float>(
      kernels::EltwiseType::SUB, {1, 2, 1, 5}, {1, 2, 3, 4, 5, 6, 7, 8, 9, 10},
      {1, 1, 1, 5}, {1, 2, 3, 4, 5}, {0, 0, 0, 0, 0, 5, 5, 5, 5, 5});
  SimpleTensorEltwise<DeviceType::GPU, float>(
      kernels::EltwiseType::SUB, {1, 1, 1, 5}, {1, 2, 3, 4, 5}, {1, 2, 1, 5},
      {1, 2, 3, 4, 5, 6, 7, 8, 9, 10}, {0, 0, 0, 0, 0, -5, -5, -5, -5, -5});
  SimpleTensorEltwise<DeviceType::GPU, float>(
      kernels::EltwiseType::PROD, {1, 1, 1, 3}, {1, 2, 3}, {1, 2, 1, 3},
      {1, 2, 3, 4, 5, 6}, {1, 4, 9, 4, 10, 18});
  SimpleTensorEltwise<DeviceType::GPU, float>(
      kernels::EltwiseType::DIV, {1, 2, 1, 5}, {1, 2, 3, 4, 5, 6, 7, 8, 9, 10},
      {1, 1, 1, 5}, {1, 1, 1, 1, 5}, {1, 2, 3, 4, 1, 6, 7, 8, 9, 2});
  SimpleTensorEltwise<DeviceType::GPU, float>(
      kernels::EltwiseType::DIV, {1, 1, 1, 5}, {1, 1, 1, 2, 4}, {1, 2, 1, 5},
      {1, 1, 1, 2, 2, 1, 1, 1, 1, 1}, {1, 1, 1, 1, 2, 1, 1, 1, 2, 4});
  SimpleTensorEltwise<DeviceType::GPU, float>(
      kernels::EltwiseType::MIN, {1, 1, 1, 5}, {1, 2, 3, 4, 5}, {1, 2, 1, 5},
      {1, 2, 3, 4, 5, 6, 7, 8, 9, 10}, {1, 2, 3, 4, 5, 1, 2, 3, 4, 5});
  SimpleTensorEltwise<DeviceType::GPU, float>(
      kernels::EltwiseType::MAX, {1, 2, 1, 5}, {1, 2, 3, 4, 5, 6, 7, 8, 9, 10},
      {1, 1, 1, 5}, {1, 2, 3, 4, 5}, {1, 2, 3, 4, 5, 6, 7, 8, 9, 10});
  SimpleTensorEltwise<DeviceType::GPU, float>(
      kernels::EltwiseType::SQR_DIFF, {1, 1, 1, 5}, {1, 2, 3, 4, 5},
      {1, 2, 1, 5}, {1, 2, 3, 4, 5, 6, 7, 8, 9, 10},
      {0, 0, 0, 0, 0, 25, 25, 25, 25, 25});
}

TEST_F(EltwiseOpTest, CPUSimpleTensorTensor) {
  SimpleTensorEltwise<DeviceType::CPU, float>(
      kernels::EltwiseType::SUM, {1, 1, 2, 3}, {1, 2, 3, 4, 5, 6}, {1, 1, 2, 3},
      {1, 2, 3, 4, 5, 6}, {2, 4, 6, 8, 10, 12});
  SimpleTensorEltwise<DeviceType::CPU, float>(
      kernels::EltwiseType::SUM, {1, 1, 2, 3}, {1, 2, 3, 4, 5, 6}, {1, 1, 2, 3},
      {1, 2, 3, 4, 5, 6}, {0.2, 0.4, 0.6, 0.8, 1, 1.2}, {0.1, 0.1});
  SimpleTensorEltwise<DeviceType::CPU, float>(
      kernels::EltwiseType::SUB, {1, 1, 1, 5}, {1, 2, 3, 4, 5}, {1, 1, 1, 5},
      {1, 2, 3, 4, 5}, {0, 0, 0, 0, 0});
  SimpleTensorEltwise<DeviceType::CPU, float>(
      kernels::EltwiseType::PROD, {1, 2, 1, 3}, {1, 2, 3, 4, 5, 6},
      {1, 2, 1, 3}, {1, 2, 3, 4, 5, 6}, {1, 4, 9, 16, 25, 36});
  SimpleTensorEltwise<DeviceType::CPU, float>(
      kernels::EltwiseType::DIV, {1, 2, 1, 3}, {1, 2, 3, 4, 5, 6}, {1, 2, 1, 3},
      {1, 2, 3, 4, 5, 6}, {1, 1, 1, 1, 1, 1});
  SimpleTensorEltwise<DeviceType::CPU, float>(
      kernels::EltwiseType::MIN, {1, 2, 1, 5}, {1, 2, 3, 4, 5, 1, 2, 3, 4, 5},
      {1, 2, 1, 5}, {1, 2, 3, 4, 5, 6, 7, 8, 9, 10},
      {1, 2, 3, 4, 5, 1, 2, 3, 4, 5});
  SimpleTensorEltwise<DeviceType::CPU, float>(
      kernels::EltwiseType::MAX, {1, 2, 1, 5}, {1, 2, 3, 4, 5, 6, 7, 8, 9, 10},
      {1, 2, 1, 5}, {1, 2, 3, 4, 5, 1, 2, 3, 4, 5},
      {1, 2, 3, 4, 5, 6, 7, 8, 9, 10});
  SimpleTensorEltwise<DeviceType::CPU, float>(
      kernels::EltwiseType::SQR_DIFF, {1, 2, 1, 5},
      {1, 2, 3, 4, 5, 1, 2, 3, 4, 5}, {1, 2, 1, 5},
      {1, 2, 3, 4, 5, 6, 7, 8, 9, 10}, {0, 0, 0, 0, 0, 25, 25, 25, 25, 25});
}
TEST_F(EltwiseOpTest, GPUSimpleTensorTensor) {
  SimpleTensorEltwise<DeviceType::GPU, float>(
      kernels::EltwiseType::SUM, {1, 1, 2, 3}, {1, 2, 3, 4, 5, 6}, {1, 1, 2, 3},
      {1, 2, 3, 4, 5, 6}, {2, 4, 6, 8, 10, 12});
  SimpleTensorEltwise<DeviceType::GPU, float>(
      kernels::EltwiseType::SUM, {1, 1, 2, 3}, {1, 2, 3, 4, 5, 6}, {1, 1, 2, 3},
      {1, 2, 3, 4, 5, 6}, {0.2, 0.4, 0.6, 0.8, 1, 1.2}, {0.1, 0.1});
  SimpleTensorEltwise<DeviceType::GPU, float>(
      kernels::EltwiseType::SUB, {1, 1, 1, 5}, {1, 2, 3, 4, 5}, {1, 1, 1, 5},
      {1, 2, 3, 4, 5}, {0, 0, 0, 0, 0});
  SimpleTensorEltwise<DeviceType::GPU, float>(
      kernels::EltwiseType::PROD, {1, 2, 1, 3}, {1, 2, 3, 4, 5, 6},
      {1, 2, 1, 3}, {1, 2, 3, 4, 5, 6}, {1, 4, 9, 16, 25, 36});
  SimpleTensorEltwise<DeviceType::GPU, float>(
      kernels::EltwiseType::DIV, {1, 2, 1, 3}, {1, 2, 3, 4, 5, 6}, {1, 2, 1, 3},
      {1, 2, 3, 4, 5, 6}, {1, 1, 1, 1, 1, 1});
  SimpleTensorEltwise<DeviceType::GPU, float>(
      kernels::EltwiseType::MIN, {1, 2, 1, 5}, {1, 2, 3, 4, 5, 1, 2, 3, 4, 5},
      {1, 2, 1, 5}, {1, 2, 3, 4, 5, 6, 7, 8, 9, 10},
      {1, 2, 3, 4, 5, 1, 2, 3, 4, 5});
  SimpleTensorEltwise<DeviceType::GPU, float>(
      kernels::EltwiseType::MAX, {1, 2, 1, 5}, {1, 2, 3, 4, 5, 6, 7, 8, 9, 10},
      {1, 2, 1, 5}, {1, 2, 3, 4, 5, 1, 2, 3, 4, 5},
      {1, 2, 3, 4, 5, 6, 7, 8, 9, 10});
  SimpleTensorEltwise<DeviceType::GPU, float>(
      kernels::EltwiseType::SQR_DIFF, {1, 2, 1, 5},
      {1, 2, 3, 4, 5, 1, 2, 3, 4, 5}, {1, 2, 1, 5},
      {1, 2, 3, 4, 5, 6, 7, 8, 9, 10}, {0, 0, 0, 0, 0, 25, 25, 25, 25, 25});
}

namespace {
template <typename T>
void RandomTensorScalar(const kernels::EltwiseType type,
                        const std::vector<index_t> &shape) {
  // Construct graph
  OpsTestNet net;

  // Add input data
  net.AddRandomInput<DeviceType::GPU, float>("Input", shape, true, true);

  net.TransformDataFormat<DeviceType::CPU, float>("Input", NHWC, "TInput",
                                                  NCHW);
  OpDefBuilder("Eltwise", "EltwiseTest")
      .Input("TInput")
      .AddIntArg("type", static_cast<int>(type))
      .AddFloatArg("value", 0.1)
      .Output("TOutput")
      .Finalize(net.NewOperatorDef());
  // Run
  net.RunOp(DeviceType::CPU);
  net.TransformDataFormat<DeviceType::CPU, float>("TOutput", NCHW, "Output",
                                                  NHWC);
  Tensor expected;
  expected.Copy(*net.GetOutput("Output"));

  BufferToImage<DeviceType::GPU, T>(&net, "Input", "InputImg",
                                    kernels::BufferType::IN_OUT_CHANNEL);
  OpDefBuilder("Eltwise", "EltwiseTest")
      .Input("InputImg")
      .AddIntArg("type", static_cast<int>(type))
      .AddFloatArg("value", 0.1)
      .Output("OutputImg")
      .AddIntArg("T", static_cast<int>(DataTypeToEnum<T>::value))
      .Finalize(net.NewOperatorDef());

  // Run
  net.RunOp(DeviceType::GPU);

  ImageToBuffer<DeviceType::GPU, float>(&net, "OutputImg", "GPUOutput",
                                        kernels::BufferType::IN_OUT_CHANNEL);

  if (DataTypeToEnum<T>::value == DT_FLOAT) {
    ExpectTensorNear<float>(expected, *net.GetOutput("GPUOutput"), 1e-5);
  } else {
    ExpectTensorNear<float>(expected, *net.GetOutput("GPUOutput"), 1e-2, 1e-2);
  }
}

template <typename T>
void RandomTensorEltwise(const kernels::EltwiseType type,
                         const std::vector<index_t> &shape0,
                         const std::vector<index_t> &shape1,
                         const std::vector<float> &coeff = {}) {
  // Construct graph
  OpsTestNet net;

  // Add input data
  net.AddRandomInput<DeviceType::GPU, float>("Input0", shape0, true, true);
  net.AddRandomInput<DeviceType::GPU, float>("Input1", shape1, true, true);

  net.TransformDataFormat<DeviceType::CPU, float>("Input0", NHWC, "TInput0",
                                                  NCHW);
  net.TransformDataFormat<DeviceType::CPU, float>("Input1", NHWC, "TInput1",
                                                  NCHW);
  OpDefBuilder("Eltwise", "EltwiseTest")
      .Input("TInput0")
      .Input("TInput1")
      .AddIntArg("type", static_cast<int>(type))
      .AddFloatsArg("coeff", coeff)
      .Output("TOutput")
      .Finalize(net.NewOperatorDef());

  // Run
  net.RunOp(DeviceType::CPU);
  net.TransformDataFormat<DeviceType::CPU, float>("TOutput", NCHW, "Output",
                                                  NHWC);
  Tensor expected;
  expected.Copy(*net.GetOutput("Output"));

  BufferToImage<DeviceType::GPU, T>(&net, "Input0", "InputImg0",
                                    kernels::BufferType::IN_OUT_CHANNEL);
  BufferToImage<DeviceType::GPU, T>(&net, "Input1", "InputImg1",
                                    kernels::BufferType::IN_OUT_CHANNEL);
  OpDefBuilder("Eltwise", "EltwiseTest")
      .Input("InputImg0")
      .Input("InputImg1")
      .AddIntArg("type", static_cast<int>(type))
      .AddFloatsArg("coeff", coeff)
      .AddIntArg("T", static_cast<int>(DataTypeToEnum<T>::value))
      .Output("OutputImg")
      .Finalize(net.NewOperatorDef());

  // Run
  net.RunOp(DeviceType::GPU);

  ImageToBuffer<DeviceType::GPU, float>(&net, "OutputImg", "GPUOutput",
                                        kernels::BufferType::IN_OUT_CHANNEL);

  if (DataTypeToEnum<T>::value == DT_FLOAT) {
    ExpectTensorNear<float>(expected, *net.GetOutput("GPUOutput"), 1e-5);
  } else {
    ExpectTensorNear<float>(expected, *net.GetOutput("GPUOutput"), 1e-2, 1e-2);
  }
}
}  // namespace

TEST_F(EltwiseOpTest, RandomTensorScalarFloat) {
  RandomTensorScalar<float>(kernels::EltwiseType::SUM, {1, 32, 32, 16});
  RandomTensorScalar<float>(kernels::EltwiseType::SUB, {3, 32, 32, 16});
  RandomTensorScalar<float>(kernels::EltwiseType::PROD, {1, 31, 37, 17});
  RandomTensorScalar<float>(kernels::EltwiseType::DIV, {3, 31, 37, 17});
  RandomTensorScalar<float>(kernels::EltwiseType::MIN, {1, 32, 32, 16});
  RandomTensorScalar<float>(kernels::EltwiseType::MAX, {3, 31, 37, 17});
  RandomTensorScalar<float>(kernels::EltwiseType::NEG, {1, 32, 32, 32});
  RandomTensorScalar<float>(kernels::EltwiseType::ABS, {3, 31, 37, 17});
  RandomTensorScalar<float>(kernels::EltwiseType::SQR_DIFF, {3, 31, 37, 17});
}

TEST_F(EltwiseOpTest, RandomTensorScalarHalf) {
  RandomTensorScalar<half>(kernels::EltwiseType::SUM, {1, 32, 32, 16});
  RandomTensorScalar<half>(kernels::EltwiseType::SUB, {3, 32, 32, 16});
  RandomTensorScalar<half>(kernels::EltwiseType::PROD, {1, 31, 37, 17});
  RandomTensorScalar<half>(kernels::EltwiseType::DIV, {3, 31, 37, 17});
  RandomTensorScalar<half>(kernels::EltwiseType::MIN, {1, 32, 32, 16});
  RandomTensorScalar<half>(kernels::EltwiseType::MAX, {3, 31, 37, 17});
  RandomTensorScalar<half>(kernels::EltwiseType::NEG, {1, 32, 32, 32});
  RandomTensorScalar<half>(kernels::EltwiseType::ABS, {3, 31, 37, 17});
  RandomTensorScalar<half>(kernels::EltwiseType::SQR_DIFF, {3, 31, 37, 17});
}

TEST_F(EltwiseOpTest, RandomTensorVecFloat) {
  RandomTensorEltwise<float>(kernels::EltwiseType::SUM, {1, 32, 32, 16},
                             {1, 1, 1, 16});
  RandomTensorEltwise<float>(kernels::EltwiseType::SUB, {5, 32, 32, 16},
                             {5, 1, 1, 16});
  RandomTensorEltwise<float>(kernels::EltwiseType::SUB, {5, 32, 32, 16},
                             {1, 1, 1, 16});
  RandomTensorEltwise<float>(kernels::EltwiseType::SUB, {5, 1, 1, 16},
                             {5, 32, 32, 16});
  RandomTensorEltwise<float>(kernels::EltwiseType::PROD, {1, 31, 37, 17},
                             {1, 1, 1, 17});
  RandomTensorEltwise<float>(kernels::EltwiseType::PROD, {1, 1, 1, 17},
                             {1, 31, 37, 17});
  RandomTensorEltwise<float>(kernels::EltwiseType::DIV, {3, 1, 1, 17},
                             {3, 31, 37, 17});
  RandomTensorEltwise<float>(kernels::EltwiseType::MIN, {1, 1, 1, 16},
                             {1, 32, 32, 16});
  RandomTensorEltwise<float>(kernels::EltwiseType::MAX, {5, 31, 37, 17},
                             {5, 1, 1, 17});
  RandomTensorEltwise<float>(kernels::EltwiseType::SQR_DIFF, {5, 31, 37, 17},
                             {5, 1, 1, 17});
}

TEST_F(EltwiseOpTest, RandomTensorVecHalf) {
  RandomTensorEltwise<half>(kernels::EltwiseType::SUM, {1, 32, 32, 16},
                            {1, 1, 1, 16});
  RandomTensorEltwise<half>(kernels::EltwiseType::SUB, {3, 32, 32, 16},
                            {3, 1, 1, 16});
  RandomTensorEltwise<half>(kernels::EltwiseType::SUB, {3, 32, 32, 16},
                            {1, 1, 1, 16});
  RandomTensorEltwise<half>(kernels::EltwiseType::SUB, {3, 1, 1, 16},
                            {3, 32, 32, 16});
  RandomTensorEltwise<half>(kernels::EltwiseType::PROD, {1, 1, 1, 17},
                            {1, 31, 37, 17});
  RandomTensorEltwise<half>(kernels::EltwiseType::DIV, {5, 31, 37, 17},
                            {5, 1, 1, 17});
  RandomTensorEltwise<half>(kernels::EltwiseType::DIV, {5, 31, 37, 17},
                            {1, 1, 1, 17});
  RandomTensorEltwise<half>(kernels::EltwiseType::DIV, {5, 1, 1, 17},
                            {5, 31, 37, 17});
  RandomTensorEltwise<half>(kernels::EltwiseType::MIN, {1, 1, 1, 16},
                            {1, 32, 32, 16});
  RandomTensorEltwise<half>(kernels::EltwiseType::MAX, {3, 31, 37, 17},
                            {3, 1, 1, 17});
  RandomTensorEltwise<half>(kernels::EltwiseType::SQR_DIFF, {3, 31, 37, 17},
                            {3, 1, 1, 17});
}

TEST_F(EltwiseOpTest, RandomTensorTensorFloat) {
  RandomTensorEltwise<float>(kernels::EltwiseType::SUM, {1, 32, 32, 16},
                             {1, 32, 32, 16});
  RandomTensorEltwise<float>(kernels::EltwiseType::SUB, {3, 32, 32, 16},
                             {3, 32, 32, 16});
  RandomTensorEltwise<float>(kernels::EltwiseType::PROD, {1, 31, 37, 17},
                             {1, 31, 37, 17});
  RandomTensorEltwise<float>(kernels::EltwiseType::DIV, {5, 31, 37, 17},
                             {5, 31, 37, 17});
  RandomTensorEltwise<float>(kernels::EltwiseType::MIN, {1, 32, 32, 16},
                             {1, 32, 32, 16});
  RandomTensorEltwise<float>(kernels::EltwiseType::MAX, {3, 31, 37, 17},
                             {3, 31, 37, 17});
  RandomTensorEltwise<float>(kernels::EltwiseType::SQR_DIFF, {3, 31, 37, 17},
                             {3, 31, 37, 17});
}

TEST_F(EltwiseOpTest, RandomTensorTensorHalf) {
  RandomTensorEltwise<half>(kernels::EltwiseType::SUM, {1, 32, 32, 16},
                            {1, 32, 32, 16});
  RandomTensorEltwise<half>(kernels::EltwiseType::SUB, {3, 32, 32, 16},
                            {3, 32, 32, 16});
  RandomTensorEltwise<half>(kernels::EltwiseType::PROD, {1, 31, 37, 17},
                            {1, 31, 37, 17});
  RandomTensorEltwise<half>(kernels::EltwiseType::DIV, {5, 31, 37, 17},
                            {5, 31, 37, 17});
  RandomTensorEltwise<half>(kernels::EltwiseType::MIN, {1, 32, 32, 16},
                            {1, 32, 32, 16});
  RandomTensorEltwise<half>(kernels::EltwiseType::MAX, {3, 31, 37, 17},
                            {3, 31, 37, 17});
  RandomTensorEltwise<half>(kernels::EltwiseType::SQR_DIFF, {3, 31, 37, 17},
                            {3, 31, 37, 17});
}

}  // namespace test
}  // namespace ops
}  // namespace mace
