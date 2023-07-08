@objc protocol ThemeListener {
  @objc func applyTheme()
}

@objc class StyleManager: NSObject {
  @objc static var shared = StyleManager()
  @objc private(set) var theme: Theme?
  private var listeners: [Weak<ThemeListener>] = []

  override private init() {
    super.init()
    SwizzleStyle.swizzle()
  }

  func setTheme (_ theme: Theme) {
    self.theme = theme;
    update()
  }

  func hasTheme () -> Bool {
    return theme != nil
  }

  func update () {
    for window in UIApplication.shared.windows {
      updateView(window.rootViewController?.view)
    }
    
    let appDelegate = UIApplication.shared.delegate as! MapsAppDelegate
    if let vc = appDelegate.window.rootViewController?.presentedViewController {
      vc.applyTheme()
      updateView(vc.view)
    }  else if let vcs = appDelegate.window.rootViewController?.children {
      for vc in vcs {
        vc.applyTheme()
      }
    }
    
    for container in listeners {
      if let listener = container.value {
        listener.applyTheme()
      }
    }

    if #available(iOS 13, *) {} else {
      UISearchBarRenderer.setAppearance()
    }
  }

  private func updateView(_ view: UIView?) {
    guard let view = view else {
      return
    }
    view.isStyleApplied = false
    for subview in view.subviews {
      self.updateView(subview)
    }
    view.applyTheme()
    view.isStyleApplied = true;
  }

  func getStyle(_ styleName: String) -> [Style]{
    return theme?.get(styleName) ?? [Style]()
  }
  
  @objc func addListener(_ themeListener: ThemeListener) {
    if theme != nil {
      themeListener.applyTheme()
    }
    if !listeners.contains(where: { themeListener === $0.value }) {
      listeners.append(Weak(value: themeListener))
    }
  }
  
  @objc func removeListener(_ themeListener: ThemeListener) {
    listeners.removeAll { (container) -> Bool in
      return container.value === themeListener
    }
  }
}
