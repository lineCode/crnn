#ifndef TANH_LAYER_H
#define TANH_LAYER_H

#include "layer.h"

class tanh_layer : public layer{
public:
    tanh_layer(block_ptr input, block_ptr output);
    virtual void setup_block();
    virtual bool begin_seq();
    virtual bool forward();
    virtual void backward();

private:
    block_ptr m_input_block;
    block_ptr m_output_block;
    std::vector<arraykd> m_mid_history;
};

#endif