import Foundation

class KeychainStorage {
  static var shared = {
    KeychainStorage()
  }()

  private init() { }

  func set(_ value: String, forKey key: String) {
    var query = self.query(forKey: key)

    if let _ = string(forKey: key) {
      var attributesToUpdate = [String: AnyObject]()
      attributesToUpdate[kSecValueData as String] = value.data(using: .utf8) as AnyObject
      SecItemUpdate(query as CFDictionary, attributesToUpdate as CFDictionary)
      return
    }

    query[kSecValueData as String] = value.data(using: .utf8) as AnyObject
    SecItemAdd(query as CFDictionary, nil)
  }

  func string(forKey key: String) -> String? {
    var query = self.query(forKey: key)
    query[kSecMatchLimit as String] = kSecMatchLimitOne
    query[kSecReturnAttributes as String] = kCFBooleanTrue
    query[kSecReturnData as String] = kCFBooleanTrue

    var keychainData: AnyObject?
    let status = withUnsafeMutablePointer(to: &keychainData) {
      SecItemCopyMatching(query as CFDictionary, UnsafeMutablePointer($0))
    }

    guard status == noErr,
      let keychainItem = keychainData as? [String: AnyObject],
      let stringData = keychainItem[kSecValueData as String] as? Data,
      let string = String(data: stringData, encoding: String.Encoding.utf8) else {
        return nil
    }

    return string
  }

  func deleteString(forKey key: String) {
    let query = self.query(forKey: key)
    SecItemDelete(query as CFDictionary)
  }

  private func query(forKey key: String) -> [String: AnyObject] {
    var query = [String: AnyObject]()
    query[kSecClass as String] = kSecClassGenericPassword
    query[kSecAttrService as String] = "com.mapswithme.full" as AnyObject
    query[kSecAttrAccount as String] = key as AnyObject
    return query
  }
}
