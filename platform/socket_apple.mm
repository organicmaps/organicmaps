#include "socket.hpp"

#import <Foundation/Foundation.h>

@interface SocketImpl : NSObject

- (BOOL)open:(NSString *)host port:(NSUInteger)port;
- (void)close;

- (BOOL)read:(uint8_t *)data count:(NSUInteger)count;
- (BOOL)write:(uint8_t const *)data count:(NSUInteger)count;

@end

@implementation SocketImpl

- (BOOL)open:(NSString *)host port:(NSUInteger)port { return YES; }
- (void)close {}
- (BOOL)read:(uint8_t *)data count:(NSUInteger)count { return YES; }
- (BOOL)write:(uint8_t const *)data count:(NSUInteger)count { return YES; }
@end

namespace platform
{
Socket::Socket() { m_socketImpl = [[SocketImpl alloc] init]; }
Socket::~Socket()
{
  Close();
  m_socketImpl = nil;
}

bool Socket::Open(string const & host, uint16_t port)
{
  return [m_socketImpl open:@(host.c_str()) port:port];
}

void Socket::Close() { [m_socketImpl close]; }
bool Socket::Read(uint8_t * data, uint32_t count) { return [m_socketImpl read:data count:count]; }
bool Socket::Write(uint8_t const * data, uint32_t count)
{
  return [m_socketImpl write:data count:count];
}
}  // namespace platform
