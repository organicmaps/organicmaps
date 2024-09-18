import UIKit
import SwiftUI

class ProfileViewController: UIViewController {
  private var profileVM: ProfileViewModel
  
  init(profileVM: ProfileViewModel) {
    self.profileVM = profileVM
    super.init(
      nibName: nil,
      bundle: nil
    )
  }
  
  required init?(coder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }
  
  override func viewDidLoad() {
    super.viewDidLoad()
    
    integrateSwiftUIScreen(
      ProfileScreen(
        profileVM: profileVM,
        onPersonalDataClick: {
          let destinationVC = PersonalDataViewController(profileVM: self.profileVM)
          self.navigationController?.pushViewController(destinationVC, animated: true)
        }
      )
    )
  }
}

struct ProfileScreen: View {
  @ObservedObject var profileVM: ProfileViewModel
  let onPersonalDataClick: () -> Void
  @ObservedObject var themeVM: ThemeViewModel = ThemeViewModel(
    profileRepository: ProfileRepositoryImpl(
      profileService: ProfileServiceImpl(userPreferences: UserPreferences.shared),
      personalDataPersistenceController: PersonalDataPersistenceController.shared
    ),
    userPreferences: UserPreferences.shared
  )
  @State private var isSheetOpen = false
  @State private var signOutLoading = false
  
  func onLanguageClick () {
    navigateToLanguageSettings()
    profileVM.setLanguageOnSystemLocaleChange()
  }
  
  var body: some View {
    ScrollView {
      VStack (alignment: .leading) {
        AppTopBar(title: L("tourism_profile"))
        VerticalSpace(height: 16)
        
        if let personalData = profileVM.personalData {
          ProfileBar(personalData: personalData)
        }
        VerticalSpace(height: 32)
        
        VStack(spacing: 20) {
          if let currencyRates = profileVM.currencyRates {
            CurrencyRatesView(currencyRates: currencyRates)
          }
          
          GenericProfileItem(
            label: L("personal_data"),
            icon: "person.circle",
            onClick: {
              onPersonalDataClick()
            }
          )
          
          GenericProfileItem(
            label: L("language"),
            icon: "globe",
            onClick: {
              onLanguageClick()
            }
          )
          
          ThemeSwitch(themeViewModel: themeVM)
          
          GenericProfileItem(
            label: L("sign_out"),
            icon: "rectangle.portrait.and.arrow.right",
            isLoading: signOutLoading,
            onClick: {
              isSheetOpen = true
            }
          )
        }
      }
      .padding(16)
    }
    .overlay(
      Group {
        if profileVM.shouldShowMessageOnProfileScreen {
          ToastView(message: profileVM.messageToShowOnProfileScreen, isPresented: $profileVM.shouldShowMessageOnProfileScreen)
            .padding(.bottom)
        }
      },
      alignment: .bottom
    )
    .sheet(isPresented: $isSheetOpen) {
      SignOutWarning(
        onSignOutClick: {
          isSheetOpen = false
          signOutLoading = true
          profileVM.signOut()
        },
        onCancelClick: {
          isSheetOpen = false
        }
      )
    }
  }
}

struct ProfileBar: View {
  var personalData: PersonalData
  
  var body: some View {
    HStack(alignment: .center) {
      LoadImageView(url: personalData.pfpUrl)
        .frame(width: 100, height: 100)
        .background(Color.surface)
        .clipShape(Circle())
      
      HorizontalSpace(width: 16)
      
      VStack(alignment: .leading) {
        Text(personalData.fullName)
          .textStyle(TextStyle.h2)
        
        UICountryAsLabelView(code: personalData.country)
          .frame(height: 30)
      }
    }
  }
}


struct CurrencyRatesView: View {
  var currencyRates: CurrencyRates
  
  var body: some View {
    HStack(spacing: 16) {
      CurrencyRatesItem(countryCode: "US", flagEmoji: "🇺🇸", value: String(format: "%.2f", currencyRates.usd))
      CurrencyRatesItem(countryCode: "EU", flagEmoji: "🇪🇺", value: String(format: "%.2f", currencyRates.eur))
      CurrencyRatesItem(countryCode: "RU", flagEmoji: "🇷🇺", value: String(format: "%.2f", currencyRates.rub))
    }
    .frame(maxWidth: .infinity, maxHeight: profileItemHeight)
    .padding(16)
    .overlay(
      RoundedRectangle(cornerRadius: 20)
        .stroke(Color.border, lineWidth: 2)
    )
    .cornerRadius(20)
  }
}

struct CurrencyRatesItem: View {
  var countryCode: String
  var flagEmoji: String
  var value: String
  
  var body: some View {
    HStack {
      Text(flagEmoji)
        .font(.system(size: 33))
      
      Text(value)
    }
  }
}

struct GenericProfileItem: View {
  var label: String
  var icon: String
  var isLoading: Bool = false
  var onClick: () -> Void
  
  var body: some View {
    HStack {
      Text(label)
        .textStyle(TextStyle.b1)
      
      Spacer()
      
      if isLoading {
        ProgressView()
      } else {
        Image(systemName: icon)
          .foregroundColor(Color.border)
      }
    }
    .contentShape(Rectangle())
    .onTapGesture {
      onClick()
    }
    .frame(height: profileItemHeight)
    .padding()
    .overlay(
      RoundedRectangle(cornerRadius: 20)
        .stroke(Color.border, lineWidth: 2)
    )
    .cornerRadius(20)
  }
}

struct ThemeSwitch: View {
  @Environment(\.colorScheme) var colorScheme
  @ObservedObject var themeViewModel: ThemeViewModel
  
  var body: some View {
    HStack {
      Text(L("Dark Theme"))
        .textStyle(TextStyle.b1)
      
      Spacer()
      
      Toggle(isOn: Binding(
        get: {
          colorScheme == .dark
        },
        set: { isDark in
          let themeCode = isDark ? "dark" : "light"
          themeViewModel.setTheme(themeCode: themeCode)
          changeTheme(themeCode: themeCode)
          themeViewModel.updateThemeOnServer(themeCode: themeCode)
        }
      )) {
        Text("")
      }
      .labelsHidden()
      .frame(height: 10)
    }
    .frame(height: profileItemHeight)
    .padding()
    .overlay(
      RoundedRectangle(cornerRadius: 20)
        .stroke(Color.border, lineWidth: 2)
    )
    .cornerRadius(20)
  }
}

struct SignOutWarning: View {
  var onSignOutClick: () -> Void
  var onCancelClick: () -> Void
  
  var body: some View {
    VStack(spacing: 16) {
      Text("Are you sure you want to sign out?")
        .font(.headline)
      
      HStack {
        SecondaryButton(
          label: L("cancel"),
          onClick: onCancelClick
        )
        
        PrimaryButton(
          label: L("sign_out"),
          onClick: onSignOutClick
        )
      }
    }
    .padding()
  }
}

let profileItemHeight: CGFloat = 25
