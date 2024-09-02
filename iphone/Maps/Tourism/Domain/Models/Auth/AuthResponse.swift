struct AuthResponse: Codable {
  let token: String
}

extension AuthResponse {
  init(from dto: AuthResponseDTO) {
    self.token = dto.token
  }
}
