// Copyright (c) 2014-present, Facebook, Inc. All rights reserved.
//
// You are hereby granted a non-exclusive, worldwide, royalty-free license to use,
// copy, modify, and distribute this software in source code or binary form for use
// in connection with the web services and APIs provided by Facebook.
//
// As with any software that integrates with the Facebook platform, your use of
// this software is subject to the Facebook Developer Principles and Policies
// [http://developers.facebook.com/policy/]. This copyright notice shall be
// included in all copies or substantial portions of the software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
// FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
// COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
// IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#import "TargetConditionals.h"

#if !TARGET_OS_TV

#include <float.h>
#include <math.h>
#include <stdint.h>
#include <unordered_map>
#include <unordered_set>

#import <Accelerate/Accelerate.h>

#include "FBSDKStandaloneModel.hpp"

#define SEQ_LEN 128
#define ALPHABET_SIZE 256
#define EMBEDDING_SIZE 64
#define DENSE_FEATURE_LEN 30

const int CONV_BLOCKS[3][3] = {{32, 2, SEQ_LEN - 1}, {32, 3, SEQ_LEN - 2}, {32, 5, SEQ_LEN - 4}};

namespace mat1 {
    static void relu(float *data, int len) {
        float min = 0;
        float max = FLT_MAX;
        vDSP_vclip(data, 1, &min, &max, data, 1, len);
    }

    static void concatenate(float *dst, float *a, float *b, int a_len, int b_len) {
        memcpy(dst, a, a_len * sizeof(float));
        memcpy(dst + a_len, b, b_len * sizeof(float));
    }

    static void softmax(float *data, int n) {
        int i = 0;
        float max = FLT_MIN;
        float sum = 0;

        for (i = 0; i < n; i++) {
            if (data[i] > max) {
                max = data[i];
            }
        }

        for (i = 0; i < n; i++){
            data[i] = expf(data[i] - max);
        }

        for (i = 0; i < n; i++){
            sum += data[i];
        }

        for (i = 0; i < n; i++){
            data[i] = data[i] / sum;
        }
    }

    static float* embedding(int *a, float *b, int n_examples, int seq_length, int embedding_size) {
        int i,j,k,val;
        float* res = (float *)malloc(sizeof(float) * (n_examples * seq_length * embedding_size));
        for (i = 0; i < n_examples; i++) {
            for (j = 0; j < seq_length; j++) {
                val = a[i * seq_length + j];
                for (k = 0; k < embedding_size; k++) {
                    res[(embedding_size * seq_length) * i + embedding_size * j + k] = b[val * embedding_size + k];
                }
            }
        }
        return res;
    }

    /*
        a shape: n_examples, in_vector_size
        b shape: n_examples, out_vector_size
        c shape: out_vector_size
        return shape: n_examples, out_vector_size
     */
    static float* dense(float *a, float *b, float *c, int n_examples, int in_vector_size, int out_vector_size) {
        int i,j;
        float *m_res = (float *)malloc(sizeof(float) * (n_examples * out_vector_size));
        vDSP_mmul(a, 1, b, 1, m_res, 1, n_examples, out_vector_size, in_vector_size);
        for (i = 0; i < n_examples; i++) {
            for (j = 0; j < out_vector_size; j++) {
                m_res[i * out_vector_size + j] += c[j];
            }
        }
        return m_res;
    }

    /*
        x shape: n_examples, seq_len, input_size
        w shape: kernel_size, input_size, output_size
        return shape: n_examples, seq_len - kernel_size + 1, output_size
     */
    static float* conv1D(float *x, float *w, int n_examples, int seq_len, int input_size, int kernel_size, int output_size) {
        int n, o, i, k, m;
        float sum;
        float *res = (float *)malloc(sizeof(float) * (n_examples * (seq_len - kernel_size + 1) * output_size));
        float *temp_x = (float *)malloc(sizeof(float) * (kernel_size * input_size));
        float *temp_w = (float *)malloc(sizeof(float) * (kernel_size * input_size));
        for (n = 0; n < n_examples; n++){
            for (o = 0; o < output_size; o++){
                for (i = 0; i < seq_len - kernel_size + 1; i++) {
                    sum = 0;
                    for (m = 0; m < kernel_size; m++) {
                        for (k = 0; k < input_size; k++) {
                          temp_x[m * input_size + k] = x[n * (seq_len * input_size) + (m + i) * input_size + k];
                          temp_w[m * input_size + k] = w[(m * input_size + k) * output_size + o];
                        }
                    }
                    vDSP_dotpr(temp_x, 1, temp_w, 1, &sum, kernel_size * input_size);
                    res[(n * (output_size * (seq_len - kernel_size + 1)) + i * output_size + o)] = sum;
                }
            }
        }
        free(temp_x);
        free(temp_w);
        return res;
    }

    /*
       input shape: n_examples, len, n_channel
       return shape: n_examples, len - pool_size + 1, n_channel
    */
    static float* maxPool1D(float *input, int n_examples, int input_len, int n_channel, int pool_size) {
        int res_len = input_len - pool_size + 1;
        float* res = (float *)calloc(n_examples * res_len * n_channel, sizeof(float));

        for (int n = 0; n < n_examples; n++) {
          for (int c = 0; c < n_channel; c++) {
            for (int i  = 0; i < res_len; i++) {
              for (int r = i; r < i + pool_size; r++) {
                int res_pos = n * (n_channel * res_len) + i * n_channel + c;
                int input_pos = n * (n_channel * input_len) + r * n_channel + c;
                if (r == i) {
                  res[res_pos] = input[input_pos];
                } else {
                  res[res_pos] = fmax(res[res_pos], input[input_pos]);
                }
              }
            }
          }
        }
        return res;
    }

    static int* vectorize(const char *texts, int str_len, int max_len) {
        int *res = (int *)malloc(sizeof(int) * max_len);
        for (int i = 0; i < max_len; i++) {
            if (i < str_len){
                res[i] = static_cast<unsigned char>(texts[i]);
            } else {
                res[i] = 0;
            }
        }
        return res;
    }

    /*
       input shape: m, n
       return shape: n, m
    */
    static float* transpose2D(float *input, int m, int n) {
        float *transposed = (float *)malloc(sizeof(float) * m * n);
        for (int i = 0; i < m; i++){
            for (int j = 0; j < n; j++) {
                transposed[j * m + i] = input[i * n + j];
            }
        }
        return transposed;
    }

    /*
       input shape: m, n, p
       return shape: p, n, m
    */
    static float* transpose3D(float *input, int64_t m, int n, int p) {
        float *transposed = (float *)malloc((size_t)(sizeof(float) * m * n * p));
        for (int i = 0; i < m; i++){
            for (int j = 0; j < n; j++) {
                for (int k = 0; k < p; k++) {
                    transposed[k * m * n + j * m + i] = input[i * n * p + j * p + k];
                }
            }
        }
        return transposed;
    }

    static float* add(float *a, float *b, int m, int n, int p) {
        for(int i = 0; i < m * n; i++){
            for(int j = 0; j < p; j++){
                a[i * p + j] += b[j];
            }
        }
        return a;
    }

    static float* predictOnText(const char *texts, std::unordered_map<std::string, mat::MTensor>& weights, float *df) {
        int *x;
        float *embed_x;
        float *dense1_x;
        float *dense2_x;
        float *dense3_x;
        float *c1;
        float *c2;
        float *c3;
        float *ca;
        float *cb;
        float *cc;

        mat::MTensor& embed_t = weights.at("embed.weight");
        mat::MTensor& conv1w_t = weights.at("convs.0.weight"); // (32, 64, 2)
        mat::MTensor& conv2w_t = weights.at("convs.1.weight");
        mat::MTensor& conv3w_t = weights.at("convs.2.weight");
        mat::MTensor& conv1b_t = weights.at("convs.0.bias");
        mat::MTensor& conv2b_t = weights.at("convs.1.bias");
        mat::MTensor& conv3b_t = weights.at("convs.2.bias");
        mat::MTensor& fc1w_t = weights.at("fc1.weight"); // (128, 126)
        mat::MTensor& fc1b_t = weights.at("fc1.bias");  // 128
        mat::MTensor& fc2w_t = weights.at("fc2.weight"); // (64, 128)
        mat::MTensor& fc2b_t = weights.at("fc2.bias"); // 64
        mat::MTensor& fc3w_t = weights.at("fc3.weight"); // (2, 64) or (4, 64)
        mat::MTensor& fc3b_t = weights.at("fc3.bias"); // 2 or 4

        float *embed_weight = embed_t.data<float>();
        float *convs_0_weight = transpose3D(conv1w_t.data<float>(), (int)conv1w_t.size(0), (int)conv1w_t.size(1), (int)conv1w_t.size(2)); // (2, 64, 32)
        float *convs_1_weight = transpose3D(conv2w_t.data<float>(), (int)conv2w_t.size(0), (int)conv2w_t.size(1), (int)conv2w_t.size(2));
        float *convs_2_weight = transpose3D(conv3w_t.data<float>(), (int)conv3w_t.size(0), (int)conv3w_t.size(1), (int)conv3w_t.size(2));
        float *convs_0_bias = conv1b_t.data<float>();
        float *convs_1_bias = conv2b_t.data<float>();
        float *convs_2_bias = conv3b_t.data<float>();
        float *fc1_weight = transpose2D(fc1w_t.data<float>(), (int)fc1w_t.size(0), (int)fc1w_t.size(1));
        float *fc2_weight = transpose2D(fc2w_t.data<float>(), (int)fc2w_t.size(0), (int)fc2w_t.size(1));
        float *fc3_weight = transpose2D(fc3w_t.data<float>(), (int)fc3w_t.size(0), (int)fc3w_t.size(1));
        float *fc1_bias = fc1b_t.data<float>();
        float *fc2_bias = fc2b_t.data<float>();
        float *fc3_bias = fc3b_t.data<float>();

        // vectorize text
        x = vectorize(texts, (int)strlen(texts), SEQ_LEN);

        // embedding
        embed_x = embedding(x, embed_weight, 1, SEQ_LEN, EMBEDDING_SIZE); // (1, 128, 64)
        free(x);

        // conv1D
        c1 = conv1D(embed_x, convs_0_weight, 1, SEQ_LEN, EMBEDDING_SIZE, (int)conv1w_t.size(2), (int)conv1w_t.size(0)); // (1, 127, 32) CONV_BLOCKS[0][1], CONV_BLOCKS[0][0]
        c2 = conv1D(embed_x, convs_1_weight, 1, SEQ_LEN, EMBEDDING_SIZE, (int)conv2w_t.size(2), (int)conv2w_t.size(0)); // (1, 126, 32)
        c3 = conv1D(embed_x, convs_2_weight, 1, SEQ_LEN, EMBEDDING_SIZE, (int)conv3w_t.size(2), (int)conv3w_t.size(0)); // (1, 124, 32)
        free(embed_x);

        // add bias
        add(c1, convs_0_bias, 1, (int)(SEQ_LEN - conv1w_t.size(2) + 1), (int)conv1w_t.size(0));
        add(c2, convs_1_bias, 1, (int)(SEQ_LEN - conv2w_t.size(2) + 1), (int)conv2w_t.size(0));
        add(c3, convs_2_bias, 1, (int)(SEQ_LEN - conv3w_t.size(2) + 1), (int)conv3w_t.size(0));

        // relu
        relu(c1, (int)(SEQ_LEN - conv1w_t.size(2) + 1) * (int)conv1w_t.size(0));
        relu(c2, (int)(SEQ_LEN - conv2w_t.size(2) + 1) * (int)conv2w_t.size(0));
        relu(c3, (int)(SEQ_LEN - conv3w_t.size(2) + 1) * (int)conv3w_t.size(0));

        // max pooling
        ca = maxPool1D(c1, 1, (int)(SEQ_LEN - conv1w_t.size(2) + 1), (int)conv1w_t.size(0), (int)(SEQ_LEN - conv1w_t.size(2) + 1)); // (1, 1, 32)
        cb = maxPool1D(c2, 1, (int)(SEQ_LEN - conv2w_t.size(2) + 1), (int)conv2w_t.size(0), (int)(SEQ_LEN - conv2w_t.size(2) + 1)); // (1, 1, 32)
        cc = maxPool1D(c3, 1, (int)(SEQ_LEN - conv3w_t.size(2) + 1), (int)conv3w_t.size(0), (int)(SEQ_LEN - conv3w_t.size(2) + 1)); // (1, 1, 32)
        free(c1);
        free(c2);
        free(c3);

        // concatenate
        float *concat = (float *)malloc((size_t)(sizeof(float) * (conv1w_t.size(0) + conv2w_t.size(0) + conv3w_t.size(0) + 30)));
        concatenate(concat, ca, cb, (int)conv1w_t.size(0), (int)conv2w_t.size(0));
        concatenate(concat + conv1w_t.size(0) + conv2w_t.size(0), cc, df, (int)conv3w_t.size(0), 30);
        free(ca);
        free(cb);
        free(cc);

        // dense + relu
        dense1_x = dense(concat, fc1_weight, fc1_bias, 1, (int)fc1w_t.size(1), (int)fc1w_t.size(0));
        free(concat);
        relu(dense1_x, (int)fc1b_t.size(0));
        dense2_x = dense(dense1_x, fc2_weight, fc2_bias, 1, (int)fc2w_t.size(1), (int)fc2w_t.size(0));
        relu(dense2_x, (int)fc2b_t.size(0));
        free(dense1_x);
        dense3_x = dense(dense2_x, fc3_weight, fc3_bias, 1, (int)fc3w_t.size(1), (int)fc3w_t.size(0));
        free(dense2_x);
        softmax(dense3_x, (int)fc3b_t.size(0));
        return dense3_x;
    }
}

#endif
