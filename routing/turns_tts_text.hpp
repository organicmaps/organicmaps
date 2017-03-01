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
  /// \brief Sets a locale.
  /// @param locale is a string representation of locale. For example "en", "ru", "zh-Hant" and so on.
  /// \note See sound/tts/languages.txt for the full list of available locales.
  void SetLocale(string const & locale);
  /// @return current TTS locale. For example "en", "ru", "zh-Hant" and so on.
  /// \note The method returns correct locale after SetLocale has been called.
  /// If not, it returns an empty string.
  string GetLocale() const;

  void ForTestingSetLocaleWithJson(string const & jsonBuffer, string const & locale);

private:
  string GetTextById(string const & textId) const;

  unique_ptr<platform::GetTextById> m_getCurLang;
};

/// Generates text message id about the distance of the notification. For example: In 300 meters.
string GetDistanceTextId(Notification const & notification);
/// Generates text message id for roundabouts.
/// For example: leave_the_roundabout or take_the_3_exit
string GetRoundaboutTextId(Notification const & notification);
/// Generates text message id for the finish of the route.
/// For example: destination or you_have_reached_the_destination
string GetYouArriveTextId(Notification const & notification);
/// Generates text message id about the direction of the notification. For example: Make a right
/// turn.
string GetDirectionTextId(Notification const & notification);
}  // namespace sound
}  // namespace turns
}  // namespace routing
