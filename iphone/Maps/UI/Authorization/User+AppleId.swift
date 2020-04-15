import Foundation

fileprivate enum Const {
  static let kAppleIdKey = "kAppleIdKey"
}

@available(iOS 13.0, *)
extension User {
  static func setAppleId(_ appleId: String) {
    UserDefaults.standard.set(appleId, forKey: Const.kAppleIdKey)
  }

  @objc static func verifyAppleId() {
    guard let appleId = UserDefaults.standard.string(forKey: Const.kAppleIdKey) else { return }
    let appleIDProvider = ASAuthorizationAppleIDProvider()
    appleIDProvider.getCredentialState(forUserID: appleId) { (state, error) in
      switch state {
      case .revoked, .notFound:
        logOut()
        UserDefaults.standard.set(nil, forKey: Const.kAppleIdKey)
      default:
        break
      }
    }
  }
}
