import BackgroundTasks

enum BackgroundTaskError: LocalizedError {
  case taskIsExpired
  case taskIsFailed
  case backgroundRefreshStatusIsDenied
  case backgroundRefreshStatusIsRestricted

  var errorDescription: String? {
    switch self {
    case .taskIsExpired:
      return "Extended background fetch task is expired"
    case .taskIsFailed:
      return "Extended background edits uploading task is failed"
    case .backgroundRefreshStatusIsDenied:
      return "Background App Refresh is denied for this app. The user may have disabled it in Settings."
    case .backgroundRefreshStatusIsRestricted:
      return "Background App Refresh is restricted for this app (e.g., due to parental controls or low power mode)."
    }
  }
}

typealias BackgroundTaskID = String
typealias BackgroundTaskCompletionHandler = (Error?) -> Void

@objc @available(iOS 13.0, *)
final class ScheduledBackgroundEditsUploadingTask: NSObject {
  static private let identifier: BackgroundTaskID = "app.organicmaps.backgroundProcessingTask.uploadEdits"
  static private var isSubmitted = false

  @objc
  static func register(completion: @escaping BackgroundTaskCompletionHandler) {
    let isRegistered = BGTaskScheduler.shared.register(forTaskWithIdentifier: identifier, using: nil) { task in
      fire(task, completion: completion)
    }
    if isRegistered {
      LOG(.info, "BGTask '\(identifier)' registered successfully.")
    } else {
      fatalError("Make sure '\(identifier)' is included in Info.plist under 'BGTaskSchedulerPermittedIdentifiers'.")
    }
  }

  @objc
  static func schedule(completion: @escaping BackgroundTaskCompletionHandler) {
    // The status should be checked first because the user can enable low power mode between the app launches.
    let status = UIApplication.shared.backgroundRefreshStatus
    switch status {
    case .restricted:
      completion(BackgroundTaskError.backgroundRefreshStatusIsRestricted)
      return
    case .denied:
      completion(BackgroundTaskError.backgroundRefreshStatusIsDenied)
      return
    case .available:
      break
    @unknown default:
      fatalError("Unknown background refresh status: \(status)")
    }

    guard !isSubmitted else {
      LOG(.info, "BGTask '\(identifier)' is already scheduled.")
      completion(nil)
      return
    }

    BGTaskScheduler.shared.getPendingTaskRequests { pendingRequests in
      if pendingRequests.contains(where: { $0.identifier == identifier }) {
        LOG(.info, "BGTask '\(identifier)' is already pending (previous session).")
        isSubmitted = true
        completion(nil)
        return
      }

      do {
        try BGTaskScheduler.shared.submit(request)
        isSubmitted = true
        LOG(.info, "BGTask '\(identifier)' is scheduled.")
        completion(nil)
      } catch {
        isSubmitted = false
        completion(error)
      }
    }
  }

  static private var request: BGTaskRequest {
    let request = BGProcessingTaskRequest(identifier: Self.identifier)
    request.requiresNetworkConnectivity = true
    request.requiresExternalPower = false
    request.earliestBeginDate = Date(timeIntervalSinceNow: 1)
    return request
  }

  static private func fire(_ task: BGTask, completion: @escaping BackgroundTaskCompletionHandler) {
    LOG(.info, "Scheduled BGTask '\(identifier)' is triggered.")

    task.expirationHandler = {
      LOG(.error, "BGTask '\(identifier)' is expired. Cancelling work and marking as failed.")
      // TODO: (KK) Cancel MWMEditorHelper.uploadEdits if possible
      task.setTaskCompleted(success: false)
      isSubmitted = false
      completion(BackgroundTaskError.taskIsExpired)
    }

    MWMEditorHelper.uploadEdits { result in
      isSubmitted = false
      switch result {
      case .newData, .noData:
        task.setTaskCompleted(success: true)
        completion(nil)
      case .failed:
        task.setTaskCompleted(success: false)
        completion(BackgroundTaskError.taskIsFailed)
      @unknown default:
        LOG(.critical, "BG task '\(identifier)' returned unknown result; marking as failed.")
        task.setTaskCompleted(success: false)
        completion(BackgroundTaskError.taskIsFailed)
      }
    }
  }
}
