class FileManagerMock: FileManager {
  var stubUbiquityIdentityToken: UbiquityIdentityToken?
  var shouldReturnContainerURL: Bool = true
  var stubCloudDirectory: URL?

  override var ubiquityIdentityToken: (any UbiquityIdentityToken)? {
    return stubUbiquityIdentityToken
  }

  override func url(forUbiquityContainerIdentifier identifier: String?) -> URL? {
    return shouldReturnContainerURL ? stubCloudDirectory ?? URL(fileURLWithPath: NSTemporaryDirectory()) :  nil
  }
}
