import Foundation
import Combine
import SwiftUI

class ProfileViewModel: ObservableObject {
  private let currencyRepository: CurrencyRepository
  private let profileRepository: ProfileRepository
  private let authRepository: AuthRepository
  private let userPreferences: UserPreferences
  var onSignOutCompleted: (() -> Void)? = nil
  
  @Published var messageToShowOnProfileScreen = ""
  @Published var shouldShowMessageOnProfileScreen = false
  
  @Published var messageToShowOnPersonalDataScreen = ""
  @Published var shouldShowMessageOnPersonalDataScreen = false
  
  @Published var pfpFromRemote: URL? = nil
  @Published var pfpToUpload = UIImage()
  @Published var isImagePickerUsed: Bool = false
  
  @Published var fullName: String = ""
  var currentEmail: String = "" // it changes only when confirmed by server
  @Published var email: String = ""
  @Published var countryCodeName: String? = nil
  @Published var personalData: PersonalData? = nil
  @Published var currencyRates: CurrencyRates? = nil
  
  private var cancellables = Set<AnyCancellable>()
  
  init(
    currencyRepository: CurrencyRepository,
    profileRepository: ProfileRepository,
    authRepository: AuthRepository,
    userPreferences: UserPreferences
  ) {
    self.currencyRepository = currencyRepository
    self.profileRepository = profileRepository
    self.authRepository = authRepository
    self.userPreferences = userPreferences
    // Automatically fetch data when initialized
    getPersonalData()
    getCurrency()
    $pfpToUpload.sink { image in
      if image != self.pfpToUpload {
        self.isImagePickerUsed = true
      }
    }.store(in: &cancellables)
  }
  
  // MARK: - Methods
  
  func setPfp(_ pfp: URL) {
    self.pfpFromRemote = pfp
  }
  
  func setFullName(_ value: String) {
    self.fullName = value
  }
  
  func setEmail(_ value: String) {
    self.email = value
  }
  
  func setCountryCodeName(_ value: String?) {
    self.countryCodeName = value
  }
  
  func getPersonalData() {
    profileRepository.personalDataPassThroughSubject
      .sink { completion in
        if case let .failure(error) = completion {
          if(error == ResourceError.unauthed) {
            self.onSignOutCompleted?()
          }
          self.showMessageOnProfileScreen(error.errorDescription)
        }
      } receiveValue: { resource in
        self.personalData = resource
        if let pfpUrl = resource.pfpUrl {
          self.pfpFromRemote = URL(string: pfpUrl)
        }
        self.fullName = resource.fullName
        self.currentEmail = resource.email
        self.email = resource.email
        self.countryCodeName = resource.country
      }
      .store(in: &cancellables)
    
    profileRepository.getPersonalData()
  }
  
  func save() {
    if(!fullName.isEmpty && ((countryCodeName?.isEmpty) != nil) && !email.isEmpty) {
      profileRepository.updateProfile(
        fullName: fullName,
        country: countryCodeName!,
        email: email,
        pfpUrl: pfpToUpload
      )
      .sink { completion in
        if case let .failure(error) = completion {
          self.showMessageOnPersonalDataScreen(error.errorDescription)
        }
      } receiveValue: { resource in
          self.updatePersonalDataInMemory(personalData: resource)
          self.showMessageOnPersonalDataScreen(L("saved"))
      }
      .store(in: &cancellables)
    } else {
      self.showMessageOnPersonalDataScreen(L("please_fill_all_fields"))
    }
  }
  
  private func updatePersonalDataInMemory(personalData: PersonalData) {
    self.fullName = personalData.fullName
    self.email = personalData.email
    self.countryCodeName = personalData.country
  }
  
  func getCurrency() {
    currencyRepository.currencyPassThroughSubject
      .sink { completion in
        if case let .failure(error) = completion {
          self.showMessageOnProfileScreen(error.errorDescription)
        }
      } receiveValue: { resource in
        self.currencyRates = resource
      }
      .store(in: &cancellables)
    
    currencyRepository.getCurrency()
  }
  
  // TODO: this doesn't work, try to find some other solutions
  // I tried to update language remotely after user set the new language
  func setLanguageOnSystemLocaleChange() {
    NotificationCenter.default.addObserver(self, selector: #selector(localeChanged), name: NSLocale.currentLocaleDidChangeNotification, object: nil)
  }
  
  @objc func localeChanged() {
    profileRepository.updateLanguage(code: NSLocale.current.identifier)
  }
  
  func signOut() {
    authRepository.signOut()
      .sink { completion in
        if case let .failure(error) = completion {
          self.showMessageOnProfileScreen(error.errorDescription)
        }
      } receiveValue: { response in
        self.userPreferences.setToken(value: nil)
        self.showMessageOnProfileScreen(response.message)
        self.onSignOutCompleted?()
      }
      .store(in: &cancellables)
  }
  
  func showMessageOnPersonalDataScreen(_ message: String) {
    messageToShowOnPersonalDataScreen = message
    shouldShowMessageOnPersonalDataScreen = true
  }
  
  func showMessageOnProfileScreen(_ message: String) {
    messageToShowOnProfileScreen = message
    shouldShowMessageOnProfileScreen = true
  }
}
