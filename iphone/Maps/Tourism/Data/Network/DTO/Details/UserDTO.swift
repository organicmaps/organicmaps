import Foundation

struct UserDTO: Codable {
  let id: Int64
  let avatar: String
  let country: String
  let fullName: String
  
  func toUser() -> User {
    return User(
      id: id,
      name: fullName,
      pfpUrl: avatar,
      countryCodeName: country
    )
  }
}
