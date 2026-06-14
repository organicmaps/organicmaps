#import "NSString+SHA256.h"

#import <CommonCrypto/CommonDigest.h>

@implementation NSString (SHA256)

- (NSString *)sha256String
{
  NSData * data = [self dataUsingEncoding:NSUTF8StringEncoding];
  if (data.length == 0)
    return @"";

  unsigned char buf[CC_SHA256_DIGEST_LENGTH];
  CC_SHA256(data.bytes, (CC_LONG)data.length, buf);
  NSMutableString * result = [NSMutableString stringWithCapacity:CC_SHA256_DIGEST_LENGTH * 2];
  for (NSInteger i = 0; i < CC_SHA256_DIGEST_LENGTH; ++i)
    [result appendFormat:@"%02x", buf[i]];

  return [result copy];
}

@end
