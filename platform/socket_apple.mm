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

  CFStreamCreatePairWithSocketToHost(NULL, (__bridge CFStringRef)(host), port, &readStream,
                                     &writeStream);

  self.inputStream = (__bridge_transfer NSInputStream *)readStream;
  self.outputStream = (__bridge_transfer NSOutputStream *)writeStream;

  [self.inputStream setProperty:NSStreamSocketSecurityLevelNegotiatedSSL
                         forKey:NSStreamSocketSecurityLevelKey];
  [self.outputStream setProperty:NSStreamSocketSecurityLevelNegotiatedSSL
                          forKey:NSStreamSocketSecurityLevelKey];

  [self.inputStream open];
  [self.outputStream open];

  while ([[NSDate date] timeIntervalSinceDate:openStart] < self.timeout)
  {
    NSStreamStatus const inputStreamStatus = self.inputStream.streamStatus;
    NSStreamStatus const outputStreamStatus = self.outputStream.streamStatus;
    if (inputStreamStatus == NSStreamStatusError || outputStreamStatus == NSStreamStatusError)
      return NO;
    else if (inputStreamStatus == NSStreamStatusOpen && outputStreamStatus == NSStreamStatusOpen)
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

  NSInteger const readCount = [self.inputStream read:data maxLength:count];

  if (readCount > 0)
  {
    LOG(LDEBUG, ("Stream has read ", readCount, " bytes."));
  }
  else if (readCount == 0)
  {
    LOG(LDEBUG, ("The end of the read buffer was reached."));
  }
  else
  {
    LOG(LERROR, ("An error has occurred on the read stream."));
#ifdef OMIN_PRODUCTION
    LOG(LERROR, (self.inputStream.streamError));
#else
    NSLog(@"%@", self.inputStream.streamError);
#endif
  }

  return readCount == count;
}

- (BOOL)write:(uint8_t const *)data count:(NSUInteger)count
{
  if (!self.outputStream || self.outputStream.streamStatus != NSStreamStatusOpen)
    return NO;

  NSInteger const writeCount = [self.outputStream write:data maxLength:count];

  if (writeCount > 0)
  {
    LOG(LDEBUG, ("Stream has written ", writeCount, " bytes."));
  }
  else if (writeCount == 0)
  {
    LOG(LDEBUG, ("The end of the write stream has been reached."));
  }
  else
  {
    LOG(LERROR, ("An error has occurred on the write stream."));
#ifdef OMIN_PRODUCTION
    LOG(LERROR, (self.outputStream.streamError));
#else
    NSLog(@"%@", self.outputStream.streamError);
#endif
  }

  return writeCount == count;
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
  bool Open(string const & host, uint16_t port) override;
  void Close() override;
  bool Read(uint8_t * data, uint32_t count) override;
  bool Write(uint8_t const * data, uint32_t count) override;
  void SetTimeout(uint32_t milliseconds) override;

private:
  SocketImpl * m_socketImpl = nullptr;
};

unique_ptr<Socket> createSocket()
{
  return make_unique<PlatformSocket>();
}

PlatformSocket::PlatformSocket() { m_socketImpl = [[SocketImpl alloc] init]; }

PlatformSocket::~PlatformSocket() { Close(); }

bool PlatformSocket::Open(string const & host, uint16_t port)
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
