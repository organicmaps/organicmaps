#ifndef FSQ_CONFIG_H
#define FSQ_CONFIG_H

#include <string>

#include "../Bricks/time/chrono.h"

#include "strategies.h"
#include "exponential_retry_strategy.h"

namespace fsq {

// Default strategy configuration for FSQ.
//
// The easiest way to create a user-specific configuration is to derive from this class
// and alter certain fields.
//
// FSQ derives itself from all strategy classes except T_PROCESSOR, T_TIME_MANAGER and T_FILE_MANAGER,
// thus allowing calling member setters for other policies directly on itself.

template <typename PROCESSOR>
struct Config {
  typedef PROCESSOR T_PROCESSOR;
  typedef std::string T_MESSAGE;
  typedef strategy::JustAppendToFile T_FILE_APPEND_STRATEGY;
  typedef strategy::AlwaysResume T_FILE_RESUME_STRATEGY;
  typedef strategy::DummyFileNamingToUnblockAlexFromMinsk T_FILE_NAMING_STRATEGY;
  template <typename FILESYSTEM>
  using T_RETRY_STRATEGY = strategy::ExponentialDelayRetryStrategy<FILESYSTEM>;
  typedef bricks::FileSystem T_FILE_SYSTEM;
  typedef strategy::UseEpochMilliseconds T_TIME_MANAGER;
  typedef strategy::KeepFilesAround100KBUnlessNoBacklog T_FINALIZE_STRATEGY;
  typedef strategy::KeepUnder20MBAndUnder1KFiles T_PURGE_STRATEGY;

  // Set to true to have FSQ detach the processing thread instead of joining it in destructor.
  inline static bool DetachProcessingThreadOnTermination() { return false; }

  // Set to false to have PushMessage() throw an exception when attempted to push while shutting down.
  inline static bool NoThrowOnPushMessageWhileShuttingDown() { return true; }

  // Set to true to have FSQ process all queued files in destructor before returning.
  inline static bool ProcessQueueToTheEndOnShutdown() { return false; }

  template <typename T_FSQ_INSTANCE>
  inline static void Initialize(T_FSQ_INSTANCE&) {
    // `T_CONFIG::Initialize(*this)` is invoked from FSQ's constructor
    // User-derived Config-s can put non-static initialization code here.
  }
};

}  // namespace fsq

#endif  // FSQ_CONFIG_H
