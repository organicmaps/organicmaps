#pragma once

#include "platform/get_text_by_id.hpp"

#include "std/string.hpp"
#include "std/unique_ptr.hpp"


namespace routing
{
namespace turns
{
namespace sound
{
struct Notification;

/// GetTtsText is responsible for generating text message for TTS in a specified locale
/// by notification. To get this message use operator().
/// If the message is not available for specified locale GetTtsText tries to find it in
/// English locale.
class GetTtsText
{
public:
  string operator()(Notification const & notification) const;
  /// TODO(vbykoianko) Check if locale is available. If not use default (en) locale.
  void SetLocale(string const & locale);
  inline string GetLocale() const { return m_locale; }
  void ForTestingSetLocaleWithJson(string const & jsonBuffer);

private:
  string GetTextById(string const & textId) const;

  unique_ptr<platform::GetTextById> m_getCurLang;
  string m_locale;
};

/// Generates text message id about the distance of the notification. For example: In 300 meters.
string GetDistanceTextId(Notification const & notification);
/// Generates text message id for roundabouts.
/// For example: "Leave the roundabout." or "Take the third exit."
string GetRoundaboutTextId(Notification const & notification);
/// Generates text message id about the direction of the notification. For example: Make a right
/// turn.
string GetDirectionTextId(Notification const & notification);
}  // namespace sound
}  // namespace turns
}  // namespace routing
