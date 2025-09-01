final class PlacePageExpandableDetailsSectionPresenter {

  private var viewModel: PlacePageExpandableDetailsSectionViewModel

  weak var view: PlacePageExpandableDetailsSectionViewController?

  init(viewModel: PlacePageExpandableDetailsSectionViewModel) {
    self.viewModel = viewModel
  }

  func process(_ responses: [PlacePageExpandableDetailsSectionResponse]) {
    var currentViewModel = viewModel
    for response in responses {
      currentViewModel = resolve(response, with: currentViewModel)
    }
    self.viewModel = currentViewModel
    view?.render(currentViewModel)
  }

  private func resolve(_ response: PlacePageExpandableDetailsSectionResponse, with previousViewModel: PlacePageExpandableDetailsSectionViewModel) -> PlacePageExpandableDetailsSectionViewModel {
    var viewModel = previousViewModel
    switch response {
    case .expandText:
      switch viewModel.expandedState {
      case .collapsed:
        viewModel.expandedState = .expanded
      case .expanded:
        viewModel.expandedState = .collapsed
      case .hidden:
        break
      }
    case .updateTitle(let string):
      viewModel.title = string
    case .updateIcon(let image):
      viewModel.icon = image
    case .updateAccessory(let image):
      viewModel.accessory = image
    case .updateExpandableText(let string, let isHTML):
      if isHTML, let string {
        Self.buildAttributedString(from: string) { [weak self] attributedString in
          guard let self else { return }
          self.viewModel.expandableAttributedText = attributedString
          self.viewModel.expandedState = attributedString.string.isEmpty ? .hidden : .collapsed
          self.view?.render(self.viewModel)
        }
      } else {
        viewModel.expandedState = (string ?? "").isEmpty ? .hidden : .collapsed
        viewModel.expandableText = string
      }
    }
    return viewModel
  }

  private static func buildAttributedString(from htmlString: String, completion: @escaping (NSAttributedString) -> Void) {
    DispatchQueue.global().async {
      let font = UIFont.regular14()
      let color = UIColor.blackPrimaryText()
      let paragraphStyle = NSMutableParagraphStyle()
      paragraphStyle.lineSpacing = 4

      let attributedString: NSAttributedString
      if let str = NSMutableAttributedString(htmlString: htmlString, baseFont: font, paragraphStyle: paragraphStyle) {
        str.addAttribute(NSAttributedString.Key.foregroundColor,
                         value: color,
                         range: NSRange(location: 0, length: str.length))
        attributedString = str;
      } else {
        attributedString = NSAttributedString(string: htmlString,
                                              attributes: [NSAttributedString.Key.font : font,
                                                           NSAttributedString.Key.foregroundColor: color,
                                                           NSAttributedString.Key.paragraphStyle: paragraphStyle])
      }

      DispatchQueue.main.async {
        completion(attributedString)
      }
    }
  }
}
