import Foundation

extension URL {
  func queryParams() -> [String : String]? {
    guard let urlComponents = URLComponents(url: self, resolvingAgainstBaseURL: false) else { return nil }
    return urlComponents.queryItems?.reduce(into: [:], { $0[$1.name] = $1.value })
  }
}
