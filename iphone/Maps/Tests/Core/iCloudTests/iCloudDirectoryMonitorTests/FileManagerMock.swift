class FileManagerMock: FileManager {
  var stubUbiquityIdentityToken: UbiquityIdentityToken?
  var shouldReturnContainerURL: Bool = true
  var stubCloudDirectory: URL?

  override var ubiquityIdentityToken: (any UbiquityIdentityToken)? {
    stubUbiquityIdentityToken
  }

  override func url(forUbiquityContainerIdentifier _: String?) -> URL? {
    shouldReturnContainerURL ? stubCloudDirectory ?? URL(fileURLWithPath: NSTemporaryDirectory()) : nil
  }
}
