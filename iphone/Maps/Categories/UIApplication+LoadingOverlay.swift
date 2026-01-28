extension UIApplication {
  private static let overlayViewController = LoadingOverlayViewController()

  @objc
  func showLoadingOverlay(completion: (() -> Void)? = nil) {
    guard let window = self.windows.first(where: { $0.isKeyWindow }) else {
      completion?()
      return
    }

    DispatchQueue.main.async {
      UIApplication.overlayViewController.modalPresentationStyle = .overFullScreen
      UIApplication.overlayViewController.modalTransitionStyle = .crossDissolve
      window.rootViewController?.present(UIApplication.overlayViewController, animated: true, completion: completion)
    }
  }

  @objc
  func hideLoadingOverlay(completion: (() -> Void)? = nil) {
    DispatchQueue.main.async {
      UIApplication.overlayViewController.dismiss(animated: true, completion: completion)
    }
  }
}
