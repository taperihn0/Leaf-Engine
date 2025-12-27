#pragma once

#include "PackedNetwork.hpp"

namespace nn {

class NeuralNetwork {
public:
    NeuralNetwork();
private:
    PackedNeuralNetwork _network;
};

} // namespace nn
