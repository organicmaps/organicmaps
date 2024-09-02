import UIKit
import SwiftUI

class ProfileViewController: UIViewController {
  override func viewDidLoad() {
    super.viewDidLoad()
    
    integrateSwiftUIScreen(ProfileScreen())
  }
}

struct ProfileScreen: View {
  @ObservedObject var profileVM: ProfileViewModel = ProfileViewModel(
    currencyRepository: CurrencyRepositoryImpl(
      currencyService: CurrencyServiceImpl(),
      currencyPersistenceController: CurrencyPersistenceController.shared
    ),
    profileRepository: ProfileRepositoryImpl(
      personalDataService: PersonalDataServiceImpl(),
      personalDataPersistenceController: PersonalDataPersistenceController.shared
    ),
    authRepository: AuthRepositoryImpl(authService: AuthServiceImpl()),
    userPreferences: UserPreferences.shared
  )
  
  @ObservedObject var themeVM: ThemeViewModel = ThemeViewModel(userPreferences: UserPreferences.shared)
  @State private var isSheetOpen = false
  @State private var signOutLoading = false
  
  @State private var showToast = false
  
  func onPersonalDataClick() {
    // TODO: cmon
  }
  
  func onLanguageClick () {
    navigateToLanguageSettings()
  }
  
  var body: some View {
    ScrollView {
      VStack (alignment: .leading) {
        AppTopBar(title: L("tourism_profile"))
        Spacer().frame(height: 16)
        
        if let personalData = profileVM.personalData {
          ProfileBar(personalData: personalData)
        }
        Spacer().frame(height: 32)
        
        if let currencyRates = profileVM.currencyRates {
          CurrencyRatesView(currencyRates: currencyRates)
          Spacer().frame(height: 20)
        }
        
        GenericProfileItem(
          label: L("personal_data"),
          icon: "person.circle",
          onClick: {
            onPersonalDataClick()
          }
        )
        Spacer().frame(height: 20)
        
        GenericProfileItem(
          label: L("language"),
          icon: "globe",
          onClick: {
            onLanguageClick()
          }
        )
        Spacer().frame(height: 20)
        ThemeSwitch(themeViewModel: themeVM)
        Spacer().frame(height: 20)
        
        GenericProfileItem(
          label: L("sign_out"),
          icon: "rectangle.portrait.and.arrow.right",
          isLoading: signOutLoading,
          onClick: {
            isSheetOpen = true
          }
        )
      }
      .padding(16)
    }
    .overlay(
      Group {
        if showToast {
          ToastView(message: "This is a toast message", isPresented: $showToast)
            .padding(.bottom)
        }
      },
      alignment: .bottom
    )
    .sheet(isPresented: $isSheetOpen) {
      SignOutWarning(
        onSignOutClick: {
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
        .clipShape(Circle())
      
      Spacer().frame(width: 16)
      
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
      Text("Dark Theme")
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
        Button("Cancel", action: onCancelClick)
          .padding()
          .background(SwiftUI.Color.clear)
          .cornerRadius(8)
        
        Button("Sign Out", action: onSignOutClick)
          .padding()
          .background(Color.heartRed)
          .foregroundColor(Color.onBackground)
          .cornerRadius(8)
      }
    }
    .padding()
  }
}

let profileItemHeight: CGFloat = 25
