
#import <Foundation/Foundation.h>
#import "MapsObservers.h"
#import "Common.h"

ActiveMapsObserver::ActiveMapsObserver(id<ActiveMapsObserverProtocol> delegateObject)
: m_delegateObject(delegateObject)
{
}

ActiveMapsObserver::~ActiveMapsObserver()
{
  m_delegateObject = nil;
}

void ActiveMapsObserver::CountryGroupChanged(ActiveMapsLayout::TGroup const & oldGroup, int oldPosition,
                                             ActiveMapsLayout::TGroup const & newGroup, int newPosition)
{
  if ([m_delegateObject respondsToSelector:@selector(countryGroupChangedFromPosition:inGroup:toPosition:inGroup:)])
    [m_delegateObject countryGroupChangedFromPosition:oldPosition inGroup:oldGroup toPosition:newPosition inGroup:newGroup];
}

void ActiveMapsObserver::CountryStatusChanged(ActiveMapsLayout::TGroup const & group, int position,
                                              TStatus const & oldStatus, TStatus const & newStatus)
{
  if ([m_delegateObject respondsToSelector:@selector(countryStatusChangedAtPosition:inGroup:)])
    [m_delegateObject countryStatusChangedAtPosition:position inGroup:group];
}

void ActiveMapsObserver::CountryOptionsChanged(ActiveMapsLayout::TGroup const & group, int position, MapOptions const & oldOpt, MapOptions const & newOpt)
{
  if ([m_delegateObject respondsToSelector:@selector(countryOptionsChangedAtPosition:inGroup:)])
    [m_delegateObject countryOptionsChangedAtPosition:position inGroup:group];
}

void ActiveMapsObserver::DownloadingProgressUpdate(ActiveMapsLayout::TGroup const & group, int position, LocalAndRemoteSizeT const & progress)
{
  if ([m_delegateObject respondsToSelector:@selector(countryDownloadingProgressChanged:atPosition:inGroup:)])
    [m_delegateObject countryDownloadingProgressChanged:progress atPosition:position inGroup:group];
}


CountryTreeObserver::CountryTreeObserver(id<CountryTreeObserverProtocol> delegateObject)
: m_delegateObject(delegateObject)
{
}

void CountryTreeObserver::ItemStatusChanged(int childPosition)
{
  if ([m_delegateObject respondsToSelector:@selector(countryStatusChangedAtPositionInNode:)])
    [m_delegateObject countryStatusChangedAtPositionInNode:childPosition];
}

void CountryTreeObserver::ItemProgressChanged(int childPosition, storage::LocalAndRemoteSizeT const & sizes)
{
  if ([m_delegateObject respondsToSelector:@selector(countryDownloadingProgressChanged:atPositionInNode:)])
    [m_delegateObject countryDownloadingProgressChanged:sizes atPositionInNode:childPosition];
}
