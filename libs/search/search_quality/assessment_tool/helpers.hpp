#pragma once

#include <string>
#include <utility>

#include <QtCore/QString>

namespace strings
{
class UniString;
}

QString ToQString(strings::UniString const & s);
QString ToQString(std::string const & s);

template <typename Layout, typename... Args>
Layout * BuildLayout(Args &&... args)
{
  return new Layout(std::forward<Args>(args)...);
}

template <typename Layout, typename... Args>
Layout * BuildLayoutWithoutMargins(Args &&... args)
{
  auto * layout = BuildLayout<Layout>(std::forward<Args>(args)...);
  layout->setContentsMargins(0 /* left */, 0 /* top */, 0 /* right */, 0 /* bottom */);
  return layout;
}
