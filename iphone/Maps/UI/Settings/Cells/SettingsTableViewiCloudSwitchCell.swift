final class SettingsTableViewiCloudSwitchCell: SettingsTableViewDetailedSwitchCell {

  @objc
  func updateWithError(_ error: NSError?) {
    if let error = error as? SynchronizationError {
      switch error {
      case .fileUnavailable, .fileNotUploadedDueToQuota, .ubiquityServerNotAvailable:
        accessoryView = switchButton
      case .iCloudIsNotAvailable, .containerNotFound:
        accessoryView = nil
        accessoryType = .detailButton
      default:
        break
      }
      detailTextLabel?.text = error.localizedDescription
    } else {
      accessoryView = switchButton
      detailTextLabel?.text?.removeAll()
    }
    setNeedsLayout()
  }
}
