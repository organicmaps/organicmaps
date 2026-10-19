#include <ctime>
#include "platform/localization.hpp"

#include <QtCore/QCollator>
#include <QtCore/QLocale>

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
  return type;
}

std::string GetLocalizedBrandName(std::string const & brand)
{
  return brand;
}

std::string GetLocalizedString(std::string const & key)
{
  return key;
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
