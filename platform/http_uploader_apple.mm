#import <Foundation/Foundation.h>
#import "platform-Swift.h"

#include "platform/http_uploader.hpp"

#include "base/assert.hpp"

namespace platform
{
void HttpUploader::Upload() const
{
  CHECK(m_callback, ());

  auto mapTransform =
      ^NSDictionary<NSString *, NSString *> *(std::map<std::string, std::string> keyValues)
  {
    NSMutableDictionary<NSString *, NSString *> * params = [@{} mutableCopy];
    for (auto const & keyValue : keyValues)
      params[@(keyValue.first.c_str())] = @(keyValue.second.c_str());
    return [params copy];
  };

  [MultipartUploader uploadWithMethod:@(m_method.c_str())
                                  url:@(m_url.c_str())
                              fileKey:@(m_fileKey.c_str())
                             filePath:@(m_filePath.c_str())
                               params:mapTransform(m_params)
                              headers:mapTransform(m_headers)
                             callback:^(NSInteger httpCode, NSString * _Nonnull description) {
                               if (m_callback)
                                 m_callback(static_cast<int>(httpCode), description.UTF8String);
                             }];
}
} // namespace platform
