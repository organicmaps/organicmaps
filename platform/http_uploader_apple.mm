#import <Foundation/Foundation.h>
#ifndef IGNORE_SWIFT // Temporary solution for CMAKE builds. TODO: Remove when support of swift code compilation is added.
#endif

#include "platform/http_uploader.hpp"

#include "base/assert.hpp"
#include "base/waiter.hpp"

#include <memory>

@interface MultipartUploadTask : NSObject

- (void)uploadWithCompletion:(void(^)())completion;

@end

@implementation MultipartUploadTask

- (void)uploadWithCompletion:(void (^)())completion {
    dispatch_async(dispatch_get_main_queue(), completion);
}

@end

namespace platform
{
HttpUploader::Result HttpUploader::Upload() const
{
#ifndef IGNORE_SWIFT // Temporary solution for CMAKE builds. TODO: Remove when support of swift code compilation is added.
  std::shared_ptr<Result> resultPtr = std::make_shared<Result>();
  std::shared_ptr<base::Waiter> waiterPtr = std::make_shared<base::Waiter>();

  auto mapTransform =
      ^NSDictionary<NSString *, NSString *> *(std::map<std::string, std::string> keyValues)
  {
    NSMutableDictionary<NSString *, NSString *> * params = [@{} mutableCopy];
    for (auto const & keyValue : keyValues)
      params[@(keyValue.first.c_str())] = @(keyValue.second.c_str());
    return [params copy];
  };

    MultipartUploadTask *uploadTask = [[MultipartUploadTask alloc] init];
    [uploadTask uploadWithCompletion:[resultPtr, waiterPtr]() {
//        resultPtr->m_httpCode = static_cast<int32_t>(httpCode);
//        resultPtr->m_description = description.UTF8String;
        waiterPtr->Notify();

    }];
//  [MultipartUploader uploadWithMethod:@(m_method.c_str())
//                                  url:@(m_url.c_str())
//                              fileKey:@(m_fileKey.c_str())
//                             filePath:@(m_filePath.c_str())
//                               params:mapTransform(m_params)
//                              headers:mapTransform(m_headers)
//                             callback:[resultPtr, waiterPtr](NSInteger httpCode, NSString * _Nonnull description) {
//                               resultPtr->m_httpCode = static_cast<int32_t>(httpCode);
//                               resultPtr->m_description = description.UTF8String;
//                               waiterPtr->Notify();
//                             }];
  waiterPtr->Wait();
  return *resultPtr;
#endif
  return {};
}
} // namespace platform
