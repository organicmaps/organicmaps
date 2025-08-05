@objc
class ExtendedBackgroundFetchTask: NSObject, BackgroundFetchTask {
  private var identifier = UIBackgroundTaskIdentifier.invalid

  func start() {
    identifier = UIApplication.shared.beginBackgroundTask(withName: description,
                                                          expirationHandler: {
      self.finish(.failed)
    })
    if identifier != UIBackgroundTaskIdentifier.invalid {
      fire()
    }
  }

  fileprivate func fire() {
    finish(.failed)
  }

  fileprivate func finish(_ result: UIBackgroundFetchResult) {
    LOG(.info, "Extended background edits uploading task is finished with result: \(result)")
    guard identifier != UIBackgroundTaskIdentifier.invalid else { return }
    UIApplication.shared.endBackgroundTask(UIBackgroundTaskIdentifier(rawValue: identifier.rawValue))
    identifier = UIBackgroundTaskIdentifier.invalid
  }
}

@objc
final class ExtendedBackgroundEditsUploadingTask: ExtendedBackgroundFetchTask {
  override fileprivate func fire() {
    MWMEditorHelper.uploadEdits(self.finish)
  }

  override var description: String {
    return "Edits upload"
  }
}
