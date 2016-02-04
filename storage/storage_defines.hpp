#pragma once

#include "storage/index.hpp"

#include "std/cstdint.hpp"
#include "std/function.hpp"
#include "std/string.hpp"
#include "std/utility.hpp"

namespace storage
{
  /// Inner status which is used inside Storage class
  enum class Status : uint8_t
  {
    EUndefined = 0,
    EOnDisk,          /**< Downloaded mwm(s) is up to date. No need to update it. */
    ENotDownloaded,   /**< Mwm can be download but not downloaded yet. */
    EDownloadFailed,  /**< Downloading failed because no internet connection. */
    EDownloading,     /**< Downloading a new mwm or updating an old one. */
    EInQueue,         /**< A mwm is waiting for downloading in the queue. */
    EUnknown,         /**< Downloading failed because of unknown error. */
    EOnDiskOutOfDate, /**< An update for a downloaded mwm is ready according to counties.txt. */
    EOutOfMemFailed,  /**< Downloading failed because it's not enough memory */
    EMixed,           /**< Descendants of a group node have different statuses. */
  };
  string DebugPrint(Status status);

  enum class NodeStatus
  {
    Undefined,
    Error,            /**< An error happened while downloading */
    OnDisk,           /**< Downloaded mwm(s) is up to date. No need to update it. */
    NotDownloaded,    /**< Mwm can be download but not downloaded yet. */
    Downloading,      /**< Downloading a new mwm or updating an old one. */
    InQueue,          /**< A mwm is waiting for downloading in the queue. */
    OnDiskOutOfDate,  /**< An update for a downloaded mwm is ready according to counties.txt. */
    Mixed,            /**< Descendants of a group node have different statuses. */
  };
  string DebugPrint(NodeStatus status);

  enum class NodeErrorCode
  {
    NoError,
    UnknownError,     /**< Downloading failed because of unknown error. */
    OutOfMemFailed,   /**< Downloading failed because it's not enough memory */
    NoInetConnection, /**< Downloading failed because internet connection was interrupted */
  };
  string DebugPrint(NodeErrorCode status);

  struct StatusAndError
  {
    StatusAndError(NodeStatus nodeStatus, NodeErrorCode nodeError) :
      status(nodeStatus), error(nodeError) {}

    bool operator==(StatusAndError const & other)
    {
      return other.status == status && other.error == error;
    }

    NodeStatus status;
    NodeErrorCode error;
  };
  string DebugPrint(StatusAndError statusAndError);

  using TLocalAndRemoteSize = pair<uint64_t, uint64_t>;

  StatusAndError ParseStatus(Status innerStatus);
}  // namespace storage

using TDownloadFn = function<void (storage::TCountryId const &)>;
