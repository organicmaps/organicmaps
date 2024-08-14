import Foundation

enum NetworkError: LocalizedError {
  case serverError(message: String)
  case other(message: String)
  case errorToUser(message: String)
  
  var errorDescription: String {
    switch self {
    case .serverError:
      return L("server_error")
    case .other:
      return L("smth_went_wrong")
    case .errorToUser(let message):
      return message
    }
  }
}
