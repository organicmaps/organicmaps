#include "about.hpp"

#include "app.hpp"
#include "main_window.hpp"

#include "map/framework.hpp"

#include "platform/platform.hpp"

#include <string>

namespace gtk
{
void ShowAboutDialog(MainWindow & window)
{
  auto * about = adw_about_dialog_new();
  adw_about_dialog_set_application_name(ADW_ABOUT_DIALOG(about), "Organic Maps");
  adw_about_dialog_set_application_icon(ADW_ABOUT_DIALOG(about), kAppId);
  adw_about_dialog_set_version(ADW_ABOUT_DIALOG(about), GetPlatform().Version().c_str());
  adw_about_dialog_set_developer_name(ADW_ABOUT_DIALOG(about), "Organic Maps contributors");
  adw_about_dialog_set_website(ADW_ABOUT_DIALOG(about), "https://organicmaps.app");
  adw_about_dialog_set_issue_url(ADW_ABOUT_DIALOG(about), "https://github.com/organicmaps/organicmaps/issues");
  adw_about_dialog_set_license_type(ADW_ABOUT_DIALOG(about), GTK_LICENSE_APACHE_2_0);
  auto const dataVersion = std::to_string(GetFramework().GetCurrentDataVersion());
  adw_about_dialog_set_comments(
      ADW_ABOUT_DIALOG(about),
      (std::string(
           "Map data © <a href=\"https://www.openstreetmap.org/copyright\">OpenStreetMap</a> contributors\n\n") +
       "Map data: " + dataVersion)
          .c_str());
  adw_about_dialog_add_legal_section(ADW_ABOUT_DIALOG(about), "OpenStreetMap", "© OpenStreetMap contributors",
                                     GTK_LICENSE_CUSTOM,
                                     "Map data is available under the Open Database License (ODbL).");
  adw_dialog_present(ADW_DIALOG(about), GTK_WIDGET(window.Window()));
}
}  // namespace gtk
