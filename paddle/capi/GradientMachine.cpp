#include "PaddleCAPI.h"
#include "PaddleCAPIPrivate.h"
#include "paddle/gserver/gradientmachines/NeuralNetwork.h"

#define cast(v) paddle::capi::cast<paddle::capi::CGradientMachine>(v)

enum GradientMatchineCreateMode {
  CREATE_MODE_NORMAL = 0,
  CREATE_MODE_TESTING = 4
};

namespace paddle {

class MyNeuralNetwork : public NeuralNetwork {
public:
  MyNeuralNetwork(const std::string& name, NeuralNetwork* network)
      : NeuralNetwork(name, network) {}
};

NeuralNetwork* newCustomNerualNetwork(const std::string& name,
                                      NeuralNetwork* network) {
  return new MyNeuralNetwork(name, network);
}
}

extern "C" {
int PDGradientMachineCreateForPredict(PD_GradiemtMachine* machine,
                                      void* modelConfigProtobuf,
                                      int size) {
  if (modelConfigProtobuf == nullptr) return kPD_NULLPTR;
  paddle::ModelConfig config;
  if (!config.ParseFromArray(modelConfigProtobuf, size) ||
      !config.IsInitialized()) {
    return kPD_PROTOBUF_ERROR;
  }

  auto ptr = new paddle::capi::CGradientMachine();
  ptr->machine.reset(paddle::GradientMachine::create(
      config, CREATE_MODE_TESTING, {paddle::PARAMETER_VALUE}));
  *machine = ptr;
  return kPD_NO_ERROR;
}

int PDGradientMachineDestroy(PD_GradiemtMachine machine) {
  delete cast(machine);
  return kPD_NO_ERROR;
}

int PDGradientMachineLoadParameterFromDisk(PD_GradiemtMachine machine,
                                           const char* path) {
  auto m = cast(machine);
  if (m == nullptr || path == nullptr || m->machine == nullptr)
    return kPD_NULLPTR;
  m->machine->loadParameters(path);
  return kPD_NO_ERROR;
}

int PDGradientMachineForward(PD_GradiemtMachine machine,
                             PD_Arguments inArgs,
                             PD_Arguments outArgs,
                             bool isTrain) {
  auto m = cast(machine);
  auto in = paddle::capi::cast<paddle::capi::CArguments>(inArgs);
  auto out = paddle::capi::cast<paddle::capi::CArguments>(outArgs);
  if (m == nullptr || in == nullptr || out == nullptr || m->machine == nullptr)
    return kPD_NULLPTR;
  m->machine->forward(
      in->args, &out->args, isTrain ? paddle::PASS_TRAIN : paddle::PASS_TEST);
  return kPD_NO_ERROR;
}

int PDGradientMachineCreateSharedParam(PD_GradiemtMachine origin,
                                       void* modelConfigProtobuf,
                                       int size,
                                       PD_GradiemtMachine* slave) {
  auto o = cast(origin);
  if (origin == nullptr || slave == nullptr || o->machine == nullptr) {
    return kPD_NULLPTR;
  }
  paddle::ModelConfig config;
  if (!config.ParseFromArray(modelConfigProtobuf, size) ||
      !config.IsInitialized()) {
    return kPD_PROTOBUF_ERROR;
  }

  std::unique_ptr<paddle::capi::CGradientMachine> ptr(
      new paddle::capi::CGradientMachine());
  auto nn = paddle::NeuralNetwork::create(config);
  nn->init(config,
           [&o](int paramId, paddle::Parameter* param) {
             auto p = o->machine->getParameters()[paramId];
             param->enableSharedType(paddle::PARAMETER_VALUE,
                                     p->getBuf(paddle::PARAMETER_VALUE));

           },
           {paddle::PARAMETER_VALUE},
           false);
  ptr->machine.reset(nn);
  *slave = ptr.release();
  return kPD_NO_ERROR;
}
}
