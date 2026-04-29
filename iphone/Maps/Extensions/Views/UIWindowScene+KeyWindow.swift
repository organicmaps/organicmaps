extension UIApplication {
  var allConnectedWindows: [UIWindow] {
    connectedScenes
      .compactMap { $0 as? UIWindowScene }
      .flatMap(\.windows)
  }

  var activeWindowScenes: [UIWindowScene] {
    connectedScenes
      .compactMap { $0 as? UIWindowScene }
      .filter { $0.activationState == .foregroundActive }
  }

  var activeKeyWindow: UIWindow? {
    foregroundActiveScene?.keyWindow
  }

  var foregroundActiveScene: UIWindowScene? {
    activeWindowScenes.first(where: { $0.keyWindow != nil }) ?? activeWindowScenes.first
  }
}
