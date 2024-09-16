import Combine

protocol ReviewsService {
  func getReviewsByPlaceId(id: Int64) async throws -> ReviewsDTO
  func postReview(review: ReviewToPost) async throws -> ReviewDTO
  func deleteReview(feedbacks: ReviewIdsDTO) async throws -> SimpleResponse
}

class ReviewsServiceImpl : ReviewsService {
  func getReviewsByPlaceId(id: Int64) async throws -> ReviewsDTO {
    return try await AppNetworkHelper.get(path: APIEndpoints.getReviewsByPlaceIdUrl(id: id))
  }
  
  func postReview(review: ReviewToPost) async throws -> ReviewDTO {
    return try await AppNetworkHelper.post(path: APIEndpoints.postReviewUrl, body: review)
  }
  
  func deleteReview(feedbacks: ReviewIdsDTO) async throws -> SimpleResponse {
    return try await AppNetworkHelper.delete(path: APIEndpoints.deleteReviewsUrl, body: feedbacks)
  }
}
