@objc protocol ThemeListener {
  @objc func applyTheme()
}

@objc class StyleManager: NSObject {
  private struct Weak {
    weak var listener: ThemeListener?
  }
  private static var _instance: StyleManager?
  @objc private(set) var theme: Theme?
  private var listeners: [Weak] = []

  func setTheme (_ theme: Theme) {
    self.theme = theme;
    update()
  }

  func hasTheme () -> Bool {
    return theme != nil
  }

  @objc class func instance() -> StyleManager{
    if StyleManager._instance == nil {
      SwizzleStyle.addSwizzle();
      StyleManager._instance = StyleManager()
    }
    return StyleManager._instance!
  }

  func update (){
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
      if let listener = container.listener {
        listener.applyTheme()
      }
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
    if listeners.contains(where: { (container) -> Bool in
      return themeListener === container.listener
      }) == false {
      listeners.append(Weak(listener: themeListener))
    }
  }
  
  @objc func removeListener(_ themeListener: ThemeListener) {
    listeners.removeAll { (container) -> Bool in
      return container.listener === themeListener
    }
  }
}
