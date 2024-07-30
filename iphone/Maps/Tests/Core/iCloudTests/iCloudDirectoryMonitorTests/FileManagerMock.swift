class FileManagerMock: FileManager {
  var stubUbiquityIdentityToken: UbiquityIdentityToken?
  var shouldReturnUbiquityContainerURL: Bool = true

  override var ubiquityIdentityToken: (any UbiquityIdentityToken)? {
    return stubUbiquityIdentityToken
  }

  override func url(forUbiquityContainerIdentifier identifier: String?) -> URL? {
    return shouldReturnUbiquityContainerURL ? URL(fileURLWithPath: NSTemporaryDirectory()) : nil
  }
}
