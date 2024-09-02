import Foundation
import Combine

class ThemeViewModel: ObservableObject {
//  private let profileRepository: ProfileRepository
  private let userPreferences: UserPreferences
  
  @Published var theme: UserPreferences.Theme?

  private var cancellables = Set<AnyCancellable>()

  init(
//    profileRepository: ProfileRepository,
    userPreferences: UserPreferences) {
//    self.profileRepository = profileRepository
    self.userPreferences = userPreferences

    self.theme = userPreferences.getTheme()
  }

  func setTheme(themeCode: String) {
    if let newTheme = userPreferences.themes.first(where: { $0.code == themeCode }) {
      self.theme = newTheme
      userPreferences.setTheme(value: themeCode)
    }
  }

  func updateThemeOnServer(themeCode: String) {
//    profileRepository.updateTheme(themeCode)
//      .sink { completion in
//        if case let .failure(error) = completion {
//          // Handle error if needed
//        }
//      } receiveValue: { response in
//        // Handle success if needed
//      }
//      .store(in: &cancellables)
  }
}

