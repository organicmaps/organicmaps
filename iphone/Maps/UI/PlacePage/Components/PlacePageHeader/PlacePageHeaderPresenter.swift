protocol PlacePageHeaderPresenterProtocol: AnyObject {
  var objectType: PlacePageObjectType { get }
  var canEditTitle: Bool { get }

  func configure()
  func onClosePress()
  func onExpandPress()
  func onShareButtonPress(from sourceView: UIView)
  func onExportTrackButtonPress(_ type: KmlFileType, from sourceView: UIView)
  func onCopy(_ content: String)
  func onFinishEditingTitle(_ newTitle: String)
}

protocol PlacePageHeaderViewControllerDelegate: AnyObject {
  func previewDidPressClose()
  func previewDidPressExpand()
  func previewDidPressShare(from sourceView: UIView)
  func previewDidPressExportTrack(_ type: KmlFileType, from sourceView: UIView)
  func previewDidCopy(_ content: String)
  func previewDidFinishEditingTitle(_ newTitle: String)
}

class PlacePageHeaderPresenter {
  enum HeaderType {
    case flexible
    case fixed
  }

  private weak var view: PlacePageHeaderViewProtocol?
  private let placePagePreviewData: PlacePagePreviewData
  let objectType: PlacePageObjectType
  private weak var delegate: PlacePageHeaderViewControllerDelegate?
  private let headerType: HeaderType

  init(view: PlacePageHeaderViewProtocol,
       placePagePreviewData: PlacePagePreviewData,
       objectType: PlacePageObjectType,
       delegate: PlacePageHeaderViewControllerDelegate?,
       headerType: HeaderType) {
    self.view = view
    self.delegate = delegate
    self.placePagePreviewData = placePagePreviewData
    self.objectType = objectType
    self.headerType = headerType
  }
}

extension PlacePageHeaderPresenter: PlacePageHeaderPresenterProtocol {
  var canEditTitle: Bool { objectType == .bookmark || objectType == .track }

  func configure() {
    view?.setTitle(placePagePreviewData.title, secondaryTitle: placePagePreviewData.secondaryTitle)
    view?.setShadowHidden(headerType == .flexible)
  }

  func onClosePress() {
    delegate?.previewDidPressClose()
  }

  func onExpandPress() {
    delegate?.previewDidPressExpand()
  }

  func onShareButtonPress(from sourceView: UIView) {
    delegate?.previewDidPressShare(from: sourceView)
  }

  func onExportTrackButtonPress(_ type: KmlFileType, from sourceView: UIView) {
    delegate?.previewDidPressExportTrack(type, from: sourceView)
  }

  func onCopy(_ content: String) {
    delegate?.previewDidCopy(content)
  }

  func onFinishEditingTitle(_ newTitle: String) {
    delegate?.previewDidFinishEditingTitle(newTitle)
  }
}
