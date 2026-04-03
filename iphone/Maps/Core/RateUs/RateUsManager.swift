import StoreKit

@objcMembers
final class RateUsManager: NSObject {
  static let shared = RateUsManager()

  func showAppStoreReviewRequest() {
    UIApplication.shared.rateApp()
  }

  func showSKStoreReviewRequest() {
    guard MapViewController.shared()!.isMapFullyVisible(),
          !CarPlayService.shared.isCarplayActivated,
          FrameworkHelper.canShowRateUsRequest() else { return }
    SKStoreReviewController.requestReviewInCurrentScene()
    FrameworkHelper.didShowRateUsRequest()
  }
}

private extension SKStoreReviewController {
  static func requestReviewInCurrentScene() {
    if let scene = UIApplication.shared.connectedScenes.first(where: { $0.activationState == .foregroundActive }) as? UIWindowScene {
      requestReview(in: scene)
      return
    }
    requestReview()
  }
}
