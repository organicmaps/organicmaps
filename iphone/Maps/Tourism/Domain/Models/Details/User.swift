import Foundation

struct User: Codable, Hashable {
  let id: Int64
  let name: String
  let pfpUrl: String?
  let countryCodeName: String
  
  func toUserEntity() -> UserEntity {
    return UserEntity(
      userId: self.id,
      fullName: self.name,
      avatar: self.pfpUrl ?? "",
      country: self.countryCodeName
    )
  }
}
