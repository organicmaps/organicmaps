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
    guard let scene = UIApplication.shared.foregroundActiveScene else {
      LOG(.error, "Attempt to display SKStoreReviewController in a non-active scene.")
      return
    }
    requestReview(in: scene)
  }
}
