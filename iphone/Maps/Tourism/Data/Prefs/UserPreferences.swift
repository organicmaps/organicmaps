import Foundation

class UserPreferences {
  static let shared = UserPreferences()
  
  private init() {}
  
  private let userDefaults = UserDefaults.standard
  
  struct Language {
    let code: String
    let name: String
  }
  
  struct Theme {
    let code: String
    let name: String
  }
  
  var languages: [Language] = [
    Language(code: "ru", name: "Русский"),
    Language(code: "en", name: "English")
  ]
  
  var themes: [Theme] = [
    Theme(code: "dark", name: L("dark_theme")),
    Theme(code: "light", name: L("light_theme"))
  ]
  
  func getLanguage() -> Language? {
    guard let languageCode = userDefaults.string(forKey: "language") else { return nil }
    return languages.first { $0.code == languageCode }
  }
  
  func setLanguage(value: String) {
    userDefaults.set(value, forKey: "language")
  }
  
  func getTheme() -> Theme? {
    guard let themeCode = userDefaults.string(forKey: "theme") else { return nil }
    return themes.first { $0.code == themeCode }
  }
  
  func setTheme(value: String?) {
    userDefaults.set(value, forKey: "theme")
  }
  
  func getToken() -> String? {
    return userDefaults.string(forKey: "token")
  }
  
  func setToken(value: String?) {
    userDefaults.set(value, forKey: "token")
  }
  
  func getUserId() -> String? {
    return userDefaults.string(forKey: "user_id")
  }
  
  func setUserId(value: String?) {
    userDefaults.set(value, forKey: "user_id")
  }
}
