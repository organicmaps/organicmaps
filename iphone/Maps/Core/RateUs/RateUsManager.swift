import StoreKit

final class RateUsManager {

  private enum Constants {
    static let requestDisplayingDelay = 1.5
    static let maxRequestsCountPerVersion = 3
  }

  enum Traget {
    case appStore
    case app
  }

  static let shared = RateUsManager()

  func requestReview(in target: Traget) {
    switch target {
    case .appStore:
      self.showReviewInAppstore()
    case .app:
      self.showSKStoreReviewRequest()
    }
  }

  private func showReviewInAppstore() {
    UIApplication.shared.rateApp()
  }

  private func showSKStoreReviewRequest() {
    guard !CarPlayService.shared.isCarplayActivated,
          FrameworkHelper.canShowRateUsRequest() else { return }
    DispatchQueue.main.asyncAfter(deadline: .now() + Constants.requestDisplayingDelay) {
      #if DEBUG
      SKStoreReviewController.requestReviewInCurrentScene()
      #else
      let alert = UIAlertController(title: "Enjoying Organic Maps?",
                                    message: "Tap a star to rate in on the App Store.\n ⭐️ ⭐️ ⭐️ ⭐️ ⭐️",
                                    preferredStyle: .alert)
      let notNowAction = UIAlertAction(title: "Not Now", style: .default, handler: {
        _ in
        alert.dismiss(animated: true)
      })
      alert.addAction(notNowAction)
      UIApplication.shared.keyWindow?.rootViewController?.present(alert, animated: true)
      #endif
      FrameworkHelper.didShowRateUsRequest()
    }
  }
}

private extension SKStoreReviewController {
  static func requestReviewInCurrentScene() {
    if #available(iOS 14.0, *), let scene = UIApplication.shared.connectedScenes.first(where: { $0.activationState == .foregroundActive }) as? UIWindowScene {
      requestReview(in: scene)
      return
    }
    requestReview()
  }
}
