#import <Foundation/Foundation.h>

#include "platform/http_uploader.hpp"

#include "base/assert.hpp"
#include "base/waiter.hpp"

#include <memory>

#include "private.h"

struct IdentityAndTrust
{
  SecIdentityRef identityRef;
  NSArray * certArray;
};

@interface MultipartUploadTask : NSObject<NSURLSessionDelegate>

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

  NSURLSession * session =
      [NSURLSession sessionWithConfiguration:[NSURLSessionConfiguration defaultSessionConfiguration]
                                    delegate:self
                               delegateQueue:nil];
  NSURLSessionUploadTask * uploadTask = [session
      uploadTaskWithRequest:uploadRequest
                   fromData:postData
          completionHandler:^(NSData * data, NSURLResponse * response, NSError * error) {
            NSHTTPURLResponse * httpResponse = (NSHTTPURLResponse *)response;
            if (error == nil)
            {
              NSString * description = [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding];
              completion(httpResponse.statusCode, description);
            }
            else
            {
              completion(-1, error.localizedDescription);
            }
          }];
  [uploadTask resume];
}

- (void)URLSession:(NSURLSession *)session
    didReceiveChallenge:(NSURLAuthenticationChallenge *)challenge
      completionHandler:(void (^)(NSURLSessionAuthChallengeDisposition disposition,
                                  NSURLCredential * _Nullable credential))completionHandler
{
  NSString * authenticationMethod = challenge.protectionSpace.authenticationMethod;
  if (authenticationMethod == NSURLAuthenticationMethodClientCertificate)
  {
    completionHandler(NSURLSessionAuthChallengeUseCredential, [self getClientUrlCredential]);
  }
  else if (authenticationMethod == NSURLAuthenticationMethodServerTrust)
  {
    NSURLCredential * credential =
        [[NSURLCredential alloc] initWithTrust:[challenge protectionSpace].serverTrust];
    completionHandler(NSURLSessionAuthChallengeUseCredential, credential);
  }
}

- (NSURLCredential *)getClientUrlCredential
{
  NSData * certData = [[NSData alloc] initWithBase64EncodedString:@USER_BINDING_PKCS12 options:0];
  IdentityAndTrust identity = [self extractIdentityWithCertData:certData
                                                   certPassword:@USER_BINDING_PKCS12_PASSWORD];

  NSURLCredential * urlCredential =
      [[NSURLCredential alloc] initWithIdentity:identity.identityRef
                                   certificates:identity.certArray
                                    persistence:NSURLCredentialPersistenceForSession];

  return urlCredential;
}

- (IdentityAndTrust)extractIdentityWithCertData:(NSData *)certData
                                   certPassword:(NSString *)certPassword
{
  IdentityAndTrust identityAndTrust;

  NSDictionary * certOptions = @{(id)kSecImportExportPassphrase: certPassword};

  CFArrayRef rawItems = NULL;
  OSStatus status = SecPKCS12Import((__bridge CFDataRef)certData,
                                    (__bridge CFDictionaryRef)certOptions, &rawItems);
  NSArray * items = (NSArray *)CFBridgingRelease(rawItems);

  if ((status == errSecSuccess) && ([items count] > 0))
  {
    NSDictionary * firstItem = items[0];

    SecIdentityRef identity =
        (SecIdentityRef)CFBridgingRetain(firstItem[(id)kSecImportItemIdentity]);

    CFArrayRef certChain = (CFArrayRef)CFBridgingRetain(firstItem[(id)kSecImportItemCertChain]);
    NSArray * certArray = CFBridgingRelease(certChain);

    identityAndTrust = {.identityRef = identity, .certArray = certArray};
  }

  return identityAndTrust;
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
