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
PlatformSocket::PlatformSocket() {}

PlatformSocket::~PlatformSocket() { Close(); }

bool PlatformSocket::Open(string const & host, uint16_t port) { return false; }

void PlatformSocket::Close() {}

bool PlatformSocket::Read(uint8_t * data, uint32_t count) { return false; }

bool PlatformSocket::Write(uint8_t const * data, uint32_t count) { return false; }

void PlatformSocket::SetTimeout(uint32_t milliseconds) {}
}  // namespace platform
