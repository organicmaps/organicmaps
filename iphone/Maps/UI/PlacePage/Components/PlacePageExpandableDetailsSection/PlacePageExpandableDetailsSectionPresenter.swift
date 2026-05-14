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
    viewModel = currentViewModel
    view?.render(currentViewModel)
  }

  private func resolve(_ response: PlacePageExpandableDetailsSectionResponse, with previousViewModel: PlacePageExpandableDetailsSectionViewModel) -> PlacePageExpandableDetailsSectionViewModel {
    var viewModel = previousViewModel
    switch response {
    case .expandText:
      if case .collapsed = viewModel.expandedState {
        viewModel.expandedState = .expanded
      }
    case .updateTitle(let string):
      viewModel.title = string
    case .updateIcon(let image):
      viewModel.icon = image
    case .updateAccessory(let image):
      viewModel.accessory = image
    case .updateExpandableText(let string, let isHTML):
      if isHTML, let string {
        // HTML is parsed off the main thread and applied in the completion below;
        // title, icon and accessory still render synchronously via process().
        Self.buildAttributedString(from: string) { [weak self] attributedString in
          self?.applyHtmlExpandableText(attributedString)
        }
      } else {
        viewModel.expandableText = string.map { .plain($0) }
        viewModel.expandedState = (string ?? "").isEmpty ? .hidden : .collapsed
      }
    }
    return viewModel
  }

  private func applyHtmlExpandableText(_ attributedString: NSAttributedString) {
    let isEmpty = attributedString.string.isEmpty
    // Hide the section on an empty parse instead of showing a blank label.
    viewModel.expandableText = isEmpty ? nil : .html(attributedString)
    viewModel.expandedState = isEmpty ? .hidden : .collapsed
    view?.render(viewModel)
  }

  private static func buildAttributedString(from htmlString: String, completion: @escaping (NSAttributedString) -> Void) {
    DispatchQueue.global().async {
      let font = UIFont.regular14
      let color = UIColor.blackPrimaryText
      let paragraphStyle = NSMutableParagraphStyle()
      paragraphStyle.lineSpacing = 4

      let attributedString: NSAttributedString
      if let str = NSMutableAttributedString(htmlString: htmlString, baseFont: font, paragraphStyle: paragraphStyle) {
        str.addAttribute(NSAttributedString.Key.foregroundColor,
                         value: color,
                         range: NSRange(location: 0, length: str.length))
        attributedString = str
      } else {
        attributedString = NSAttributedString(string: htmlString,
                                              attributes: [NSAttributedString.Key.font: font,
                                                           NSAttributedString.Key.foregroundColor: color,
                                                           NSAttributedString.Key.paragraphStyle: paragraphStyle])
      }

      DispatchQueue.main.async {
        completion(attributedString)
      }
    }
  }
}
