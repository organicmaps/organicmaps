import Foundation

@objc(TourismUserPreferences)
class UserPreferences: NSObject {
  @objc static let shared = UserPreferences()
  
  private override init() {}
  
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
  
  @objc func getToken() -> String? {
    return userDefaults.string(forKey: "token")
  }
  
  @objc func setToken(value: String?) {
    userDefaults.set(value, forKey: "token")
  }
  
  func getUserId() -> String? {
    return userDefaults.string(forKey: "user_id")
  }
  
  func setUserId(value: String?) {
    userDefaults.set(value, forKey: "user_id")
  }
  
  @objc func getLocation() -> PlaceLocation? {
    let name = userDefaults.string(forKey: "name")
    let lat = userDefaults.double(forKey: "lat")
    let lon = userDefaults.double(forKey: "lon")
    return PlaceLocation(name: name ?? "", lat: lat, lon: lon)
  }
  
  @objc func setLocation(value: PlaceLocation) {
    userDefaults.set(value.name, forKey: "name")
    userDefaults.set(value.lat, forKey: "lat")
    userDefaults.set(value.lon, forKey: "lon")
  }
  
  @objc func clearLocation() {
    userDefaults.removeObject(forKey: "name")
    userDefaults.removeObject(forKey: "lat")
    userDefaults.removeObject(forKey: "lon")
  }
  
  @objc func isLocationEmpty() -> Bool {
    let location = getLocation()
    if let location {
      return location.lat == 0.0 && location.lon == 0.0
    }
    return true
  }
  
  @objc func getShouldGoToTourismMain() -> Bool {
      userDefaults.bool(forKey: "should_go_to_tourism_main")
  }
  
  @objc func setShouldGoToTourismMain(value: Bool) {
      userDefaults.set(value, forKey: "should_go_to_tourism_main")
  }
  
  @objc func getShouldGoToAuth() -> Bool {
      userDefaults.bool(forKey: "should_go_to_auth")
  }
  
  @objc func setShouldGoToAuth(value: Bool) {
      userDefaults.set(value, forKey: "should_go_to_auth")
  }
}
