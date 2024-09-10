import Foundation
import SwiftUI
import Combine

class PostReviewViewModel: ObservableObject {
  @Published var rating: Double = 5
  @Published var comment: String = ""
  @Published var files: [UIImage] = []
  @Published var isPosting: Bool = false
  
  private var cancellables = Set<AnyCancellable>()
  //    private let reviewsRepository: ReviewsRepository
  
  let uiEvents = PassthroughSubject<UiEvent, Never>()
  
  //    init(reviewsRepository: ReviewsRepository) {
  //        self.reviewsRepository = reviewsRepository
  //    }
  
  func setRating(_ value: Double) {
    rating = value
  }
  
  func addPickedImage() {
    //        guard let pickedImage = pickedImage else { return }
    //        Task {
    //            if let data = try? await pickedImage.loadTransferable(type: Data.self), let image = UIImage(data: data) {
    //                files.append(image)
    //            }
    //        }
  }
  
  func removeFile(_ file: UIImage) {
    files.removeAll { $0 == file }
  }
  
  func postReview(placeId: Int64) {
    //          isPosting = true
    //
    //          let review = ReviewToPost(placeId: placeId, comment: comment, rating: rating, images: files)
    //          reviewsRepository.postReview(review)
    //              .receive(on: DispatchQueue.main)
    //              .sink { completion in
    //                  self.isPosting = false
    //                  switch completion {
    //                  case .finished:
    //                      self.uiEvents.send(.showToast(message: "Review Posted"))
    //                      self.uiEvents.send(.closeReviewBottomSheet)
    //                  case .failure(let error):
    //                      self.uiEvents.send(.showToast(message: error.localizedDescription))
    //                  }
    //              } receiveValue: { response in
    //                  print("Review posted successfully")
    //              }
    //              .store(in: &cancellables)
  }
}

enum UiEvent {
  case closeReviewBottomSheet
  case showToast(message: String)
}
