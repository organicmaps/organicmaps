import Combine

class AuthRepositoryImpl: AuthRepository {
  private let authService: AuthService
  
  init(authService: AuthService) {
    self.authService = authService
  }
  
  func signIn(body: SignInRequest) -> AnyPublisher<AuthResponse, ResourceError> {
    return authService.signIn(body: SignInRequestDTO.init(from: body))
      .map { dto in AuthResponse.init(from: dto) }
      .eraseToAnyPublisher()
  }
  
  func signUp(body: SignUpRequest) -> AnyPublisher<AuthResponse, ResourceError> {
    return authService.signUp(body: SignUpRequestDTO.init(from: body))
      .map { dto in AuthResponse.init(from: dto) }
      .eraseToAnyPublisher()
  }
  
  func signOut() -> AnyPublisher<SimpleResponse, ResourceError> {
    return authService.signOut()
      .eraseToAnyPublisher()
  }
}
