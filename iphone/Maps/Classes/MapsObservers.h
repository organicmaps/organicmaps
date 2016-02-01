
#include "Framework.h"
#include "storage/storage_defines.hpp"

using namespace storage;

@protocol ActiveMapsObserverProtocol <NSObject>

- (void)countryStatusChangedAtPosition:(int)position inGroup:(ActiveMapsLayout::TGroup const &)group;

@optional
- (void)countryGroupChangedFromPosition:(int)oldPosition inGroup:(ActiveMapsLayout::TGroup const &)oldGroup toPosition:(int)newPosition inGroup:(ActiveMapsLayout::TGroup const &)newGroup;
- (void)countryOptionsChangedAtPosition:(int)position inGroup:(ActiveMapsLayout::TGroup const &)group;
- (void)countryDownloadingProgressChanged:(LocalAndRemoteSizeT const &)progress atPosition:(int)position inGroup:(ActiveMapsLayout::TGroup const &)group;

@end

@protocol CountryTreeObserverProtocol <NSObject>

- (void)countryStatusChangedAtPositionInNode:(int)position;
- (void)countryDownloadingProgressChanged:(LocalAndRemoteSizeT const &)progress atPositionInNode:(int)position;

@end

@class MWMWatchNotification;

class ActiveMapsObserver : public ActiveMapsLayout::ActiveMapsListener
{
public:
  ActiveMapsObserver(id<ActiveMapsObserverProtocol> delegateObject);
  virtual ~ActiveMapsObserver();

  virtual void CountryGroupChanged(ActiveMapsLayout::TGroup const & oldGroup, int oldPosition,
                                   ActiveMapsLayout::TGroup const & newGroup, int newPosition);
  virtual void CountryStatusChanged(ActiveMapsLayout::TGroup const & group, int position,
                                    TStatus const & oldStatus, TStatus const & newStatus);
  virtual void CountryOptionsChanged(ActiveMapsLayout::TGroup const & group, int position,
                                     MapOptions const & oldOpt, MapOptions const & newOpt);
  virtual void DownloadingProgressUpdate(ActiveMapsLayout::TGroup const & group, int position, LocalAndRemoteSizeT const & progress);

private:
  id<ActiveMapsObserverProtocol> m_delegateObject;
};


class CountryTreeObserver : public CountryTree::CountryTreeListener
{
public:
  CountryTreeObserver(id<CountryTreeObserverProtocol> delegateObject);

  virtual void ItemStatusChanged(int childPosition);
  virtual void ItemProgressChanged(int childPosition, LocalAndRemoteSizeT const & sizes);

private:
  id<CountryTreeObserverProtocol> m_delegateObject;
};
