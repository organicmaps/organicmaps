class FileManagerMock: FileManager {
  var stubUbiquityIdentityToken: UbiquityIdentityToken?
  var shouldReturnContainerURL: Bool = true
  var stubCloudDirectory: URL?
  /// Every container lookup waits until this group is left: the test enters it to keep iCloud from answering
  /// while it observes what a start does.
  let containerUrlLookup = DispatchGroup()

  override var ubiquityIdentityToken: (any UbiquityIdentityToken)? {
    stubUbiquityIdentityToken
  }

  override func url(forUbiquityContainerIdentifier _: String?) -> URL? {
    containerUrlLookup.wait()
    return shouldReturnContainerURL ? stubCloudDirectory ?? URL(fileURLWithPath: NSTemporaryDirectory()) : nil
  }
}
