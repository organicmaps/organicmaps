import Combine

protocol ReviewsService {
  func getReviewsByPlaceId(id: Int64) async throws -> ReviewsDTO
  func postReview(review: ReviewToPostDTO) async throws -> SimpleResponse
  func deleteReview(reviews: ReviewIdsDTO) async throws -> SimpleResponse
}

class ReviewsServiceImpl : ReviewsService {
  let userPreferences: UserPreferences
  
  init(userPreferences: UserPreferences) {
    self.userPreferences = userPreferences
  }
  
  func getReviewsByPlaceId(id: Int64) async throws -> ReviewsDTO {
    return try await AppNetworkHelper.get(path: APIEndpoints.getReviewsByPlaceIdUrl(id: id))
  }
  
  func postReview(review: ReviewToPostDTO) async throws -> SimpleResponse {
    guard let url = URL(string: APIEndpoints.postReviewUrl) else {
      throw ResourceError.other(message: "Invalid URL")
    }
    
    let boundary = "Boundary-\(UUID().uuidString)"
    var request = URLRequest(url: url)
    request.httpMethod = "POST"
    request.setValue("application/json", forHTTPHeaderField: "Accept")
    if let token = userPreferences.getToken() {
      request.setValue("Bearer \(token)", forHTTPHeaderField: "Authorization")
    }
    request.setValue("multipart/form-data; boundary=\(boundary)", forHTTPHeaderField: "Content-Type")
    
    let parameters: [[String: Any]] = [
      ["key": "message", "value": review.comment, "type": "text"],
      ["key": "mark_id", "value": "\(review.placeId)", "type": "text"],
      ["key": "points", "value": "\(review.rating)", "type": "text"]
    ] + review.images.map { ["key": "images[]", "src": $0.path, "type": "file"] }
    
    let body = try createBody(with: parameters, boundary: boundary)
    request.httpBody = body
    
    let (data, response) = try await URLSession.shared.data(for: request)
    
    if let httpResponse = response as? HTTPURLResponse {
      if !(200...299).contains(httpResponse.statusCode) {
        throw ResourceError.other(message: "Response not successful")
      }
    }
    
    let decoder = JSONDecoder()
    return try decoder.decode(SimpleResponse.self, from: data)
  }
  
  private func createBody(with parameters: [[String: Any]], boundary: String) throws -> Data {
    var body = Data()
    
    for param in parameters {
      if param["disabled"] != nil { continue }
      
      let paramName = param["key"] as! String
      body.append("--\(boundary)\r\n".data(using: .utf8)!)
      body.append("Content-Disposition:form-data; name=\"\(paramName)\"".data(using: .utf8)!)
      
      if let contentType = param["contentType"] as? String {
        body.append("\r\nContent-Type: \(contentType)".data(using: .utf8)!)
      }
      
      let paramType = param["type"] as! String
      if paramType == "text" {
        let paramValue = param["value"] as! String
        body.append("\r\n\r\n\(paramValue)\r\n".data(using: .utf8)!)
      } else {
        let paramSrc = param["src"] as! String
        let fileURL = URL(fileURLWithPath: paramSrc)
        let fileName = fileURL.lastPathComponent
        let data = try Data(contentsOf: fileURL)
        body.append("; filename=\"\(fileName)\"\r\n".data(using: .utf8)!)
        body.append("Content-Type: \"content-type header\"\r\n\r\n".data(using: .utf8)!)
        body.append(data)
        body.append("\r\n".data(using: .utf8)!)
      }
    }
    
    body.append("--\(boundary)--\r\n".data(using: .utf8)!)
    return body
  }
  
  func deleteReview(reviews: ReviewIdsDTO) async throws -> SimpleResponse {
    return try await AppNetworkHelper.delete(path: APIEndpoints.deleteReviewsUrl, body: reviews)
  }
}
