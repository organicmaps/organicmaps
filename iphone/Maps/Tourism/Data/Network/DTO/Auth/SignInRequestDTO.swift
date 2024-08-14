struct SignInRequestDTO: Codable {
    let email: String
    let password: String
}

extension SignInRequestDTO {
    init(from domainModel: SignInRequest) {
        self.email = domainModel.email
        self.password = domainModel.password
    }
}
