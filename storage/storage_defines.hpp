#pragma once

#include "storage/index.hpp"

#include <cstdint>
#include <functional>
#include <string>
#include <utility>

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
    EApplying,        /**< Applying downloaded diff for an old mwm. */
    EInQueue,         /**< A mwm is waiting for downloading in the queue. */
    EUnknown,         /**< Downloading failed because of unknown error. */
    EOnDiskOutOfDate, /**< An update for a downloaded mwm is ready according to counties.txt. */
    EOutOfMemFailed,  /**< Downloading failed because it's not enough memory */
  };
  std::string DebugPrint(Status status);

  /// \note The order of enum items is important. It is used in Storage::NodeStatus method.
  /// If it's necessary to add more statuses it's better to add to the end.
  enum class NodeStatus
  {
    Undefined,
    Downloading,      /**< Downloading a new mwm or updating an old one. */
    Applying,         /**< Applying downloaded diff for an old mwm. */
    InQueue,          /**< An mwm is waiting for downloading in the queue. */
    Error,            /**< An error happened while downloading */
    OnDiskOutOfDate,  /**< An update for a downloaded mwm is ready according to counties.txt. */
    OnDisk,           /**< Downloaded mwm(s) is up to date. No need to update it. */
    NotDownloaded,    /**< An mwm can be downloaded but not downloaded yet. */
    Partly,           /**< Leafs of group node has a mix of NotDownloaded and OnDisk status. */
  };
  std::string DebugPrint(NodeStatus status);

  enum class NodeErrorCode
  {
    NoError,
    UnknownError,     /**< Downloading failed because of unknown error. */
    OutOfMemFailed,   /**< Downloading failed because it's not enough memory */
    NoInetConnection, /**< Downloading failed because internet connection was interrupted */
  };
  std::string DebugPrint(NodeErrorCode status);

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
  std::string DebugPrint(StatusAndError statusAndError);

  StatusAndError ParseStatus(Status innerStatus);
}  // namespace storage

using TDownloadFn = std::function<void(storage::TCountryId const &)>;
