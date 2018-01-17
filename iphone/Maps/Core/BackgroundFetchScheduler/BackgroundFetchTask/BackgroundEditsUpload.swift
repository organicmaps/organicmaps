@objc(MWMBackgroundEditsUpload)
final class BackgroundEditsUpload: BackgroundFetchTask {
  override lazy var block = {
    MWMEditorHelper.uploadEdits(self.finish)
  }
}
