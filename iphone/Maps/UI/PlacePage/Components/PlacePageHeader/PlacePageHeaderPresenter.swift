protocol PlacePageHeaderPresenterProtocol: class {
  func configure()
  func onClosePress()
  func onExpandPress()
}

protocol PlacePageHeaderViewControllerDelegate: AnyObject {
  func previewDidPressClose()
  func previewDidPressExpand()
}

class PlacePageHeaderPresenter {
  enum HeaderType {
    case flexible
    case fixed
  }

  private weak var view: PlacePageHeaderViewProtocol?
  private let placePagePreviewData: PlacePagePreviewData
  private weak var delegate: PlacePageHeaderViewControllerDelegate?
  private let headerType: HeaderType

  init(view: PlacePageHeaderViewProtocol,
       placePagePreviewData: PlacePagePreviewData,
       delegate: PlacePageHeaderViewControllerDelegate?,
       headerType: HeaderType) {
    self.view = view
    self.delegate = delegate
    self.placePagePreviewData = placePagePreviewData
    self.headerType = headerType
  }
}

extension PlacePageHeaderPresenter: PlacePageHeaderPresenterProtocol {
  func configure() {
    view?.setTitle(placePagePreviewData.title ?? "")
    switch headerType {
    case .flexible:
      view?.setViewStyle("PPHeaderView")
      view?.setExpandViewEnabled(true)
    case .fixed:
      view?.setViewStyle("PPNavigationBarView")
      view?.setExpandViewEnabled(false)
    }
  }

  func onClosePress() {
    delegate?.previewDidPressClose()
  }

  func onExpandPress() {
    delegate?.previewDidPressExpand()
  }
}
