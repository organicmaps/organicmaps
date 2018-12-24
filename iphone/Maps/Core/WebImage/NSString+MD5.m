#import "NSString+MD5.h"

#import <CommonCrypto/CommonDigest.h>

@implementation NSString (MD5)

- (NSString *)md5String {
  NSData *data = [self dataUsingEncoding:NSUTF8StringEncoding];
  if (data.length == 0) {
    return @"";
  }

  unsigned char buf[CC_MD5_DIGEST_LENGTH];
  CC_MD5(data.bytes, (CC_LONG)data.length, buf);
  NSMutableString *result = [NSMutableString stringWithCapacity:CC_MD5_DIGEST_LENGTH * 2];
  for (NSInteger i = 0; i < CC_MD5_DIGEST_LENGTH; ++i) {
    [result appendFormat:@"%02x", buf[i]];
  }

  return [result copy];
}

@end
