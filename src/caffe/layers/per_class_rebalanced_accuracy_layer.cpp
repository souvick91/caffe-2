// Copyright 2014 BVLC and contributors.
// #include <iostream>
#include <algorithm>
#include <functional>
#include <utility>
#include <vector>
// #include <iostream>
// #include <cmath>
// #include <cfloat>

#include "caffe/layer.hpp"
#include "caffe/vision_layers.hpp"
#include "caffe/util/math_functions.hpp"
#include "caffe/util/io.hpp"

using std::max;

namespace caffe {
  
template <typename Dtype>
void PerClassRebalancedAccuracyLayer<Dtype>::LayerSetUp(
  const vector<Blob<Dtype>*>& bottom, vector<Blob<Dtype>*>* top) {
  top_k_ = this->layer_param_.accuracy_param().top_k();
}

template <typename Dtype>
void AccuracyLayer<Dtype>::Reshape(
  const vector<Blob<Dtype>*>& bottom, vector<Blob<Dtype>*>* top) {
  CHECK_EQ(bottom[0]->num(), bottom[1]->num())
      << "The data and label should have the same number.";
  CHECK_LE(top_k_, bottom[0]->count() / bottom[0]->num())
      << "top_k must be less than or equal to the number of classes.";
  CHECK_EQ(bottom[1]->channels(), 1);
  CHECK_EQ(bottom[1]->height(), 1);
  CHECK_EQ(bottom[1]->width(), 1);
  (*top)[0]->Reshape(1, 1, 1, 1);
}


template <typename Dtype>
Dtype PerClassRebalancedAccuracyLayer<Dtype>::Forward_cpu(const vector<Blob<Dtype>*>& bottom,
    vector<Blob<Dtype>*>* top) {
  Dtype accuracy = 0;
  const Dtype* bottom_data = bottom[0]->cpu_data();
  const Dtype* bottom_label = bottom[1]->cpu_data(); //threshold_layer calls this bottom_data
  int num = bottom[0]->num();
  int dim = bottom[0]->count() / bottom[0]->num();

  int label_count = bottom[1]->count();
  
  Dtype* labels_count = labels_.mutable_cpu_data();
  caffe_set(labels_.count(), Dtype(FLT_MIN), labels_count);
  for (int i = 0; i < label_count; ++i) {
    labels_count[static_cast<int>(bottom_label[i])] += 1.0;
  } 

  float prior[2] = {0,0};
  for (int i = 0; i < num; ++i) 
    prior[static_cast<int>(bottom_label[i])] += 1.0 / num;
    
  vector<Dtype> maxval(top_k_+1);
  vector<int> max_id(top_k_+1);
  for (int i = 0; i < num; ++i) {
    // Top-k accuracy
    std::vector<std::pair<Dtype, int> > bottom_data_vector;
    for (int j = 0; j < dim; ++j) {
      bottom_data_vector.push_back(
          std::make_pair(bottom_data[i * dim + j], j));
    }
    std::partial_sort(
        bottom_data_vector.begin(), bottom_data_vector.begin() + top_k_,
        bottom_data_vector.end(), std::greater<std::pair<Dtype, int> >());
    // check if true label is in top k predictions
    for (int k = 0; k < top_k_; k++) {
      if (bottom_data_vector[k].second == static_cast<int>(bottom_label[i])) {
        ++accuracy;
        break;
      }
    }
  }
  
  // for (int i = 0; i < num; ++i) {
  //   // Accuracy
  //   Dtype maxval = -FLT_MAX;
  //   int max_id = 0;
  //   for (int j = 0; j < dim; ++j) {
  //     if (bottom_data[i * dim + j] > maxval) {
  //       maxval = bottom_data[i * dim + j];
  //       max_id = j;
  //     }
  //   }
  //   if (max_id == static_cast<int>(bottom_label[i])){
  //     accuracy += 1.0/static_cast<float>(labels_count[max_id]);
  //   }
  //   Dtype prob = max(bottom_data[i * dim + static_cast<int>(bottom_label[i])],
  //                    Dtype(kLOG_THRESHOLD));
  //   logprob -= log(prob) / (dim*num*prior[static_cast<int>(bottom_label[i])]);
  // }
  // (*top)[0]->mutable_cpu_data()[0] = accuracy / static_cast<float>(dim); //test score 0
  // (*top)[0]->mutable_cpu_data()[1] = logprob ;  //test score 1

  // Accuracy layer should not be used as a loss function.
  return Dtype(0);
}

INSTANTIATE_CLASS(PerClassRebalancedAccuracyLayer);

}  // namespace caffe
