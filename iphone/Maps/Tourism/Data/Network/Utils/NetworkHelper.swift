import Foundation
import Combine

class CombineNetworkHelper {
  // MARK: - HTTP requests
  static func get<T: Decodable>(path: String, headers: [String: String] = [:], decoder: JSONDecoder = JSONDecoder()) -> AnyPublisher<T, ResourceError> {
    guard let url = URL(string: path) else {
      print("Invalid url")
      return Fail(error: ResourceError.other(message: "Invalid url")).eraseToAnyPublisher()
    }
    
    return performRequest(url: url, method: "GET", headers: headers, decoder: decoder)
  }
  
  static func post<T: Decodable, U: Encodable>(path: String, body: U, headers: [String: String] = [:], decoder: JSONDecoder = JSONDecoder()) -> AnyPublisher<T, ResourceError> {
    guard let url = URL(string: path) else {
      print("Invalid url")
      return Fail(error: ResourceError.other(message: "Invalid url")).eraseToAnyPublisher()
    }
    
    do {
      let jsonData = try AppNetworkHelper.encodeRequestBody(body)
      return performRequest(url: url, method: "POST", body: jsonData, headers: headers, decoder: decoder)
    } catch {
      return Fail(error: ResourceError.other(message: "Encoding error: \(error)")).eraseToAnyPublisher()
    }
  }
  
  static func postWithoutBody<T: Decodable>(path: String, headers: [String: String] = [:], decoder: JSONDecoder = JSONDecoder()) -> AnyPublisher<T, ResourceError> {
    guard let url = URL(string: path) else {
      print("Invalid url")
      return Fail(error: ResourceError.other(message: "Invalid url")).eraseToAnyPublisher()
    }
    
    return performRequest(url: url, method: "POST", headers: headers, decoder: decoder)
  }
  
  // MARK: - Lower level code
  static func performRequest<T: Decodable>(url: URL,
                                           method: String,
                                           body: Data? = nil,
                                           headers: [String: String] = [:],
                                           decoder: JSONDecoder = JSONDecoder()) -> AnyPublisher<T, ResourceError> {
    let request = AppNetworkHelper.createRequest(url: url, method: method, headers: headers, body: body)
    
    return URLSession.shared.dataTaskPublisher(for: request)
      .tryMap { data, response in
        try AppNetworkHelper.handleResponse(data: data, response: response, decoder: decoder)
      }
      .mapError { error in
        AppNetworkHelper.handleMappingError(error)
      }
      .receive(on: DispatchQueue.main)
      .eraseToAnyPublisher()
  }
  
}

class AppNetworkHelper {
  // MARK: - HTTP requests
  static func get<T: Decodable>(
    path: String,
    headers: [String: String] = [:],
    decoder: JSONDecoder = JSONDecoder()
  ) async -> Result<T, ResourceError> {
    guard let url = URL(string: path) else {
      return .failure(.other(message: "Invalid URL"))
    }

    return await performRequest(
      url: url,
      method: "GET",
      headers: headers,
      decoder: decoder
    )
  }


  static func post<T: Decodable, U: Encodable>(
    path: String,
    body: U,
    headers: [String: String] = [:],
    decoder: JSONDecoder = JSONDecoder()
  ) async -> Result<T, ResourceError> {
    guard let url = URL(string: path) else {
      return .failure(.other(message: "Invalid URL"))
    }
    
    do {
      let jsonData = try AppNetworkHelper.encodeRequestBody(body)
      return await performRequest(
        url: url,
        method: "POST",
        body: jsonData,
        headers: headers,
        decoder: decoder
      )
    } catch {
      return .failure(ResourceError.other(message: "Encoding error"))
    }
  }
  
  static func postWithoutBody<T: Decodable>(
    path: String,
    headers: [String: String] = [:],
    decoder: JSONDecoder = JSONDecoder()
  ) async -> Result<T, ResourceError> {
    guard let url = URL(string: path) else {
      return .failure(.other(message: "Invalid URL"))
    }

    return await performRequest(
      url: url,
      method: "POST",
      headers: headers,
      decoder: decoder
    )
  }

  static func put<T: Decodable, U: Encodable>(
    path: String,
    body: U,
    headers: [String: String] = [:],
    decoder: JSONDecoder = JSONDecoder()
  ) async -> Result<T, ResourceError> {
    guard let url = URL(string: path) else {
      return .failure(.other(message: "Invalid URL"))
    }
    
    do {
      let jsonData = try AppNetworkHelper.encodeRequestBody(body)
      return await performRequest(
        url: url,
        method: "PUT",
        body: jsonData,
        headers: headers,
        decoder: decoder
      )
    } catch {
      return .failure(ResourceError.other(message: "Encoding error"))
    }
  }
  
  static func performRequest<T: Decodable>(
    url: URL,
    method: String,
    body: Data? = nil,
    headers: [String: String] = [:],
    decoder: JSONDecoder
  ) async -> Result<T, ResourceError> {
    var request = createRequest(url: url, method: method, headers: headers, body: body)

    do {
      let (data, response) = try await URLSession.shared.data(for: request)

      // Handle response and decode data
      do {
        let decodedData: T = try handleResponse(data: data, response: response, decoder: decoder)
        return .success(decodedData)
      } catch {
        return .failure(.other(message: "Failed to handle response: \(error.localizedDescription)"))
      }
    } catch {
      return .failure(handleMappingError(error))
    }
  }

  // MARK: - Lower level code
  static func createRequest(url: URL, method: String, headers: [String: String] = [:], body: Data? = nil) -> URLRequest {
    var request = URLRequest(url: url)
    request.httpMethod = method
    request.addValue("application/json", forHTTPHeaderField: "Accept")
    request.setValue("application/json", forHTTPHeaderField: "Content-Type")
    if let token = UserPreferences.shared.getToken() {
      request.setValue("Bearer \(token)", forHTTPHeaderField: "Authorization")
    }
    
    headers.forEach { key, value in
      request.addValue(value, forHTTPHeaderField: key)
    }
    
    request.httpBody = body
    return request
  }
  
  static func handleResponse<T: Decodable>(data: Data, response: URLResponse, decoder: JSONDecoder = JSONDecoder()) throws -> T {
    guard let httpResponse = response as? HTTPURLResponse else {
      throw ResourceError.other(message: "Network request error")
    }
    
    print("Status Code: \(httpResponse.statusCode)")
    
    switch httpResponse.statusCode {
    case 200...299:
      return try decodeResponse(data: data, as: T.self)
    case 401:
      throw ResourceError.unauthed
    case 422:
      let decodedResponse = try decodeResponse(data: data, as: ErrorResponse.self)
      throw ResourceError.errorToUser(message: decodedResponse.message)
    case 500...599:
      throw ResourceError.serverError(message: "Server Error: \(httpResponse.statusCode)")
    default:
      throw ResourceError.other(message: "Unknown error")
    }
  }
  
  static func encodeRequestBody<T: Encodable>(_ body: T) throws -> Data {
    let encoder = JSONEncoder()
    encoder.outputFormatting = .withoutEscapingSlashes
    encoder.keyEncodingStrategy = .convertToSnakeCase
    return try encoder.encode(body)
  }
  
  static func decodeResponse<T: Decodable>(data: Data, as type: T.Type = T.self) throws -> T {
    let decoder = JSONDecoder()
    decoder.keyDecodingStrategy = .convertFromSnakeCase
    return try decoder.decode(type, from: data)
  }
  
  static func handleMappingError(_ error: Error) -> ResourceError {
    print("Mapping error: \(error)")
    return error as? ResourceError ?? ResourceError.other(message: "\(error)")
  }
  
}
