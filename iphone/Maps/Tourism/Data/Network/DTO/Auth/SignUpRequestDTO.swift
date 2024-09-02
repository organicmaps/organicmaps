struct SignUpRequestDTO: Codable {
  let fullName: String
  let email: String
  let password: String
  let passwordConfirmation: String
  let country: String
}

extension SignUpRequestDTO {
  init(from domainModel: SignUpRequest) {
    self.fullName = domainModel.fullName
    self.email = domainModel.email
    self.password = domainModel.password
    self.passwordConfirmation = domainModel.passwordConfirmation
    self.country = domainModel.country
  }
}
