import Foundation

struct APIEndpoints {
  // MARK: - Auth
  static let signInUrl = "\(BASE_URL)login"
  static let signUpUrl = "\(BASE_URL)register"
  static let signOutUrl = "\(BASE_URL)logout"
  static let forgotPassword = "\(BASE_URL)forgot-password"
  
  // MARK: - Profile
  static let getUserUrl = "\(BASE_URL)user"
  static let updateProfileUrl = "\(BASE_URL)profile"
  static let updateLanguageUrl = "\(BASE_URL)profile/lang"
  static let updateThemeUrl = "\(BASE_URL)profile/theme"
  
  // MARK: - Places
  static func getPlacesByCategoryUrl(id: Int64, hash: String) -> String {
    return "\(BASE_URL)marks/\(id)?hash=\(hash)"
  }
  static let getAllPlacesUrl = "\(BASE_URL)marks/all"
  
  // MARK: - Favorites
  static let getFavoritesUrl = "\(BASE_URL)favourite-marks"
  static let addFavoritesUrl = "\(BASE_URL)favourite-marks"
  static let removeFromFavoritesUrl = "\(BASE_URL)favourite-marks"
  
  // MARK: - Reviews
  static func getReviewsByPlaceIdUrl(id: Int64) -> String {
    return "\(BASE_URL)feedbacks/\(id)"
  }
  static let postReviewUrl = "\(BASE_URL)feedbacks"
  static let deleteReviewsUrl = "\(BASE_URL)feedbacks"
  
  // MARK: - Currency
  static let currencyUrl = "\(BASE_URL)currency"
}
