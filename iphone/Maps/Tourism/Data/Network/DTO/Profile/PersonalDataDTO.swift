import Foundation

struct PersonalDataDTO: Codable {
  let data: DataDTO
  
  struct DataDTO: Codable {
    let id: Int64
    let fullName: String
    let country: String
    let avatar: String?
    let email: String
    let language: String?
    let theme: String?
  }
  
  func toPersonalData() -> PersonalData {
    return PersonalData(
      id: data.id,
      fullName: data.fullName,
      country: data.country,
      pfpUrl: data.avatar,
      email: data.email,
      language: data.language,
      theme: data.theme
    )
  }
}
