#import "Logger.h"
#import "base/logging.hpp"

@interface Logger ()
+ (base::LogLevel)baseLevel:(LogLevel)level;
@end

@implementation Logger

+ (void)log:(LogLevel)level message:(NSString*)message {
  LOG_SHORT([Logger baseLevel:level], (message.UTF8String));
}

+ (BOOL)canLog:(LogLevel)level {
  return [Logger baseLevel:level] >= base::g_LogLevel;
}

+ (base::LogLevel)baseLevel:(LogLevel)level {
  switch (level) {
    case LogLevelDebug:
      return LDEBUG;
    case LogLevelInfo:
      return LINFO;
    case LogLevelWarning:
      return LWARNING;
    case LogLevelError:
      return LERROR;
    case LogLevelCritical:
      return LCRITICAL;
  }
}

@end
