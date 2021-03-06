#include "test.h"
#include <iostream>
#include <omp.h>
#include "memory.h"
#include "utility.h"
#include "picojson.h"
#include "layer.h"
#include "array_layer.h"
#include "inner_product_layer.h"
#include "softmax_loss_layer.h"
#include "sigmoid_layer.h"
#include "image_data_layer.h"
#include "conv_layer.h"
#include "relu_layer.h"
#include "softmax_layer.h"
#include "network.h"
#include "gru_layer.h"
#include "solver.h"
#include "image_split_layer.h"
using namespace std;

//void test_xor() {
//    arraykd s1(2), s2(2), s3(2), s4(2);
//    array2d l1(1, 2), l2(1, 2), l3(1, 2), l4(1, 2);
//    s1.at(0) = 0, s1.at(1) = 1; l1.at(0) = 0; l1.at(1) = 1;
//    s2.at(0) = 1, s2.at(1) = 0; l2.at(0) = 0; l2.at(1) = 1;
//    s3.at(0) = 0, s3.at(1) = 0; l3.at(0) = 1; l3.at(1) = 0;
//    s4.at(0) = 1, s4.at(1) = 1; l4.at(0) = 1; l4.at(1) = 0;
//    vector<array_sample> samples;
//    samples.push_back(array_sample(s1, l1));
//    samples.push_back(array_sample(s2, l2));
//    samples.push_back(array_sample(s3, l3));
//    samples.push_back(array_sample(s4, l4));
//
//    shared_ptr<block> data_block(new block());
//    shared_ptr<block> label_block(new block());
//    shared_ptr<block> ip_hid1_block(new block());
//    shared_ptr<block> sig_hid1_block(new block());
//    shared_ptr<block> ip_hid2_block(new block());
//
//    shared_ptr<array_layer> array_layer(new array_layer(samples, data_block,
//        label_block, 4, 50000, 1));
//    shared_ptr<layer> ip1_layer(new inner_product_layer(data_block, ip_hid1_block, 2));
//    shared_ptr<layer> sig1_layer(new relu_layer(ip_hid1_block, sig_hid1_block));
//    shared_ptr<layer> ip2_layer(new inner_product_layer(sig_hid1_block, ip_hid2_block, 2));
//    shared_ptr<softmax_loss_layer> loss_layer(new softmax_loss_layer(ip_hid2_block, label_block));
//
//    loss_layer->set_report(false);
//
//    ip1_layer->set_learn_rate(0.1f);
//    ip2_layer->set_learn_rate(0.1f);
//
//    ip1_layer->set_momentum_decay(0.9f);
//    ip2_layer->set_momentum_decay(0.9f);
//
//    vector<shared_ptr<layer> > layer_seq;
//    layer_seq.push_back(array_layer);
//    layer_seq.push_back(ip1_layer);
//    layer_seq.push_back(sig1_layer);
//    layer_seq.push_back(ip2_layer);
//    layer_seq.push_back(loss_layer);
//
//    setup_block(layer_seq);
//    setup_params(layer_seq);
//    train(layer_seq, loss_layer, array_layer->batch());
//}
//


void train_and_test_network(const string& filename) {
    ::printf("load file %s.\n", filename.c_str());
    string json = read_file(filename);

    bool is_train = yes_no("train or predict");
    if (is_train){
        network train_net(json, "train");
        train_net.train();
    }
    else{
        network predict_net(json, "predict");

        while (true) {
            //read image
            auto file_name = promote_file_name("image file");
            auto image = imread(file_name);

            //recongnize
            int start_time = clock();
            predict_net.set_input(image);


            //read output
            string ans = "";
            const int max_length = 20;

            for (int i = 0;; ++i) {
                //forward
                arraykd output = predict_net.forward();
                //dim == 1
                if (output.dim() == 1) {
                    int k = output.arg_max();
                    string label = predict_net.translate(k);
                    if (label == "eof") {
                        break;
                    }
                    if (i > max_length) {
                        ::printf("seq is too long!");
                        break;
                    }
                    ans += label;
                    ::printf("%.3f ", output.at(k));
                }
                //dim == 2
                if (output.dim() == 2) {
                    array2d output2d = output;
                    for (int i = 0; i < output2d.rows(); ++i) {
                        int k = output2d.arg_max_row(i);
                        ans += predict_net.translate(k);
                        ::printf("%.3f ", output2d.at2(i, k));
                    }
                    break;
                }
                //dim > 2
                CHECK(output.dim() <= 2);
            }

            ::printf("\n");

            //print info
            float freq = 1.0f * CLOCKS_PER_SEC / (clock() - start_time);
            ::printf("result: %s, speed %.3f/s\n", ans.c_str(), freq);
        }
    }
}

void test_gru_layer(){
    auto input_block = block::new_block();
    auto output_block = block::new_block();
    auto label_block = block::new_block();
    layer_ptr layer1(new gru_layer(input_block, output_block, 2));
    layer1->set_learn_rate(0.01f);
    layer_ptr layer2(new softmax_loss_layer(output_block, label_block));
    ((softmax_loss_layer*) layer2.get())->set_report(true);

    input_block->resize(1);
    label_block->resize(2);

    vector<layer_ptr> layers = { layer1, layer2 };
    for (auto layer : layers){
        layer->setup_block();
    }
    for (auto layer : layers){
        layer->setup_params();
    }

    input_block->clear(0);
    label_block->clear(0);
    label_block->signal().at(0) = 1;

    while (true){
        float sr = input_block->error().sum();

        for (auto& layer : layers){
            layer->begin_seq();
        }

        for (int i = 0; i < 2; ++i){
            for (auto& layer : layers){
                layer->forward();
            }
        }

        for (int i = 0; i < 2; ++i){
            label_block->signal().clear();
            label_block->signal().at(i) = 1.0f;
            for (auto it = layers.rbegin(); it != layers.rend(); ++it){
                (*it)->backward();
            }
        }



        loss_layer* los_layer = (loss_layer*) layer2.get();
        float loss = los_layer->loss();
        printf("%.9f\n", loss);

        for (auto& layer : layers){
            layer->end_batch(1);
        }
    }

}