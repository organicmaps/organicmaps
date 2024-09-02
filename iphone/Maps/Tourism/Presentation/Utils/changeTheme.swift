func changeTheme(themeCode: String) {
  let style: UIUserInterfaceStyle = (themeCode == "light") ? .light : (themeCode == "dark") ? .dark : .unspecified
  UIApplication.shared.keyWindow?.overrideUserInterfaceStyle = style
}
