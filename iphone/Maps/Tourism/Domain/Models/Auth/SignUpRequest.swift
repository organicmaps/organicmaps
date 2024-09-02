struct SignUpRequest: Codable {
  let fullName: String
  let email: String
  let password: String
  let passwordConfirmation: String
  let country: String
}
