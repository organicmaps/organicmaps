#pragma once

#include "routing/routing_callbacks.hpp"

#include "geometry/latlon.hpp"

#include "base/cancellable.hpp"
#include "base/timer.hpp"

#include <mutex>

namespace routing
{
class RouterDelegate
{
public:
  static auto constexpr kNoTimeout = std::numeric_limits<uint32_t>::max();

  RouterDelegate();
  // Parent cancellation and deadlines also stop this synchronous subcalculation.
  explicit RouterDelegate(base::Cancellable const & parent);

  /// Set routing progress. Waits current progress status from 0 to 100.
  void OnProgress(float progress) const;
  void OnPointCheck(ms::LatLon const & point) const;

  void SetProgressCallback(ProgressCallback const & progressCallback);
  void SetPointCheckCallback(PointCheckCallback const & pointCallback);

  void SetTimeout(uint32_t timeoutSec);

  base::Cancellable const & GetCancellable() const { return m_cancellable; }
  void Reset() { return m_cancellable.Reset(); }
  void Cancel() { return m_cancellable.Cancel(); }
  bool IsCancelled() const { return m_cancellable.IsCancelled(); }

private:
  ProgressCallback m_progressCallback;
  PointCheckCallback m_pointCallback;

  class Cancellable final : public base::Cancellable
  {
  public:
    explicit Cancellable(base::Cancellable const * parent = nullptr) : m_parent(parent) {}
    bool IsCancelled() const override
    {
      return base::Cancellable::IsCancelled() || (m_parent && m_parent->IsCancelled());
    }
    Status CancellationStatus() const override
    {
      auto const status = base::Cancellable::CancellationStatus();
      return status != Status::Active || !m_parent ? status : m_parent->CancellationStatus();
    }

  private:
    base::Cancellable const * m_parent;
  };
  Cancellable m_cancellable;
};
}  //  namespace routing
