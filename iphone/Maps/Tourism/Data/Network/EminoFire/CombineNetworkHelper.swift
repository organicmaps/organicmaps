import Foundation
import Combine

// EminoFire is kinda "library" for the abstraction of http code.
// It is named after the inventor of this piece Emin

class CombineNetworkHelper {
  
  static func createRequest(url: URL, method: String, headers: [String: String] = [:], body: Data? = nil) -> URLRequest {
    var request = URLRequest(url: url)
    request.httpMethod = method
    request.addValue("application/json", forHTTPHeaderField: "Accept")
    request.setValue("application/json", forHTTPHeaderField: "Content-Type")
    
    headers.forEach { key, value in
      request.addValue(value, forHTTPHeaderField: key)
    }
    
    request.httpBody = body
    return request
  }
  
  static func encodeRequestBody<T: Encodable>(_ body: T) throws -> Data {
    let encoder = JSONEncoder()
    encoder.outputFormatting = .withoutEscapingSlashes
    encoder.keyEncodingStrategy = .convertToSnakeCase
    return try encoder.encode(body)
  }
  
  static func decodeResponse<T: Decodable>(data: Data, as type: T.Type = T.self) throws -> T {
    let decoder = JSONDecoder()
    return try decoder.decode(type, from: data)
  }
  
  static func handleResponse<T: Decodable>(data: Data, response: URLResponse, decoder: JSONDecoder = JSONDecoder()) throws -> T {
    guard let httpResponse = response as? HTTPURLResponse else {
      throw NetworkError.other(message: "Network request error")
    }
    
    debugPrint("Status Code: \(httpResponse.statusCode)")
    
    switch httpResponse.statusCode {
    case 200...299:
      return try decodeResponse(data: data, as: T.self)
    case 422:
      let decodedResponse = try decodeResponse(data: data, as: ErrorResponse.self)
      throw NetworkError.errorToUser(message: decodedResponse.message)
    case 500...599:
      throw NetworkError.serverError(message: "Server Error: \(httpResponse.statusCode)")
    default:
      throw NetworkError.other(message: "Unknown error")
    }
  }
  
  static func handleMappingError(_ error: Error) -> NetworkError {
    debugPrint("Mapping error: \(error)")
    return error as? NetworkError ?? NetworkError.other(message: "\(error)")
  }
  
  static func performRequest<T: Decodable>(url: URL,
                                           method: String,
                                           body: Data? = nil,
                                           headers: [String: String] = [:],
                                           decoder: JSONDecoder = JSONDecoder()) -> AnyPublisher<T, NetworkError> {
    let request = createRequest(url: url, method: method, headers: headers, body: body)
    
    return URLSession.shared.dataTaskPublisher(for: request)
      .tryMap { data, response in
        try handleResponse(data: data, response: response, decoder: decoder)
      }
      .mapError { error in
        handleMappingError(error)
      }
      .receive(on: DispatchQueue.main)
      .eraseToAnyPublisher()
  }
  
  static func get<T: Decodable>(url: URL, headers: [String: String] = [:], decoder: JSONDecoder = JSONDecoder()) -> AnyPublisher<T, NetworkError> {
    return performRequest(url: url, method: "GET", headers: headers, decoder: decoder)
  }
  
  static func post<T: Decodable, U: Encodable>(path: String, body: U, headers: [String: String] = [:], decoder: JSONDecoder = JSONDecoder()) -> AnyPublisher<T, NetworkError> {
    guard let url = URL(string: path) else {
      debugPrint("Invalid url")
      return Fail(error: NetworkError.other(message: "Invalid url")).eraseToAnyPublisher()
    }
    
    do {
      let jsonData = try encodeRequestBody(body)
      return performRequest(url: url, method: "POST", body: jsonData, headers: headers, decoder: decoder)
    } catch {
      return Fail(error: NetworkError.other(message: "Encoding error: \(error)")).eraseToAnyPublisher()
    }
  }
  
  static func postWithoutBody<T: Decodable>(path: String, headers: [String: String] = [:], decoder: JSONDecoder = JSONDecoder()) -> AnyPublisher<T, NetworkError> {
    guard let url = URL(string: path) else {
      debugPrint("Invalid url")
      return Fail(error: NetworkError.other(message: "Invalid url")).eraseToAnyPublisher()
    }
    
    return performRequest(url: url, method: "POST", headers: headers, decoder: decoder)
  }
}
