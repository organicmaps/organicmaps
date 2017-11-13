#import "MWMBookmarksManager.h"
#import "Statistics.h"
#import "SwiftBridge.h"

#include "Framework.h"

namespace
{
using Observer = id<MWMBookmarksObserver>;
using Observers = NSHashTable<Observer>;

using TLoopBlock = void (^)(Observer observer);
}  // namespace

@interface MWMBookmarksManager ()

@property(nonatomic) Observers * observers;
@property(nonatomic) BOOL areBookmarksLoaded;

@end

@implementation MWMBookmarksManager

+ (instancetype)manager
{
  static MWMBookmarksManager * manager;
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    manager = [[super alloc] initManager];
  });
  return manager;
}

+ (void)addObserver:(Observer)observer
{
  dispatch_async(dispatch_get_main_queue(), ^{
    [[MWMBookmarksManager manager].observers addObject:observer];
  });
}

+ (void)removeObserver:(Observer)observer
{
  dispatch_async(dispatch_get_main_queue(), ^{
    [[MWMBookmarksManager manager].observers removeObject:observer];
  });
}

- (instancetype)initManager
{
  self = [super init];
  if (self)
  {
    _observers = [Observers weakObjectsHashTable];
    [self registerBookmarksObserver];
  }
  return self;
}

- (void)registerBookmarksObserver
{
  BookmarkManager::AsyncLoadingCallbacks bookmarkCallbacks;
  {
    __weak auto wSelf = self;
    bookmarkCallbacks.m_onStarted = [wSelf]() {
      wSelf.areBookmarksLoaded = NO;
    };
  }
  {
    __weak auto wSelf = self;
    bookmarkCallbacks.m_onFinished = [wSelf]() {
      __strong auto self = wSelf;
      if (!self)
        return;
      self.areBookmarksLoaded = YES;
      [self loopObservers:^(Observer observer) {
        [observer onBookmarksLoadFinished];
      }];
    };
  }
  {
    __weak auto wSelf = self;
    bookmarkCallbacks.m_onFileSuccess = [wSelf](std::string const & filePath,
                                                bool isTemporaryFile) {
      __strong auto self = wSelf;
      if (!self)
        return;
      [self processFileEvent:YES];
      [self loopObservers:^(Observer observer) {
        if ([observer respondsToSelector:@selector(onBookmarksFileLoadSuccess)])
          [observer onBookmarksFileLoadSuccess];
      }];
      [Statistics logEvent:kStatEventName(kStatApplication, kStatImport)
            withParameters:@{kStatValue: kStatKML}];
    };
  }
  {
    __weak auto wSelf = self;
    bookmarkCallbacks.m_onFileError = [wSelf](std::string const & filePath, bool isTemporaryFile) {
      [wSelf processFileEvent:NO];
    };
  }
  GetFramework().GetBookmarkManager().SetAsyncLoadingCallbacks(std::move(bookmarkCallbacks));
}

+ (BOOL)areBookmarksLoaded { return [MWMBookmarksManager manager].areBookmarksLoaded; }

+ (void)loadBookmarks
{
  [MWMBookmarksManager manager];
  GetFramework().LoadBookmarks();
}

- (void)loopObservers:(TLoopBlock)block
{
  for (Observer observer in self.observers)
  {
    if (observer)
      block(observer);
  }
}

- (void)processFileEvent:(BOOL)success
{
  UIAlertController * alert = [UIAlertController
      alertControllerWithTitle:L(@"load_kmz_title")
                       message:success ? L(@"load_kmz_successful") : L(@"load_kmz_failed")
                preferredStyle:UIAlertControllerStyleAlert];
  UIAlertAction * action =
      [UIAlertAction actionWithTitle:L(@"ok") style:UIAlertActionStyleDefault handler:nil];
  [alert addAction:action];
  alert.preferredAction = action;
  [[UIViewController topViewController] presentViewController:alert animated:YES completion:nil];
}

@end
