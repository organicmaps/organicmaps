import StoreKit

final class RateUsManager {

  private enum Constants {
    static let requestDisplayingDelay = 1.5
    static let maxRequestsCountPerVersion = 3
  }

  enum Sender {
    case about
    case addBookmark
  }

  static let shared = RateUsManager()

  func requestReview(from sender: Sender) {
    switch sender {
    case .about:
      self.showReviewInAppstore()
    case .addBookmark:
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
      SKStoreReviewController.requestReviewInCurrentScene()
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
