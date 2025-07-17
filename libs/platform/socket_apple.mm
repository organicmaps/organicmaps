#import <Foundation/Foundation.h>

#include "platform/socket.hpp"

#include "base/logging.hpp"

@interface SocketImpl : NSObject

@property(nonatomic) NSInputStream * inputStream;
@property(nonatomic) NSOutputStream * outputStream;

@property(nonatomic) uint32_t timeout;

- (BOOL)open:(NSString *)host port:(NSUInteger)port;
- (void)close;

- (BOOL)read:(uint8_t *)data count:(NSUInteger)count;
- (BOOL)write:(uint8_t const *)data count:(NSUInteger)count;

@end

@implementation SocketImpl

- (BOOL)open:(NSString *)host port:(NSUInteger)port
{
  [self close];

  NSDate * openStart = [NSDate date];

  CFReadStreamRef readStream;
  CFWriteStreamRef writeStream;

  CFStreamCreatePairWithSocketToHost(NULL, (__bridge CFStringRef)(host), (UInt32)port, &readStream,
                                     &writeStream);

  NSDictionary * settings = @{
#ifndef RELEASE
                              (id)kCFStreamSSLValidatesCertificateChain : @NO,
#endif
                              (id)kCFStreamSSLLevel : (id)kCFStreamSocketSecurityLevelNegotiatedSSL
                              };

  CFReadStreamSetProperty(readStream, kCFStreamPropertySSLSettings, (CFTypeRef)settings);
  CFWriteStreamSetProperty(writeStream, kCFStreamPropertySSLSettings, (CFTypeRef)settings);

  self.inputStream = (__bridge_transfer NSInputStream *)readStream;
  self.outputStream = (__bridge_transfer NSOutputStream *)writeStream;

  [self.inputStream open];
  [self.outputStream open];

  while ([[NSDate date] timeIntervalSinceDate:openStart] < self.timeout)
  {
    NSStreamStatus const inputStreamStatus = self.inputStream.streamStatus;
    NSStreamStatus const outputStreamStatus = self.outputStream.streamStatus;
    if (inputStreamStatus == NSStreamStatusError || outputStreamStatus == NSStreamStatusError)
      return NO;
    if (inputStreamStatus == NSStreamStatusOpen && outputStreamStatus == NSStreamStatusOpen)
      return YES;
  }

  return NO;
}

- (void)close
{
  if (self.inputStream)
  {
    [self.inputStream close];
    self.inputStream = nil;
  }

  if (self.outputStream)
  {
    [self.outputStream close];
    self.outputStream = nil;
  }
}

- (BOOL)read:(uint8_t *)data count:(NSUInteger)count
{
  if (!self.inputStream || self.inputStream.streamStatus != NSStreamStatusOpen)
    return NO;

  NSDate * readStart = [NSDate date];
  uint8_t * readPtr = data;
  while (count != 0 && [[NSDate date] timeIntervalSinceDate:readStart] < self.timeout)
  {
    NSInteger const readCount = [self.inputStream read:readPtr maxLength:count];

    if (readCount > 0)
    {
      LOG(LDEBUG, ("Stream has read ", readCount, " bytes."));
      count -= readCount;
      readPtr += readCount;
    }
    else if (readCount == 0)
    {
      LOG(LDEBUG, ("The end of the read buffer was reached."));
    }
    else
    {
      LOG(LERROR, ("An error has occurred on the read stream."));
#ifdef RELEASE
      LOG(LERROR, (self.inputStream.streamError));
#else
      NSLog(@"%@", self.inputStream.streamError);
#endif
      return NO;
    }
  }

  return count == 0;
}

- (BOOL)write:(uint8_t const *)data count:(NSUInteger)count
{
  if (!self.outputStream || self.outputStream.streamStatus != NSStreamStatusOpen)
    return NO;

  uint8_t const * writePtr = data;

  NSDate * writeStart = [NSDate date];
  while (count != 0 && [[NSDate date] timeIntervalSinceDate:writeStart] < self.timeout)
  {
    NSInteger const writeCount = [self.outputStream write:writePtr maxLength:count];

    if (writeCount > 0)
    {
      LOG(LDEBUG, ("Stream has written ", writeCount, " bytes."));
      count -= writeCount;
      writePtr += writeCount;
    }
    else if (writeCount == 0)
    {
      LOG(LDEBUG, ("The end of the write stream has been reached."));
    }
    else
    {
      LOG(LERROR, ("An error has occurred on the write stream."));
#ifdef RELEASE
      LOG(LERROR, (self.outputStream.streamError));
#else
      NSLog(@"%@", self.outputStream.streamError);
#endif
      return NO;
    }
  }

  return count == 0;
}

@end

namespace platform
{
class PlatformSocket final : public Socket
{
public:
  PlatformSocket();
  // Socket overrides
  ~PlatformSocket();
  bool Open(std::string const & host, uint16_t port) override;
  void Close() override;
  bool Read(uint8_t * data, uint32_t count) override;
  bool Write(uint8_t const * data, uint32_t count) override;
  void SetTimeout(uint32_t milliseconds) override;

private:
  SocketImpl * m_socketImpl = nullptr;
};

std::unique_ptr<Socket> CreateSocket()
{
  return std::make_unique<PlatformSocket>();
}

PlatformSocket::PlatformSocket() { m_socketImpl = [[SocketImpl alloc] init]; }

PlatformSocket::~PlatformSocket()
{
  Close();
  m_socketImpl = nullptr;
}

bool PlatformSocket::Open(std::string const & host, uint16_t port)
{
  return [m_socketImpl open:@(host.c_str()) port:port];
}

void PlatformSocket::Close() { [m_socketImpl close]; }

bool PlatformSocket::Read(uint8_t * data, uint32_t count)
{
  return [m_socketImpl read:data count:count];
}

bool PlatformSocket::Write(uint8_t const * data, uint32_t count)
{
  return [m_socketImpl write:data count:count];
}

void PlatformSocket::SetTimeout(uint32_t milliseconds) { m_socketImpl.timeout = milliseconds; }
}  // namespace platform
