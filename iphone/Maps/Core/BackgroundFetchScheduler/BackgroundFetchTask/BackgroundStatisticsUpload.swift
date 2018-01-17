@objc(MWMBackgroundStatisticsUpload)
final class BackgroundStatisticsUpload: BackgroundFetchTask {
  override lazy var block = {
    Alohalytics.forceUpload(self.finish)
  }
}
