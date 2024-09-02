import Foundation
import Combine

protocol AuthRepository {
  func signIn(body: SignInRequest) -> AnyPublisher<AuthResponse, ResourceError>
  func signUp(body: SignUpRequest) -> AnyPublisher<AuthResponse, ResourceError>
  func signOut() -> AnyPublisher<SimpleResponse, ResourceError>
}
