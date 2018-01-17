@objc(MWMBackgroundDownloadMapNotification)
final class BackgroundDownloadMapNotification: BackgroundFetchTask {
  override var queue: DispatchQueue { return .main }
  override var frameworkType: BackgroundFetchTaskFrameworkType { return .full }

  override lazy var block = {
    LocalNotificationManager.shared().showDownloadMapNotificationIfNeeded(self.finish)
  }
}
