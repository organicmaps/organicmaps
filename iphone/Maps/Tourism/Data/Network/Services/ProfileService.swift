import Combine

protocol ProfileService {
  func getPersonalData() -> AnyPublisher<PersonalDataDTO, ResourceError>
}

class PersonalDataServiceImpl: ProfileService {
  
  func getPersonalData() -> AnyPublisher<PersonalDataDTO, ResourceError> {
    return CombineNetworkHelper.get(path: APIEndpoints.getUserUrl)
  }
}
