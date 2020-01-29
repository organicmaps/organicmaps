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

#include <cassert>
#include <cmath>
#include <cstring>
#include <iostream>
#include <memory>
#include <stddef.h>
#include <stdint.h>
#include <unordered_map>
#include <vector>

#import <Accelerate/Accelerate.h>

// minimal aten implementation
#define MAT_ALWAYS_INLINE inline __attribute__((always_inline))
namespace mat {

    template <typename T, size_t N>
    class MTensorAccessor {
    public:
        MAT_ALWAYS_INLINE
        MTensorAccessor(T* data, const int64_t* sizes, const int64_t* strides)
        : data_(data), sizes_(sizes), strides_(strides) {}

        MAT_ALWAYS_INLINE MTensorAccessor<T, N - 1> operator[](int64_t i) {
            return MTensorAccessor<T, N - 1>(
                                             this->data_ + this->strides_[0] * i,
                                             this->sizes_ + 1,
                                             this->strides_ + 1);
        }
        T* data_;
    private:
        const int64_t* sizes_;
        const int64_t* strides_;
    };

    template <typename T>
    class MTensorAccessor<T, 1> {
    public:
        MAT_ALWAYS_INLINE
        MTensorAccessor(T* data, const int64_t* sizes, const int64_t* strides)
        : data_(data), sizes_(sizes), strides_(strides) {}

        MAT_ALWAYS_INLINE T& operator[](int64_t i) {
            // assume stride==1 in innermost dimension.
            // DCHECK_EQ(strides_[0], 1);
            return this->data_[i];
        }
        T* data_;
        
    private:
        const int64_t* sizes_;
        const int64_t* strides_;
    };

    static void* MAllocateMemory(size_t nbytes) {
        void* ptr = nullptr;
        assert(nbytes > 0);
#ifdef __ANDROID__
        ptr = memalign(64, nbytes);
#else
        const int ret = posix_memalign(&ptr, 64, nbytes);
        (void)ret;
        assert(ret == 0);
#endif
        return ptr;
    }

    static void MFreeMemory(void* ptr) {
        free(ptr);
    }

    static void MCheckPtr(void* ptr) {
      if (ptr) {
          MFreeMemory(ptr);
      }
    }

    class MTensor {
    public:
        MTensor(){};
        MTensor(const std::vector<int64_t>& sizes) {
            auto strides = std::vector<int64_t>(sizes.size());
            strides[strides.size() - 1] = 1;
            for (auto i = static_cast<int32_t>(strides.size()) - 2; i >= 0; --i) {
                strides[i] = strides[i + 1] * sizes[i + 1];
            }
            strides_ = strides;
            sizes_ = sizes;
            // assume float32 storage.
            size_t nbytes = sizeof(float);
            for (auto size : sizes) {
                nbytes *= size;
            }
            storage_ = std::shared_ptr<void>(MAllocateMemory(nbytes), MCheckPtr);
        }

        int64_t size(int dim) {
            return sizes_[dim];
        }

        const std::vector<int64_t>& sizes() const {
            return sizes_;
        }

        const std::vector<int64_t>& strides() const {
            return strides_;
        }

        template <typename T>
        T* data() {
            return static_cast<T*>(storage_.get());
        }

        template <typename T, size_t N>
        MTensorAccessor<T, N> accessor() {
            return MTensorAccessor<T, N>(data<T>(), sizes().data(), strides().data());
        }

    private:
        std::vector<int64_t> sizes_;
        std::vector<int64_t> strides_;
        std::shared_ptr<void> storage_;
    };

    static MTensor mempty(const std::vector<int64_t>& sizes) {
        return MTensor(sizes);
    }
} // namespace mat

#endif
