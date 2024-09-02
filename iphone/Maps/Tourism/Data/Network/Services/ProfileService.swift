import Combine
import Foundation
import UIKit

protocol ProfileService {
  func getPersonalData() -> AnyPublisher<PersonalDataDTO, ResourceError>
  
  func updateProfile(
    fullName: String,
    country: String,
    email: String,
    pfpUrl: UIImage?
  ) -> AnyPublisher<PersonalDataDTO, ResourceError>
  
  func updateLanguage(code: String)
  
  func updateTheme(code: String)
}

class ProfileServiceImpl: ProfileService {
  func getPersonalData() -> AnyPublisher<PersonalDataDTO, ResourceError> {
    return CombineNetworkHelper.get(path: APIEndpoints.getUserUrl)
  }
  
  func updateProfile(
    fullName: String,
    country: String,
    email: String,
    pfpUrl: UIImage?
  ) -> AnyPublisher<PersonalDataDTO, ResourceError> {
    let body = createMultipartFormData(fullName: fullName, country: country, email: email, pfpUrl: pfpUrl)
    
    let boundary = UUID().uuidString
    let headers = ["Content-Type": "multipart/form-data; boundary=\(boundary)"]
    
    return CombineNetworkHelper.post(path: APIEndpoints.updateProfileUrl, body: body, headers: headers)
  }
  
  func updateLanguage(code: String) {
    // TODO: cmon
  }
  
  func updateTheme(code: String) {
    // TODO: cmon
  }
}


func createMultipartFormData(fullName: String, country: String, email: String?, pfpUrl: UIImage?) -> Data {
  let boundary = UUID().uuidString
  var body = Data()
  
  let boundaryPrefix = "--\(boundary)\r\n"
  
  body.appendString(boundaryPrefix)
  body.appendString("Content-Disposition: form-data; name=\"fullName\"\r\n\r\n")
  body.appendString("\(fullName)\r\n")
  
  body.appendString(boundaryPrefix)
  body.appendString("Content-Disposition: form-data; name=\"country\"\r\n\r\n")
  body.appendString("\(country)\r\n")
  
  if let email = email {
    body.appendString(boundaryPrefix)
    body.appendString("Content-Disposition: form-data; name=\"email\"\r\n\r\n")
    body.appendString("\(email)\r\n")
  }
  
  if let image = pfpUrl, let imageData = image.jpegData(compressionQuality: 0.5) {
    body.appendString(boundaryPrefix)
    body.appendString("Content-Disposition: form-data; name=\"pfpUrl\"; filename=\"profile.jpg\"\r\n")
    body.appendString("Content-Type: image/jpeg\r\n\r\n")
    body.append(imageData)
    body.appendString("\r\n")
  }
  
  body.appendString("--\(boundary)--\r\n")
  return body
}

extension Data {
  mutating func appendString(_ string: String) {
    if let data = string.data(using: .utf8) {
      append(data)
    }
  }
}
