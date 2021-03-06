#include "image_data_layer.h"
#include "utility.h"
#include <fstream>
#include <sstream>
#include "network.h"
using namespace std;

vector<array_sample> load_images(const string& dirname,
    const string& label_file, int width, int height, int label_size){
    ifstream fin(label_file);
    CHECK(fin);
    vector<array_sample> samples;
    int k = 0;
    while (fin) {
        string line;
        while (fin) {
            getline(fin, line);
            if (!line.empty())
                break;
        }
        int pos = (int) line.find('\t');
        if (pos <= 0) {
            pos = (int) line.find(' ');
        }
        if (pos <= 0) continue;
        string file_name = line.substr(0, pos);
        string label_str = line.substr(pos);
        CHECK(!file_name.empty());
        stringstream ss(label_str);
        int label_num;
        ss >> label_num;
        vector<int> vlabel;
        for (int i = 0; i<label_num; ++i){
            int nlabel;
            ss >> nlabel;
            vlabel.push_back(nlabel);
        }
        string file_path = dirname + "/" + file_name;
        ++k;
        printf("%d loading %s .\r", k, file_name.c_str());
        array3d image = imread(file_path);
        if (width >0 && height > 0) {
            if (image.rows() != height || image.cols() != width) {
                image = resize(image, width, height);
            }
        }
        if (image.size() == 0) {
            cout << "load image " << file_name << "failed" << endl;
            continue;
        }
        array2d label((int) vlabel.size(), label_size);
        label.clear(0);
        for (int i = 0; i<(int) vlabel.size(); ++i){
            CHECK(vlabel[i]<label_size && vlabel[i] >= 0);
            label.at2(i, vlabel[i]) = 1.0f;
        }
        samples.push_back(array_sample(image, label));
        if (samples.size() > 2) {
            CHECK(cmp_array_dim(samples.front().data(), samples.back().data()));
        }
    }
    printf("\n");
    CHECK(samples.size() > 0);
    return samples;
}

image_data_layer::image_data_layer(
    const string& dirname,
    const string& label_file,
    shared_ptr<block> data,
    shared_ptr<block> label,
    int batch, int iter,
    int loop, int label_size,
    int width, int height)
    :array_layer(
    load_images(dirname, label_file, width, height, label_size),
    data, label, batch, iter, loop) {
}


layer_ptr create_image_layer(
    const picojson::value& config,
    const string& layer_name,
    network* net) {
    string label_file = config.get("label_file").get<string>();
    string data_dir = config.get("data_dir").get<string>();
    int batch = (int) config.get("batch").get<double>();
    int label_size = (int) config.get("label_size").get<double>();

    //default iter = 1
    int iter = 1;
    if (config.contains("iter")){
        iter = (int) config.get("iter").get<double>();
    }

    //default loop
    int loop = INT_MAX;
    if (config.contains("loop")){
        loop = (int) config.get("loop").get<double>();
    }

    //width
    CHECK(config.contains("width"));
    int width = (int) config.get("width").get<double>();

    //height
    CHECK(config.contains("height"));
    int height = (int) config.get("height").get<double>();

    auto data_block = net->block("data");
    auto label_block = net->block("label");

    return layer_ptr(
        new image_data_layer(data_dir, label_file,
        data_block, label_block, batch, iter, loop, label_size, width, height));
}

REGISTER_LAYER(image_data, create_image_layer);