#include "map/country_status_display.hpp"
#include "map/framework.hpp"

#include "gui/controller.hpp"
#include "gui/button.hpp"
#include "gui/text_view.hpp"

#include "graphics/overlay_renderer.hpp"
#include "graphics/display_list.hpp"

#include "platform/platform.hpp"

#include "base/thread.hpp"
#include "base/string_format.hpp"

#include "std/bind.hpp"
#include "std/sstream.hpp"


using namespace storage;

CountryStatusDisplay::CountryStatusDisplay(Params const & p)
  : gui::Element(p)
  , m_activeMaps(p.m_activeMaps)
{
  m_activeMapsSlotID = m_activeMaps.AddListener(this);
  gui::Button::Params bp;

  bp.m_depth = depth();
  bp.m_minWidth = 200;
  bp.m_minHeight = 40;
  bp.m_position = graphics::EPosCenter;

  auto createButtonFn = [this] (gui::Button::Params const & params)
  {
    gui::Button * result = new gui::Button(params);
    result->setIsVisible(false);
    result->setOnClickListener(bind(&CountryStatusDisplay::OnButtonClicked, this, _1));

    result->setFont(EActive, graphics::FontDesc(16, graphics::Color(255, 255, 255, 255)));
    result->setFont(EPressed, graphics::FontDesc(16, graphics::Color(255, 255, 255, 255)));

    result->setColor(EActive, graphics::Color(0, 0, 0, 0.6 * 255));
    result->setColor(EPressed, graphics::Color(0, 0, 0, 0.4 * 255));

    return result;
  };

  m_primaryButton.reset(createButtonFn(bp));
  m_secondaryButton.reset(createButtonFn(bp));

  gui::TextView::Params tp;
  tp.m_depth = depth();
  tp.m_position = graphics::EPosCenter;

  m_label.reset(new gui::TextView(tp));
  m_label->setIsVisible(false);
  m_label->setFont(gui::Element::EActive, graphics::FontDesc(18));

  setIsVisible(false);
}

CountryStatusDisplay::~CountryStatusDisplay()
{
  m_activeMaps.RemoveListener(m_activeMapsSlotID);
}

void CountryStatusDisplay::SetCountryIndex(TIndex const & idx)
{
  if (m_countryIdx != idx)
  {
    Lock();
    m_countryIdx = idx;

    if (m_countryIdx.IsValid())
    {
      m_countryStatus = m_activeMaps.GetCountryStatus(m_countryIdx);
      m_displayMapName = m_activeMaps.GetFormatedCountryName(m_countryIdx);
    }

    Repaint();
    Unlock();
  }
}

void CountryStatusDisplay::setIsVisible(bool isVisible) const
{
  if (isVisible && isVisible != TBase::isVisible())
  {
    Lock();
    Repaint();
    Unlock();
  }

  TBase::setIsVisible(isVisible);
}

void CountryStatusDisplay::setIsDirtyLayout(bool isDirty) const
{
  TBase::setIsDirtyLayout(isDirty);
  m_label->setIsDirtyLayout(isDirty);
  m_primaryButton->setIsDirtyLayout(isDirty);
  m_secondaryButton->setIsDirtyLayout(isDirty);

  if (isDirty)
    SetVisibilityForState();
}

void CountryStatusDisplay::draw(graphics::OverlayRenderer * r,
                                math::Matrix<double, 3, 3> const & m) const
{
  if (isVisible())
  {
    Lock();
    checkDirtyLayout();

    m_label->draw(r, m);
    m_primaryButton->draw(r, m);
    m_secondaryButton->draw(r, m);
    Unlock();
  }
}

void CountryStatusDisplay::layout()
{
  if (!isVisible())
    return;
  SetContentForState();

  auto layoutFn = [] (gui::Element * e)
  {
    if (e->isVisible())
      e->layout();
  };

  layoutFn(m_label.get());
  layoutFn(m_primaryButton.get());
  layoutFn(m_secondaryButton.get());

  ComposeElementsForState();

  // !!!! Hack !!!!
  // ComposeElementsForState modify pivot point of elements.
  // After setPivot all elements must be relayouted.
  // For reduce "cache" operations we call layout secondary
  layoutFn(m_label.get());
  layoutFn(m_primaryButton.get());
  layoutFn(m_secondaryButton.get());
}

void CountryStatusDisplay::purge()
{
  m_label->purge();
  m_primaryButton->purge();
  m_secondaryButton->purge();
}

void CountryStatusDisplay::cache()
{
  auto cacheFn = [] (gui::Element * e)
  {
    if (e->isVisible())
      e->cache();
  };

  cacheFn(m_label.get());
  cacheFn(m_primaryButton.get());
  cacheFn(m_secondaryButton.get());
}

m2::RectD CountryStatusDisplay::GetBoundRect() const
{
  ASSERT(isVisible(), ());
  m2::RectD r(pivot(), pivot());

  if (m_primaryButton->isVisible())
    r.Add(m_primaryButton->GetBoundRect());
  if (m_secondaryButton->isVisible())
    r.Add(m_secondaryButton->GetBoundRect());

  return r;
}

void CountryStatusDisplay::setController(gui::Controller * controller)
{
  Element::setController(controller);
  m_label->setController(controller);
  m_primaryButton->setController(controller);
  m_secondaryButton->setController(controller);
}

bool CountryStatusDisplay::onTapStarted(m2::PointD const & pt)
{
  return OnTapAction(bind(&gui::Button::onTapStarted, _1, _2), pt);
}

bool CountryStatusDisplay::onTapMoved(m2::PointD const & pt)
{
  return OnTapAction(bind(&gui::Button::onTapMoved, _1, _2), pt);
}

bool CountryStatusDisplay::onTapEnded(m2::PointD const & pt)
{
  return OnTapAction(bind(&gui::Button::onTapEnded, _1, _2), pt);
}

bool CountryStatusDisplay::onTapCancelled(m2::PointD const & pt)
{
  return OnTapAction(bind(&gui::Button::onTapCancelled, _1, _2), pt);
}

void CountryStatusDisplay::CountryStatusChanged(ActiveMapsLayout::TGroup const & group, int position,
                                                TStatus const & /*oldStatus*/, TStatus const & newStatus)
{
  TIndex index = m_activeMaps.GetCoreIndex(group, position);
  if (m_countryIdx == index)
  {
    Lock();
    m_countryStatus = newStatus;
    if (m_countryStatus == TStatus::EDownloading)
      m_progressSize = m_activeMaps.GetDownloadableCountrySize(m_countryIdx);
    else
      m_progressSize = LocalAndRemoteSizeT(0, 0);
    Repaint();
    Unlock();
  }
}

void CountryStatusDisplay::DownloadingProgressUpdate(ActiveMapsLayout::TGroup const & group, int position, LocalAndRemoteSizeT const & progress)
{
  TIndex index = m_activeMaps.GetCoreIndex(group, position);
  if (m_countryIdx == index)
  {
    Lock();
    m_countryStatus = m_activeMaps.GetCountryStatus(index);
    m_progressSize = progress;
    Repaint();
    Unlock();
  }
}

template <class T1, class T2>
string CountryStatusDisplay::FormatStatusMessage(string const & msgID, T1 const * t1, T2 const * t2)
{
  string msg = m_controller->GetStringsBundle()->GetString(msgID);
  if (t1)
  {
    if (t2)
      msg = strings::Format(msg, *t1, *t2);
    else
    {
      msg = strings::Format(msg, *t1);

      size_t const count = msg.size();
      if (count > 0)
      {
        if (msg[count-1] == '\n')
          msg.erase(count-1, 1);
      }
    }
  }

  return msg;
}

void CountryStatusDisplay::SetVisibilityForState() const
{
  uint8_t visibilityFlags = 0;
  uint8_t const labelVisibility = 0x1;
  uint8_t const primeVisibility = 0x2;
  uint8_t const secondaryVisibility = 0x4;

  if (m_countryIdx.IsValid())
  {
    switch (m_countryStatus)
    {
    case TStatus::EDownloadFailed:
      visibilityFlags |= labelVisibility;
      visibilityFlags |= primeVisibility;
      break;
    case TStatus::EDownloading:
    case TStatus::EInQueue:
      visibilityFlags |= labelVisibility;
      break;
    case TStatus::ENotDownloaded:
      visibilityFlags |= labelVisibility;
      visibilityFlags |= primeVisibility;
      visibilityFlags |= secondaryVisibility;
      break;
    default:
      break;
    }
  }

  m_label->setIsVisible(visibilityFlags & labelVisibility);
  m_primaryButton->setIsVisible(visibilityFlags & primeVisibility);
  m_secondaryButton->setIsVisible(visibilityFlags & secondaryVisibility);

  TBase::setIsVisible(m_label->isVisible() || m_primaryButton->isVisible() || m_secondaryButton->isVisible());
}

void CountryStatusDisplay::SetContentForState()
{
  if (!isVisible())
    return;

  switch (m_countryStatus)
  {
  case TStatus::EDownloadFailed:
  case TStatus::EOutOfMemFailed:
    SetContentForError();
    break;
  case TStatus::ENotDownloaded:
    SetContentForDownloadPropose();
    break;
  case TStatus::EDownloading:
    SetContentForProgress();
    break;
  case TStatus::EInQueue:
    SetContentForInQueue();
    break;
  default:
    break;
  }
}

namespace
{
  void FormatMapSize(uint64_t sizeInBytes, string & units, uint64_t & sizeToDownload)
  {
    int const mbInBytes = 1024 * 1024;
    int const kbInBytes = 1024;
    if (sizeInBytes < mbInBytes)
    {
      sizeToDownload = (sizeInBytes + kbInBytes / 2) / kbInBytes;
      units = "KB";
    }
    else
    {
      sizeToDownload = (sizeInBytes + mbInBytes / 2) / mbInBytes;
      units = "MB";
    }
  }
}

void CountryStatusDisplay::SetContentForDownloadPropose()
{
  ASSERT(m_label->isVisible(), ());
  ASSERT(m_primaryButton->isVisible(), ());
  ASSERT(m_secondaryButton->isVisible(), ());

  LocalAndRemoteSizeT mapAndRoutingSize = m_activeMaps.GetRemoteCountrySizes(m_countryIdx);

  m_label->setText(m_displayMapName);
  uint64_t sizeToDownload;
  string units;
  FormatMapSize(mapAndRoutingSize.first + mapAndRoutingSize.second, units, sizeToDownload);
  m_primaryButton->setText(FormatStatusMessage("country_status_download_routing", &sizeToDownload, &units));

  FormatMapSize(mapAndRoutingSize.first, units, sizeToDownload);
  m_secondaryButton->setText(FormatStatusMessage("country_status_download", &sizeToDownload, &units));
}

void CountryStatusDisplay::SetContentForProgress()
{
  ASSERT(m_label->isVisible(), ());
  int percent = 0;
  if (m_progressSize.second != 0)
    percent = m_progressSize.first * 100 / m_progressSize.second;
  m_label->setText(FormatStatusMessage<string, int>("country_status_downloading", &m_displayMapName, &percent));
}

void CountryStatusDisplay::SetContentForInQueue()
{
  ASSERT(m_label->isVisible(), ());
  m_label->setText(FormatStatusMessage<string, int>("country_status_added_to_queue", &m_displayMapName));
}

void CountryStatusDisplay::SetContentForError()
{
  ASSERT(m_label->isVisible(), ());
  ASSERT(m_primaryButton->isVisible(), ());

  if (m_countryStatus == TStatus::EDownloadFailed)
    m_label->setText(FormatStatusMessage<string, int>("country_status_download_failed", &m_displayMapName));
  else
    m_label->setText(FormatStatusMessage<int, int>("not_enough_free_space_on_sdcard"));

  m_primaryButton->setText(m_controller->GetStringsBundle()->GetString("try_again"));
}

void CountryStatusDisplay::ComposeElementsForState()
{
  ASSERT(isVisible(), ());
  int visibleCount = 0;
  auto visibleCheckFn = [&visibleCount] (gui::Element const * e)
  {
    if (e->isVisible())
      ++visibleCount;
  };

  visibleCheckFn(m_label.get());
  visibleCheckFn(m_primaryButton.get());
  visibleCheckFn(m_secondaryButton.get());

  ASSERT(visibleCount > 0, ());

  m2::PointD const & pv = pivot();
  if (visibleCount == 1)
    m_label->setPivot(pv);
  else if (visibleCount == 2)
  {
    size_t const labelHeight = m_label->GetBoundRect().SizeY();
    size_t const buttonHeight = m_primaryButton->GetBoundRect().SizeY();
    size_t const commonHeight = buttonHeight + labelHeight + 10 * visualScale();

    m_label->setPivot(m2::PointD(pv.x, pv.y - commonHeight / 2 + labelHeight / 2));
    m_primaryButton->setPivot(m2::PointD(pv.x, pv.y + commonHeight / 2 - buttonHeight / 2));
  }
  else
  {
    size_t const labelHeight = m_label->GetBoundRect().SizeY();
    size_t const primButtonHeight = m_primaryButton->GetBoundRect().SizeY();
    size_t const secButtonHeight = m_secondaryButton->GetBoundRect().SizeY();
    size_t const emptySpace = 10 * visualScale();

    double const offsetFromCenter = (primButtonHeight / 2 + emptySpace);

    m_label->setPivot(m2::PointD(pv.x, pv.y - offsetFromCenter - labelHeight / 2));
    m_primaryButton->setPivot(pv);
    m_secondaryButton->setPivot(m2::PointD(pv.x, pv.y + offsetFromCenter + secButtonHeight / 2.0));
  }
}

bool CountryStatusDisplay::OnTapAction(TTapActionFn const & action, const m2::PointD & pt)
{
  bool result = false;
  if (m_primaryButton->isVisible() && m_primaryButton->hitTest(pt))
    result |= action(m_primaryButton, pt);
  else if (m_secondaryButton->isVisible() && m_secondaryButton->hitTest(pt))
    result |= action(m_secondaryButton, pt);

  return result;
}

void CountryStatusDisplay::OnButtonClicked(gui::Element const * button)
{
  ASSERT(m_countryIdx.IsValid(), ());

  TMapOptions options = TMapOptions::EMap;
  if (button == m_primaryButton.get())
    options = SetOptions(options, TMapOptions::ECarRouting);

  ASSERT(m_downloadCallback, ());
  int opt = static_cast<int>(options);
  if (IsStatusFailed())
    opt = -1;

  m_downloadCallback(m_countryIdx, opt);
}

void CountryStatusDisplay::Repaint() const
{
  setIsDirtyLayout(true);
  const_cast<CountryStatusDisplay *>(this)->invalidate();
}

bool CountryStatusDisplay::IsStatusFailed() const
{
  return m_countryStatus == TStatus::EOutOfMemFailed || m_countryStatus == TStatus::EDownloadFailed;
}

void CountryStatusDisplay::Lock() const
{
#ifdef OMIM_OS_ANDROID
  m_mutex.Lock();
#endif
}

void CountryStatusDisplay::Unlock() const
{
#ifdef OMIM_OS_ANDROID
  m_mutex.Unlock();
#endif
}
