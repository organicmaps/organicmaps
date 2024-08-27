#define IPAD (UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPad)

#define L(str) NSLocalizedString(str, nil)

#define UnexpectedCondition(desc, ...)  \
  __PRAGMA_PUSH_NO_EXTRA_ARG_WARNINGS \
  [[NSAssertionHandler currentHandler] handleFailureInMethod:_cmd \
    object:self file:@(__FILE_NAME__) \
    lineNumber:__LINE__ description:(desc), ##__VA_ARGS__]; \
  __PRAGMA_POP_NO_EXTRA_ARG_WARNINGS
