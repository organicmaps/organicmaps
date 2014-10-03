#include "country_tree.hpp"

#include "framework.hpp"

#include "../../storage/storage.hpp"

using namespace storage;

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

CountryTree::CountryTree(Framework * framework)
  : m_framework(framework)
{
  auto statusChangedFn = [this](TIndex const & index)
  {
    int childPosition = GetChildPosition(index);
    if (childPosition != -1)
      m_itemCallback(childPosition);
  };

  auto progressChangedFn = [this](TIndex const & index, LocalAndRemoteSizeT const & progress)
  {
    int childPosition = GetChildPosition(index);
    if (childPosition != -1)
      m_progressCallback(childPosition, progress);
  };

  m_subscribeSlotID = m_framework->Storage().Subscribe(statusChangedFn, progressChangedFn);
}

CountryTree::~CountryTree()
{
  m_framework->Storage().Unsubscribe(m_subscribeSlotID);
}

ActiveMapsLayout & CountryTree::GetActiveMapLayout()
{
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
  return m_framework->Storage().CountriesCount(GetChild(childPosition));
}

string const & CountryTree::GetChildName(int position) const
{
  return m_framework->Storage().CountryName(GetChild(position));
}

TStatus CountryTree::GetLeafStatus(int position) const
{
  return m_framework->Storage().CountryStatusEx(GetChild(position));
}

TMapOptions CountryTree::GetLeafOptions(int position) const
{
  TStatus status;
  TMapOptions options;
  m_framework->Storage().CountryStatusEx(GetChild(position), status, options);
  return options;
}

void CountryTree::DownloadCountry(int childPosition, TMapOptions const & options)
{
  ASSERT(IsLeaf(childPosition), ());
  m_framework->DownloadCountry(GetChild(childPosition), options);
}

void CountryTree::DeleteCountry(int childPosition, TMapOptions const & options)
{
  ASSERT(IsLeaf(childPosition), ());
  m_framework->DeleteCountry(GetChild(childPosition), options);
}

void CountryTree::CancelDownloading(int childPosition)
{
  m_framework->Storage().DeleteFromDownloader(GetChild(childPosition));
}

void CountryTree::SetItemChangedListener(TItemChangedFn const & callback)
{
  m_itemCallback = callback;
}

void CountryTree::SetItemProgressListener(TItemProgressChangedFn const & callback)
{
  m_progressCallback = callback;
}

TIndex const & CountryTree::GetCurrentRoot() const
{
  ASSERT(HasRoot(), ());
  return m_levelItems[RootItemIndex];
}

void CountryTree::SetRoot(TIndex const & index)
{
  ResetRoot();

  size_t const count = m_framework->Storage().CountriesCount(index);
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

int CountryTree::GetChildPosition(const TIndex & index)
{
  int result = -1;
  if (HasRoot())
  {
    for (size_t i = ChildItemsOffset; i < m_levelItems.size(); ++i)
    {
      if (m_levelItems[i] == index)
        result = i;
    }
  }

  return result;
}
