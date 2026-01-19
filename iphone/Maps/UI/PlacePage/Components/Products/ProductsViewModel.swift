struct ProductsViewModel {
  private let productsManager: ProductsManager.Type

  let title: String = L("support_organic_maps")
  let description: String
  let leadingSubtitle: String = L("already_donated")
  let trailingSubtitle: String = L("remind_me_later")
  let products: [Product]

  init(manager: ProductsManager.Type, configuration: ProductsConfiguration) {
    productsManager = manager
    description = configuration.placePagePrompt
    products = configuration.products
  }

  func didSelectProduct(_ product: Product) {
    UIViewController.topViewController().openUrl(product.link, externally: true)
    productsManager.didSelect(product)
  }

  func didClose(reason: ProductsPopupCloseReason) {
    productsManager.didCloseProductsPopup(with: reason)
  }
}
