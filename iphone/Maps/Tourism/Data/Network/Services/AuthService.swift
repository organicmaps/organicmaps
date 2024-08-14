import Combine
import Foundation

protocol AuthService {
  func signIn(body: SignInRequest) -> AnyPublisher<AuthResponseDTO, NetworkError>
  func signUp(body: SignUpRequest) -> AnyPublisher<AuthResponseDTO, NetworkError>
  func signOut() -> AnyPublisher<AuthResponseDTO, NetworkError>
}

class AuthServiceImpl: AuthService {
  
  private let baseURL = BASE_URL
  
  func signIn(body: SignInRequest) -> AnyPublisher<AuthResponseDTO, NetworkError> {
    return CombineNetworkHelper.post(path: APIEndpoints.signInUrl, body: body)
  }
  
  func signUp(body: SignUpRequest) -> AnyPublisher<AuthResponseDTO, NetworkError> {
    return CombineNetworkHelper.post(path: APIEndpoints.signUpUrl, body: body)
  }
  
  func signOut() -> AnyPublisher<AuthResponseDTO, NetworkError> {
    return CombineNetworkHelper.postWithoutBody(path: APIEndpoints.signOutUrl)
  }
}
