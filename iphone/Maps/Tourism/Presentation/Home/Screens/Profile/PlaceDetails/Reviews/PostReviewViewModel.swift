import Foundation
import SwiftUI
import Combine

class PostReviewViewModel: ObservableObject {
  private var cancellables = Set<AnyCancellable>()
  
  private let reviewsRepository: ReviewsRepository
  
  @Published var rating: Double = 5
  @Published var comment: String = ""
  @Published var images: [UIImage] = []
  @Published var isPosting: Bool = false
  
  @Published var retrievedImages: [UIImage] = []
  
  let uiEvents = PassthroughSubject<UiEvent, Never>()
  
  init(reviewsRepository: ReviewsRepository) {
    self.reviewsRepository = reviewsRepository
  }
  
  func setRating(_ value: Double) {
    rating = value
  }
  
  func removeFile(_ file: UIImage) {
    images.removeAll { $0 == file }
  }
  
  func postReview(placeId: Int64) {
    isPosting = true
    
    let urls = saveMultipleImages(images, placeId: placeId)
    print(urls)
    let review = ReviewToPost(
      placeId: placeId,
      comment: comment,
      rating: Int(rating),
      images: urls
    )
    
    reviewsRepository.postReview(review: review)
      .receive(on: DispatchQueue.main)
      .sink { completion in
        self.isPosting = false
        switch completion {
        case .finished:
          self.uiEvents.send(.showToast(message: "Review Posted"))
          self.uiEvents.send(.closeReviewBottomSheet)
        case .failure(let error):
          self.uiEvents.send(.showToast(message: error.errorDescription))
        }
      } receiveValue: { response in
        print(response)
      }
      .store(in: &cancellables)
  }
}

enum UiEvent {
  case closeReviewBottomSheet
  case showToast(message: String)
}
