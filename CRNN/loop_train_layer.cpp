#include "loop_train_layer.h"
using namespace std;

loop_train_layer::loop_train_layer(const std::vector<layer_ptr>& layers){
    this->m_layers = layers;
}

void loop_train_layer::setup_block(){
    for (auto &layer : m_layers){
        layer->setup_block();
    }
}

void loop_train_layer::setup_params(){
    for (auto &layer : m_layers){
        layer->setup_params();
    }
}

bool loop_train_layer::begin_seq(){
    for (auto& layer : m_layers){
        if (!layer->begin_seq()){
            return false;
        }
    }
    return true;
}

bool loop_train_layer::forward(){
    while (true){
        for (auto& layer : m_layers){
            if (!layer->forward()){
                goto end;
            }
        }
    }
end:
    return true;
}

void loop_train_layer::backward(){
    for_each(m_layers.rbegin(), m_layers.rend(), [](layer_ptr& layer){
        layer->backward();
    });
}

void loop_train_layer::end_batch(int size){
    for (auto& layer : m_layers){
        layer->end_batch(size);
    }
}