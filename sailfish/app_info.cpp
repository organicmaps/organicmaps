#include "sailfish/app_info.hpp"

#include "sailfish/bundled_html.hpp"
#include "sailfish/framework_access.hpp"
#include "sailfish/helpers.hpp"
#include "sailfish/voice_guide.hpp"

#include "map/framework.hpp"

#include "platform/platform.hpp"

#include "base/timer.hpp"

#include <QDateTime>

namespace sailfish
{
QString AppInfo::version() const
{
  return QString::fromStdString(GetPlatform().Version());
}

QStringList AppInfo::bookmarkColors() const
{
  return PresetColors();
}

QDate AppInfo::dataVersion() const
{
  auto const yymmdd = static_cast<uint32_t>(GetFramework().GetCurrentDataVersion());
  return QDateTime::fromTime_t(static_cast<uint>(base::YYMMDDToSecondsSinceEpoch(yymmdd)), Qt::UTC).date();
}

bool AppInfo::voiceSupported() const
{
  return kVoiceSupported;
}

QString AppInfo::localized(QString const & key, QStringList const & args) const
{
  return Localized(key, args);
}

QString AppInfo::formatSize(qint64 bytes) const
{
  return FormatSize(bytes);
}

QVariantList AppInfo::bundledHtml(QString const & file) const
{
  return BundledHtmlSections(file);
}
}  // namespace sailfish
