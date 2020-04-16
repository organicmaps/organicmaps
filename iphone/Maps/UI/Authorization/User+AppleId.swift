import Foundation

fileprivate enum Const {
  static let kAppleIdKey = "kAppleIdKey"
  static let kAppleIdFirstName = "kAppleIdFirstName"
  static let kAppleIdLastName = "kAppleIdLastName"
}

struct AppleId {
  let userId: String
  let firstName: String
  let lastName: String
}

@available(iOS 13.0, *)
extension User {
  static func setAppleId(_ appleId: AppleId) {
    KeychainStorage.shared.set(appleId.userId, forKey: Const.kAppleIdKey)
    KeychainStorage.shared.set(appleId.firstName, forKey: Const.kAppleIdFirstName)
    KeychainStorage.shared.set(appleId.lastName, forKey: Const.kAppleIdLastName)
  }

  static func getAppleId() -> AppleId? {
    guard let userId = KeychainStorage.shared.string(forKey: Const.kAppleIdKey),
      let firstName = KeychainStorage.shared.string(forKey: Const.kAppleIdFirstName),
      let lastName = KeychainStorage.shared.string(forKey: Const.kAppleIdLastName) else {
        return nil
    }
    return AppleId(userId: userId, firstName: firstName, lastName: lastName)
  }

  @objc static func verifyAppleId() {
    guard let userId = KeychainStorage.shared.string(forKey: Const.kAppleIdKey) else { return }
    let appleIDProvider = ASAuthorizationAppleIDProvider()
    appleIDProvider.getCredentialState(forUserID: userId) { (state, error) in
      switch state {
      case .revoked, .notFound:
        logOut()
        KeychainStorage.shared.deleteString(forKey: Const.kAppleIdKey)
        KeychainStorage.shared.deleteString(forKey: Const.kAppleIdFirstName)
        KeychainStorage.shared.deleteString(forKey: Const.kAppleIdLastName)
      default:
        break
      }
    }
  }
}
