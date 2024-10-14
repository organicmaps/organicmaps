import Combine
import Foundation

protocol AuthService {
  func signIn(body: SignInRequestDTO) -> AnyPublisher<AuthResponseDTO, ResourceError>
  func signUp(body: SignUpRequestDTO) -> AnyPublisher<AuthResponseDTO, ResourceError>
  func signOut() -> AnyPublisher<SimpleResponse, ResourceError>
  func sendEmailForPasswordReset(email: String) -> AnyPublisher<SimpleResponse, ResourceError>
}

class AuthServiceImpl: AuthService {
  
  func signIn(body: SignInRequestDTO) -> AnyPublisher<AuthResponseDTO, ResourceError> {
    return CombineNetworkHelper.post(path: APIEndpoints.signInUrl, body: body)
  }
  
  func signUp(body: SignUpRequestDTO) -> AnyPublisher<AuthResponseDTO, ResourceError> {
    return CombineNetworkHelper.post(path: APIEndpoints.signUpUrl, body: body)
  }
  
  func signOut() -> AnyPublisher<SimpleResponse, ResourceError> {
    return CombineNetworkHelper.postWithoutBody(path: APIEndpoints.signOutUrl)
  }
  
  func sendEmailForPasswordReset(email: String) -> AnyPublisher<SimpleResponse, ResourceError> {
    return CombineNetworkHelper.post(path: APIEndpoints.forgotPassword, body: EmailBodyDto(email: email))
  }
}
