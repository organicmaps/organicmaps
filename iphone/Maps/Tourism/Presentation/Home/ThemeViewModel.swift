import Foundation
import Combine

class ThemeViewModel: ObservableObject {
  private let profileRepository: ProfileRepository
  private let userPreferences: UserPreferences
  
  @Published var theme: UserPreferences.Theme?
  
  init(
    profileRepository: ProfileRepository,
    userPreferences: UserPreferences) {
      self.profileRepository = profileRepository
      self.userPreferences = userPreferences
      
      self.theme = userPreferences.getTheme()
    }
  
  func setTheme(themeCode: String) {
    profileRepository.updateTheme(code: themeCode)
  }
  
  func updateThemeOnServer(themeCode: String) {
    profileRepository.updateTheme(code: themeCode)
  }
}

