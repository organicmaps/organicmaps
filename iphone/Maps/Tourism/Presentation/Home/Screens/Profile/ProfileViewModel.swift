import Foundation
import Combine
import SwiftUI

class ProfileViewModel: ObservableObject {
  private let currencyRepository: CurrencyRepository
  private let profileRepository: ProfileRepository
  private let authRepository: AuthRepository
  private let userPreferences: UserPreferences
  var onMessageToUserRequested: ((String) -> Void)? = nil
  var onSignOutCompleted: (() -> Void)? = nil
  
  @Published var pfpFile: URL? = nil
  @Published var fullName: String = ""
  @Published var email: String = ""
  @Published var countryCodeName: String? = nil
  @Published var personalData: PersonalData? = nil
  @Published var signOutResponse: SimpleResponse? = nil
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
  }
  
  // MARK: - Methods
  
  func setPfpFile(pfpFile: URL) {
    self.pfpFile = pfpFile
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
          self.onMessageToUserRequested?(error.errorDescription)
        }
      } receiveValue: { resource in
        self.personalData = resource
      }
      .store(in: &cancellables)
    
    profileRepository.getPersonalData()
  }
  
  //  func save() {
  //    guard case let .success(personalData) = personalDataResource else { return }
  //1
  //    profileRepository.updateProfile(
  //      fullName: fullName,
  //      country: countryCodeName ?? "",
  //      email: email != personalData.email ? email : nil,
  //      pfpUrl: pfpFile
  //    )
  //    .sink { completion in
  //      if case let .failure(error) = completion {
  //        self.showToast(message: error.localizedDescription)
  //      }
  //    } receiveValue: { resource in
  //      if case let .success(personalData) = resource {
  //        self.updatePersonalDataInMemory(personalData: personalData)
  //        self.showToast(message: "Saved")
  //      }
  //    }
  //    .store(in: &cancellables)
  //  }
  //
  //  private func updatePersonalDataInMemory(personalData: PersonalData) {
  //    self.fullName = personalData.fullName
  //    self.email = personalData.email
  //    self.countryCodeName = personalData.country
  //  }
  
  func getCurrency() {
    currencyRepository.currencyPassThroughSubject
      .sink { completion in
        if case let .failure(error) = completion {
          self.onMessageToUserRequested?(error.errorDescription)
        }
      } receiveValue: { resource in
        self.currencyRates = resource
      }
      .store(in: &cancellables)
    
    currencyRepository.getCurrency()
  }
  
  func signOut() {
    authRepository.signOut()
      .sink { completion in
        if case let .failure(error) = completion {
          self.onMessageToUserRequested?(error.errorDescription)
        }
      } receiveValue: { response in
        self.signOutResponse = response
        self.userPreferences.setToken(value: nil)
        self.onSignOutCompleted?()
        self.onMessageToUserRequested?(response.message)
      }
      .store(in: &cancellables)
  }
}
