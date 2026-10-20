#include "platform/localization.hpp"

#include <QtCore/QCollator>
#include <QtCore/QCoreApplication>
#include <QtCore/QLocale>

#include <algorithm>
#include <ctime>

namespace platform
{
namespace
{
class QtStringCollator final : public StringCollator
{
public:
  explicit QtStringCollator(std::string const & locale) : m_collator(QLocale(QString::fromStdString(locale))) {}

  bool Less(std::string const & lhs, std::string const & rhs) const override
  {
    return m_collator.compare(QString::fromStdString(lhs), QString::fromStdString(rhs)) < 0;
  }

private:
  QCollator m_collator;
};
}  // namespace

std::unique_ptr<StringCollator> CreateStringCollator(std::string const & locale)
{
  return std::make_unique<QtStringCollator>(locale);
}

std::string GetLocalizedTypeName(std::string const & type)
{
  auto key = "type." + type;
  std::replace(key.begin(), key.end(), '-', '.');
  std::replace(key.begin(), key.end(), ':', '_');
  auto const translated = QCoreApplication::translate(nullptr, key.c_str());
  // Command-line clients do not install catalogs and retain the readable classificator name.
  return translated == QString::fromStdString(key) ? type : translated.toStdString();
}

std::string GetLocalizedBrandName(std::string const & brand)
{
  return brand;
}

std::string GetLocalizedString(std::string const & key)
{
  return QCoreApplication::translate(nullptr, key.c_str()).toStdString();
}

std::string GetCurrencySymbol(std::string const & currencyCode)
{
  return currencyCode;
}

std::string GetLocalizedMyPositionBookmarkName()
{
  std::time_t t = std::time(nullptr);
  char buf[100] = {0};
  (void)std::strftime(buf, sizeof(buf), "%Ec", std::localtime(&t));
  return buf;
}
}  // namespace platform
