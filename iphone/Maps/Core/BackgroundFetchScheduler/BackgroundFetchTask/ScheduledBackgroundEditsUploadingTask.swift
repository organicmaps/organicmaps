import BackgroundTasks

@objc @available(iOS 13.0, *)
final class ScheduledBackgroundEditsUploadingTask: NSObject, BackgroundFetchTask {
  static let identifier = "com.backgroundTask.uploadEdits"

  static func register() {
    BGTaskScheduler.shared.register(forTaskWithIdentifier: identifier, using: nil) { task in
      LOG(.info, "Background edits upload task is triggered.")
      task.expirationHandler = {
        LOG(.error, "Scheduled background edits upload task is expired.")
      }
      MWMEditorHelper.uploadEdits { result in
        switch result {
        case .newData, .noData:
          task.setTaskCompleted(success: true)
        case .failed:
          task.setTaskCompleted(success: false)
        @unknown default:
          fatalError("Unknown result type received in background task.")
        }
      }
    }
  }

  func start() {
    scheduleTask()
  }

  private func scheduleTask() {
    let request = BGProcessingTaskRequest(identifier: Self.identifier)
    request.requiresNetworkConnectivity = true
    request.requiresExternalPower = false
    request.earliestBeginDate = Date()

    do {
      try BGTaskScheduler.shared.submit(request)
      LOG(.info, "Background edits upload task is scheduled.")
    } catch {
      LOG(.error ,"Could not schedule background upload task: \(error.localizedDescription)")
    }
  }
}
