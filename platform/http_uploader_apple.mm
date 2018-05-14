#import <Foundation/Foundation.h>

#include "platform/http_uploader.hpp"

#include "base/assert.hpp"
#include "base/waiter.hpp"

#include <memory>

@interface MultipartUploadTask : NSObject

@property(copy, nonatomic) NSString * method;
@property(copy, nonatomic) NSString * urlString;
@property(copy, nonatomic) NSString * fileKey;
@property(copy, nonatomic) NSString * filePath;
@property(copy, nonatomic) NSDictionary<NSString *, NSString *> * params;
@property(copy, nonatomic) NSDictionary<NSString *, NSString *> * headers;

@end

@implementation MultipartUploadTask

- (NSData *)requestDataWithBoundary:(NSString *)boundary {
  NSMutableData * data = [NSMutableData data];

  [self.params enumerateKeysAndObjectsUsingBlock:^(NSString * key, NSString * value, BOOL * stop) {
    [data appendData:[[NSString stringWithFormat:@"--%@\r\n", boundary]
                         dataUsingEncoding:NSUTF8StringEncoding]];
    [data appendData:[[NSString stringWithFormat:@"Content-Disposition: form-data; name=\"%@\"\r\n\r\n",
                          key] dataUsingEncoding:NSUTF8StringEncoding]];
    [data appendData:[[NSString stringWithFormat:@"%@\r\n", value]
                         dataUsingEncoding:NSUTF8StringEncoding]];
  }];

  NSString * fileName = self.filePath.lastPathComponent;
  NSData * fileData = [NSData dataWithContentsOfFile:self.filePath];
  NSString * mimeType = @"application/octet-stream";

  [data appendData:[[NSString stringWithFormat:@"--%@\r\n", boundary]
                       dataUsingEncoding:NSUTF8StringEncoding]];
  [data appendData:[[NSString stringWithFormat:@"Content-Disposition: form-data; name=\"%@\"; filename=\"%@\"\r\n",
                        self.fileKey,
                        fileName]
                       dataUsingEncoding:NSUTF8StringEncoding]];
  [data appendData:[[NSString stringWithFormat:@"Content-Type: %@\r\n\r\n", mimeType]
                      dataUsingEncoding:NSUTF8StringEncoding]];
  [data appendData:fileData];
  [data appendData:[@"\r\n" dataUsingEncoding:NSUTF8StringEncoding]];
  [data appendData:[[NSString stringWithFormat:@"--%@--", boundary]
                                      dataUsingEncoding:NSUTF8StringEncoding]];

  return data;
}

- (void)uploadWithCompletion:(void (^)(NSInteger httpCode, NSString *description))completion {
  NSURL * url = [NSURL URLWithString:self.urlString];
  NSMutableURLRequest * uploadRequest = [NSMutableURLRequest requestWithURL:url];
  uploadRequest.timeoutInterval = 5;
  uploadRequest.HTTPMethod = self.method;

  NSString * boundary = [NSString stringWithFormat:@"Boundary-%@", [[NSUUID UUID] UUIDString]];
  NSString * contentType = [NSString stringWithFormat:@"multipart/form-data; boundary=%@", boundary];
  [uploadRequest setValue:contentType forHTTPHeaderField:@"Content-Type"];

  NSData * postData = [self requestDataWithBoundary:boundary];

  NSURLSessionUploadTask * uploadTask = [[NSURLSession sharedSession]
                                        uploadTaskWithRequest:uploadRequest
                                        fromData:postData
                                        completionHandler:^(NSData * data,
                                                            NSURLResponse * response,
                                                            NSError * error) {
            NSHTTPURLResponse * httpResponse = (NSHTTPURLResponse *)response;
            if (error == nil) {
              NSString * description = [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding];
              completion(httpResponse.statusCode, description);
            } else {
              completion(-1, error.localizedDescription);
            }
          }];
  [uploadTask resume];
}

@end

namespace platform
{
HttpUploader::Result HttpUploader::Upload() const
{
  std::shared_ptr<Result> resultPtr = std::make_shared<Result>();
  std::shared_ptr<base::Waiter> waiterPtr = std::make_shared<base::Waiter>();

  auto mapTransform =
      ^NSDictionary<NSString *, NSString *> * (std::map<std::string, std::string> keyValues)
  {
    NSMutableDictionary<NSString *, NSString *> * params = [NSMutableDictionary dictionary];
    for (auto const & keyValue : keyValues)
      params[@(keyValue.first.c_str())] = @(keyValue.second.c_str());
    return [params copy];
  };

  auto uploadTask = [[MultipartUploadTask alloc] init];
  uploadTask.method = @(m_method.c_str());
  uploadTask.urlString = @(m_url.c_str());
  uploadTask.fileKey = @(m_fileKey.c_str());
  uploadTask.filePath = @(m_filePath.c_str());
  uploadTask.params = mapTransform(m_params);
  uploadTask.headers = mapTransform(m_headers);
  [uploadTask uploadWithCompletion:[resultPtr, waiterPtr](NSInteger httpCode, NSString * description) {
    resultPtr->m_httpCode = static_cast<int32_t>(httpCode);
    resultPtr->m_description = description.UTF8String;
    waiterPtr->Notify();
  }];
  waiterPtr->Wait();
  return *resultPtr;
}
} // namespace platform
