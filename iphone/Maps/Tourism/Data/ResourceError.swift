import Foundation

enum ResourceError: Error, Equatable {
  case serverError(message: String)
  case cacheError
  case unauthed
  case other(message: String)
  case errorToUser(message: String)
  
  var errorDescription: String {
    switch self {
    case .serverError:
      return L("server_error")
    case .cacheError:
      return L("cache_error")
    case .unauthed:
      return L("unauthed_error")
    case .other:
      return L("smth_went_wrong")
    case .errorToUser(let message):
      return message
    }
  }
}
