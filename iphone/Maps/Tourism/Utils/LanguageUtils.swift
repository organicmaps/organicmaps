func navigateToLanguageSettings() {
  guard let settingsURL = URL(string: UIApplication.openSettingsURLString) else {return}
  UIApplication.shared.open(settingsURL, options: [:], completionHandler: nil)
}
