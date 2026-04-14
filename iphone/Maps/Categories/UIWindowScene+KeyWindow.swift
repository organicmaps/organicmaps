extension UIWindowScene {
  var keyWindow: UIWindow? {
    windows.first(where: \.isKeyWindow)
  }
}

extension UIApplication {
  var activeWindowScenes: [UIWindowScene] {
    connectedScenes
      .compactMap { $0 as? UIWindowScene }
      .filter { $0.activationState == .foregroundActive }
  }

  var activeKeyWindow: UIWindow? {
    foregroundActiveScene?.keyWindow
  }

  var activeWindows: [UIWindow] {
    activeWindowScenes.flatMap(\.windows)
  }

  var foregroundActiveScene: UIWindowScene? {
    activeWindowScenes.first(where: { $0.keyWindow != nil }) ?? activeWindowScenes.first
  }
}
