import Foundation
import Combine

protocol ProfileRepository {
  var personalDataPassThroughSubject: PassthroughSubject<PersonalData, ResourceError> { get }
  
  func getPersonalData()
  
  func updateProfile(
    fullName: String,
    country: String,
    email: String,
    pfpUrl: UIImage?
  ) -> AnyPublisher<PersonalData, ResourceError>
  
  func updateLanguage(code: String)
  
  func updateTheme(code: String)
}
