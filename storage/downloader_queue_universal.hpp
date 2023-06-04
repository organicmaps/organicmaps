#pragma once

#include "storage/downloader_queue_interface.hpp"
#include "storage/queued_country.hpp"
#include "storage/storage_defines.hpp"

#include <list>
#include <utility>

namespace storage
{
class Queue : public QueueInterface
{
public:
  using ForEachCountryMutable = std::function<void(QueuedCountry & country)>;

  // QueueInterface overrides:
  bool IsEmpty() const override;
  size_t Count() const override;
  bool Contains(CountryId const & country) const override;
  void ForEachCountry(ForEachCountryFunction const & fn) const override;

  void ForEachCountry(ForEachCountryMutable const & fn);

  CountryId const & GetFirstId() const;
  QueuedCountry const & GetFirstCountry() const;
  void PopFront();

  void Append(QueuedCountry && country);

  void Remove(CountryId const & country);
  void Clear();

private:
  std::list<QueuedCountry> m_queue;
};
}  // namespace storage
