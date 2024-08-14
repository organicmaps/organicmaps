import Combine

class AuthRepositoryImpl: AuthRepository {
  private let authService: AuthService
  
  init(authService: AuthService) {
    self.authService = authService
  }
  
  func signIn(body: SignInRequest) -> AnyPublisher<AuthResponse, NetworkError> {
    return authService.signIn(body: body).map { dto in
      AuthResponse.init(from: dto)
    }
    .eraseToAnyPublisher()
  }
  
  func signUp(body: SignUpRequest) -> AnyPublisher<AuthResponse, NetworkError> {
    return authService.signUp(body: body).map { dto in AuthResponse.init(from: dto) }
    .eraseToAnyPublisher()
  }
  
  func signOut() -> AnyPublisher<AuthResponse, NetworkError> {
    return authService.signOut().map { dto in AuthResponse.init(from: dto) }
    .eraseToAnyPublisher()
  }
}
