#ifndef INNER_PRODUCT_LAYER_H
#define INNER_PRODUCT_LAYER_H

#include "layer.h"
#include "array_operator.h"

class inner_product_layer : public layer {
public:
    inner_product_layer(
        const std::vector<std::shared_ptr<block> > &input_blocks,
        const std::shared_ptr<block> &output_block,
        int output_num);

    inner_product_layer(
        const std::shared_ptr<block> &input_block,
        const std::shared_ptr<block> &output_block,
        int output_num);

    virtual void setup_block();
    virtual void setup_params();

    virtual bool begin_seq();
    virtual bool forward();
    virtual void backward();
    virtual void end_batch(int size);

    virtual void save(std::ostream& os);
    virtual void load(std::istream& is);

private:
    void initialize(
        const std::vector<std::shared_ptr<block> > &input_blocks,
        const std::shared_ptr<block> &output_block,
        int output_num);
private:
    int m_output_num;

    std::vector<std::shared_ptr<block> > m_input_blocks;
    std::shared_ptr<block> m_output_block;

    std::vector<std::vector<arraykd> > m_inputs_history;

    std::vector<array2d> m_weights;
    arraykd m_bias;

    std::vector<array2d> m_grad_weights;
    arraykd m_grad_bias;
};

#endif