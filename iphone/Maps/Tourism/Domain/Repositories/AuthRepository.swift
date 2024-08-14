import Foundation
import Combine

protocol AuthRepository {
  func signIn(body: SignInRequest) -> AnyPublisher<AuthResponse, NetworkError>
  func signUp(body: SignUpRequest) -> AnyPublisher<AuthResponse, NetworkError>
  func signOut() -> AnyPublisher<AuthResponse, NetworkError>
}
