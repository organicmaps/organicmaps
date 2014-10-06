#include "country_tree.hpp"

#include "framework.hpp"

#include "../../storage/storage.hpp"

namespace storage
{

namespace
{
  int const RootItemIndex = 0;
  int const ChildItemsOffset = 1;
}

inline TIndex GetIndexChild(TIndex const & index, int i)
{
  TIndex child(index);
  if (child.m_group == TIndex::INVALID)
    child.m_group = i;
  else if (child.m_country == TIndex::INVALID)
    child.m_country = i;
  else if (child.m_region == TIndex::INVALID)
    child.m_region = i;

  return child;
}

inline TIndex GetIndexParent(TIndex const & index)
{
  ASSERT(index != TIndex(), ());
  TIndex parent(index);
  if (parent.m_region != TIndex::INVALID)
    parent.m_region = TIndex::INVALID;
  else if (parent.m_country != TIndex::INVALID)
    parent.m_country = TIndex::INVALID;
  else
    parent.m_group = TIndex::INVALID;

  return parent;
}

CountryTree::CountryTree(Framework & framework)
  : m_layout(framework)
{
  m_subscribeSlotID = GetStorage().Subscribe(bind(&CountryTree::NotifyStatusChanged, this, _1),
                                             bind(&CountryTree::NotifyProgressChanged, this, _1, _2));
}

CountryTree::~CountryTree()
{
  GetStorage().Unsubscribe(m_subscribeSlotID);
}

ActiveMapsLayout & CountryTree::GetActiveMapLayout()
{
  m_layout.Init();
  return m_layout;
}

void CountryTree::SetDefaultRoot()
{
  SetRoot(TIndex());
}

void CountryTree::SetParentAsRoot()
{
  ASSERT(HasParent(), ());
  SetRoot(GetIndexParent(GetCurrentRoot()));
}

void CountryTree::SetChildAsRoot(int childPosition)
{
  ASSERT(!IsLeaf(childPosition), ());
  SetRoot(GetChild(childPosition));
}

void CountryTree::ResetRoot()
{
  m_levelItems.clear();
}

bool CountryTree::HasRoot() const
{
  return !m_levelItems.empty();
}

bool CountryTree::HasParent() const
{
  return GetCurrentRoot() != TIndex();
}

int CountryTree::GetChildCount() const
{
  ASSERT(HasRoot(), ());
  return m_levelItems.size() - ChildItemsOffset;
}

bool CountryTree::IsLeaf(int childPosition) const
{
  return GetStorage().CountriesCount(GetChild(childPosition));
}

string const & CountryTree::GetChildName(int position) const
{
  return GetStorage().CountryName(GetChild(position));
}

TStatus CountryTree::GetLeafStatus(int position) const
{
  return GetStorage().CountryStatusEx(GetChild(position));
}

TMapOptions CountryTree::GetLeafOptions(int position) const
{
  TStatus status;
  TMapOptions options;
  GetStorage().CountryStatusEx(GetChild(position), status, options);
  return options;
}

void CountryTree::DownloadCountry(int childPosition, TMapOptions const & options)
{
  ASSERT(IsLeaf(childPosition), ());
  GetActiveMapLayout().DownloadMap(GetChild(childPosition), options);
}

void CountryTree::DeleteCountry(int childPosition, TMapOptions const & options)
{
  ASSERT(IsLeaf(childPosition), ());
  GetActiveMapLayout().DeleteMap(GetChild(childPosition), options);
}

void CountryTree::CancelDownloading(int childPosition)
{
  GetStorage().DeleteFromDownloader(GetChild(childPosition));
}

void CountryTree::SetListener(CountryTreeListener * listener)
{
  m_listener = listener;
}

void CountryTree::ResetListener()
{
  m_listener = nullptr;
}

Storage const & CountryTree::GetStorage() const
{
  return m_layout.GetStorage();
}

Storage & CountryTree::GetStorage()
{
  return m_layout.GetStorage();
}

TIndex const & CountryTree::GetCurrentRoot() const
{
  ASSERT(HasRoot(), ());
  return m_levelItems[RootItemIndex];
}

void CountryTree::SetRoot(TIndex const & index)
{
  ResetRoot();

  size_t const count = GetStorage().CountriesCount(index);
  m_levelItems.reserve(ChildItemsOffset + count);
  m_levelItems.push_back(index);
  for (size_t i = 0; i < count; ++i)
    m_levelItems.push_back(GetIndexChild(index, i));
}

TIndex const & CountryTree::GetChild(int childPosition) const
{
  ASSERT(ChildItemsOffset + childPosition < GetChildCount(), ());
  return m_levelItems[ChildItemsOffset + childPosition];
}

int CountryTree::GetChildPosition(TIndex const & index)
{
  int result = -1;
  if (HasRoot())
  {
    auto iter = find(m_levelItems.begin(), m_levelItems.end(), index);
    if (iter != m_levelItems.end())
      result = distance(m_levelItems.begin(), iter) - ChildItemsOffset;
  }

  return result;
}

void CountryTree::NotifyStatusChanged(TIndex const & index)
{
  if (m_listener != nullptr)
  {
    int position = GetChildPosition(index);
    if (position != -1)
      m_listener->ItemStatusChanged(position);
  }
}

void CountryTree::NotifyProgressChanged(TIndex const & index, LocalAndRemoteSizeT const & sizes)
{
  if (m_listener != nullptr)
  {
    int position = GetChildPosition(index);
    if (position != -1)
      m_listener->ItemProgressChanged(position, sizes);
  }
}

}
