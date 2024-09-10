import Combine
import Foundation
import UIKit

protocol ProfileService {
  func getPersonalData() -> AnyPublisher<PersonalDataDTO, ResourceError>
  
  func updateProfile(
    fullName: String,
    country: String,
    email: String?,
    pfpUrl: UIImage?
  ) -> AnyPublisher<PersonalDataDTO, ResourceError>
  
  func updateLanguage(code: String)
  
  func updateTheme(code: String)
}

class ProfileServiceImpl: ProfileService {
  let userPreferences: UserPreferences
  
  init(userPreferences: UserPreferences) {
    self.userPreferences = userPreferences
  }
  
  func getPersonalData() -> AnyPublisher<PersonalDataDTO, ResourceError> {
    return CombineNetworkHelper.get(path: APIEndpoints.getUserUrl)
  }
  
  func updateProfile(
    fullName: String,
    country: String,
    email: String?,
    pfpUrl: UIImage?
  ) -> AnyPublisher<PersonalDataDTO, ResourceError> {
    var parameters = [
      [
        "key": "full_name",
        "value": fullName,
        "type": "text"
      ],
      [
        "key": "country",
        "value": country,
        "type": "text"
      ],
      [
        "key": "_method",
        "value": "PUT",
        "type": "text"
      ]] as [[String: Any]]
    
    if let newEmail = email {
      parameters.append([
        "key": "email",
        "value": newEmail,
        "type": "text"])
    }
    
    let theme = userPreferences.getTheme()
    parameters.append([
      "key": "theme",
      "value": theme?.code ?? "light",
      "type": "text"])
    
    let language = userPreferences.getLanguage()
    parameters.append([
      "key": "language",
      "value": language?.code ?? "ru",
      "type": "text"])
    
    let boundary = "Boundary-\(UUID().uuidString)"
    var body = Data()
    
    for param in parameters {
      let paramName = param["key"] as! String
      body += Data("--\(boundary)\r\n".utf8)
      body += Data("Content-Disposition: form-data; name=\"\(paramName)\"\r\n\r\n".utf8)
      body += Data("\(param["value"] as! String)\r\n".utf8)
    }
    
    // Add image file data if it exists
    if let image = pfpUrl, let imageData = image.jpegData(compressionQuality: 0.01) {
      body += Data("--\(boundary)\r\n".utf8)
      body += Data("Content-Disposition: form-data; name=\"avatar\"; filename=\"avatar.jpg\"\r\n".utf8)
      body += Data("Content-Type: image/jpeg\r\n\r\n".utf8)
      body += imageData
      body += Data("\r\n".utf8)
    }
    
    body += Data("--\(boundary)--\r\n".utf8)
    
    var request = URLRequest(url: URL(string: "https://product.rebus.tj/api/profile")!, timeoutInterval: Double.infinity)
    request.addValue("multipart/form-data; boundary=\(boundary)", forHTTPHeaderField: "Content-Type")
    
    if let token = userPreferences.getToken() {
      request.addValue("Bearer \(token)", forHTTPHeaderField: "Authorization")
    }
    
    request.httpMethod = "POST"
    request.httpBody = body
    
    return URLSession.shared.dataTaskPublisher(for: request)
      .tryMap { data, response in
        try AppNetworkHelper.handleResponse(data: data, response: response)
      }
      .mapError { error in
        AppNetworkHelper.handleMappingError(error)
      }
      .receive(on: DispatchQueue.main)
      .eraseToAnyPublisher()
  }
  
  func updateLanguage(code: String) {
    Task {
      await AppNetworkHelper.put(
        path: APIEndpoints.updateLanguageUrl,
        body: LanguageDTO(language: code)
      ) as Result<SimpleResponse, ResourceError>
    }
  }
  
  func updateTheme(code: String) {
    Task {
      await AppNetworkHelper.put(
        path: APIEndpoints.updateThemeUrl,
        body: ThemeDTO(theme: code)
      ) as Result<SimpleResponse, ResourceError>
    }
  }
}
