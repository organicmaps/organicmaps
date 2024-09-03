import Foundation
import Combine

class ProfileRepositoryImpl: ProfileRepository {
  private let profileService: ProfileService
  private let persistenceController: PersonalDataPersistenceController
  
  let personalDataPassThroughSubject = PassthroughSubject<PersonalData, ResourceError>()
  
  private var cancellables = Set<AnyCancellable>()
  
  init(personalDataService: ProfileService, personalDataPersistenceController: PersonalDataPersistenceController) {
    self.profileService = personalDataService
    self.persistenceController = personalDataPersistenceController
  }
  
  func getPersonalData() {
    // Local persistence subscription
    persistenceController.personalDataSubject
      .compactMap { $0?.toPersonalData() }
      .sink { completion in
        if case let .failure(error) = completion {
          print(error.localizedDescription)
          self.personalDataPassThroughSubject.send(completion: .failure(error))
        }
      } receiveValue: { personalData in
        self.personalDataPassThroughSubject.send(personalData)
      }
      .store(in: &cancellables) // Store the cancellable
    
    persistenceController.observePersonalData()
    
    // Remote service subscription
    profileService.getPersonalData()
      .flatMap { [weak self] remotePersonalData -> AnyPublisher<PersonalData, ResourceError> in
        guard let self = self else {
          print("ProfileRepositoryImpl/getPersonalData/ self was null")
          return Fail(error: ResourceError.other(message: "")).eraseToAnyPublisher()
        }
        
        let newPersonalData = remotePersonalData.toPersonalData()
        return self.persistenceController.updatePersonalData(personalData: newPersonalData)
          .map { newPersonalData }
          .eraseToAnyPublisher()
      }
      .sink { completion in
        if case let .failure(error) = completion {
          print(error.localizedDescription)
          self.personalDataPassThroughSubject.send(completion: .failure(error))
        }
      } receiveValue: { personalData in
        // Yes, nothing, we observe anyway
      }
      .store(in: &cancellables) // Store the cancellable
  }
  
  func updateProfile(
    fullName: String,
    country: String,
    email: String?,
    pfpUrl: UIImage?
  ) -> AnyPublisher<PersonalData, ResourceError> {
    return profileService.updateProfile(
      fullName: fullName,
      country: country,
      email: email,
      pfpUrl: pfpUrl
    )
    .flatMap{ dto in
      let personalData = dto.toPersonalData()
      
      return self.persistenceController.updatePersonalData(personalData: personalData)
        .map { personalData }
        .eraseToAnyPublisher()
    }
    .eraseToAnyPublisher()
  }
  
  func updateLanguage(code: String) {
    // TODO: cmon
  }
  
  func updateTheme(code: String) {
    // TODO: cmon
  }
}
