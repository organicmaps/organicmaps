#import "storage/downloader_subscriber_adapter_ios.h"

using namespace storage;

@interface SubscriberAdapter()
{
  std::vector<storage::MapFilesDownloader::Subscriber *> * m_subscribers;
}

@end

@implementation SubscriberAdapter

- (instancetype)initWithSubscribers:(std::vector<storage::MapFilesDownloader::Subscriber *> &)subscribers
{
  self = [super init];

  if (self)
    m_subscribers = &subscribers;

  return self;
}

- (void)didStartDownloading
{
  for (auto const & subscriber : *m_subscribers)
    subscriber->OnStartDownloading();
}

- (void)didFinishDownloading
{
  for (auto const & subscriber : *m_subscribers)
    subscriber->OnFinishDownloading();
}

@end
