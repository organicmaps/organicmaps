@objc(MWMBackgroundUGCUpload)
final class BackgroundUGCUpload: BackgroundFetchTask {
  override var queue: DispatchQueue { return .main }
  override var frameworkType: BackgroundFetchTaskFrameworkType { return .full }

  override lazy var block = {
    MWMUGCHelper.uploadEdits(self.finish)
  }
}
