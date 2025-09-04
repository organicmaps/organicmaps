@available(iOS 18.0, *)
final class DefaultTranslationController {
  static let shared = DefaultTranslationController()

  private var translationHost: TranslationHostingController?
  private var isPresented = false
  private var binding: Binding<Bool> {
    Binding<Bool>(
      get: { [weak self] in self?.isPresented ?? false },
      set: { [weak self] presented in
        guard let self else { return }
        self.isPresented = presented
        if !presented {
          self.cleanupTranslationHost()
        }
      }
    )
  }

  private init() {}

  func presentTranslation(for text: String?, from viewController: UIViewController) {
    guard let text, translationHost == nil else { return }
    let host = TranslationHostingController(isPresented: binding, text: text, frame: viewController.view.frame)
    isPresented = true
    viewController.addChild(host)
    viewController.view.addSubview(host.view)
    host.didMove(toParent: viewController)
    translationHost = host
  }

  private func cleanupTranslationHost() {
    guard let host = translationHost else { return }
    host.willMove(toParent: nil)
    host.view.removeFromSuperview()
    host.removeFromParent()
    translationHost = nil
  }
}

@available(iOS 18, *)
private class TranslationHostingController: UIHostingController<TranslateAccessoryView> {

  init(isPresented: Binding<Bool>, text: String, frame: CGRect) {
    let view = TranslateAccessoryView(isPresented: isPresented, text: text, frame: frame)
    super.init(rootView: view)
  }

  @available(*, unavailable)
  required dynamic init?(coder aDecoder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }
}

import SwiftUI
import Translation

@available(iOS 18.0, *)
private struct TranslateAccessoryView: View {
  @Binding var isPresented: Bool
  let text: String
  let frame: CGRect

  var body: some View {
    Color.clear
      .frame(width: frame.width, height: frame.height)
      .translationPresentation(isPresented: $isPresented, text: text, arrowEdge: .trailing)
  }
}
