#include "pooling_layer.h"
#include "network.h"
using namespace std;

max_pooling_layer::max_pooling_layer(
    block_ptr input_block, block_ptr output_block, int size,int stride){
    this->m_input_block = input_block;
    this->m_output_block = output_block;
    this->m_size = size;
    this->m_stride = stride;
    CHECK(m_size >= 2);
}

bool max_pooling_layer::begin_seq() {
    this->m_max_history.clear();
    this->m_output_block->signal().clear(0);
    this->m_output_block->error().clear(0);
    return true;
}

void max_pooling_layer::setup_block() {
    CHECK(this->m_input_block->dims().size() == 3);
    auto &ds = this->m_input_block->dims();
    const int rows = (ds[0] - m_size) / m_stride + 1;
    const int cols = (ds[1] - m_size) / m_stride + 1;
    const int channels = ds[2];

    if (m_output_block->empty()) {
        m_output_block->resize(rows, cols, channels);
    }

    auto &ods = m_output_block->dims();
    CHECK(ods[0] == rows);
    CHECK(ods[1] == cols);
    CHECK(ods[2] == channels);
}

bool max_pooling_layer::forward() {
    array3d output = m_output_block->new_signal();
    array3d max_index = output.clone(false);
    array3d input = m_input_block->signal();
    int orows = output.rows(), ocols = output.cols(), channels = output.channels();
    int irows = input.rows(), icols = input.cols();

    const int index = m_size * m_size;

    OMP_FOR
    for (int r = 0; r < orows; ++r) {
        for (int c = 0; c < ocols; ++c) {
            for (int ch = 0; ch < channels; ++ch){
                //left top corner
                const int br = r * m_stride, bc = c * m_stride;

                //first
                float mmax = input.at3(br, bc, ch);
                int k = 0;
                for (int i = 1; i < index; ++i) {
                    const int nr = br + i % m_size, nc = bc + i / m_size;
                    if (nr < irows && nc < icols) {
                        const float val = input.at3(nr, nc, ch);
                        if (val > mmax) {
                            k = i;
                            mmax = val;
                        }
                    }
                }

                //output
                output.at3(r, c, ch) = mmax;

                //record max index
                max_index.at3(r, c, ch) = (float) k;
            }
        }
    }

    this->m_max_history.push_back(max_index);
    return true;
}

void max_pooling_layer::backward() {
    array3d oerror = m_output_block->error();
    array3d ierror = m_input_block->error();
    array3d &max_index = this->m_max_history.back();
    const int orows = oerror.rows(), 
        ocols = oerror.cols(), channels = oerror.channels();


    OMP_FOR
    for (int r = 0; r < orows; ++r) {
        for (int c = 0; c < ocols; ++c) {
            for (int ch = 0; ch < channels; ++ch) {
                const float err = oerror.at3(r, c, ch);
                const int idx = (int) max_index.at3(r, c, ch);
                const int nrow = r * m_stride + idx % m_size;
                const int ncol = c * m_stride + idx / m_size;
                ierror.at3(nrow, ncol, ch) += err;
            }
        }
    }

    oerror.clear(0);
    m_max_history.pop_back();
}


layer_ptr create_max_pooling_layer(
    const picojson::value& config,
    const string& layer_name,
    network* net) {
    CHECK(config.contains("input"));
    CHECK(config.contains("size"));
    auto input_block_id = config.get("input").get<string>();
    auto input_block = net->block(input_block_id);
    auto output_block = net->block(layer_name);
    int size = (int) config.get("size").get<double>();
    int stride = size;
    if (config.contains("stride")){
        stride = (int) config.get("stride").get<double>();
    }
    return layer_ptr(new max_pooling_layer(input_block, output_block, size, stride));
}

REGISTER_LAYER(max_pooling, create_max_pooling_layer)
