#include "app.hpp"

#include "base/assert.hpp"

namespace gtk
{
App & GetApp()
{
  static App app;
  return app;
}

Framework & GetFramework()
{
  CHECK(GetApp().framework, ());
  return *GetApp().framework;
}

GtkWidget * MakeIconButton(char const * iconName, char const * tooltip, char const * cssClass)
{
  GtkWidget * b = gtk_button_new_from_icon_name(iconName);
  if (tooltip)
  {
    gtk_widget_set_tooltip_text(b, tooltip);
    gtk_accessible_update_property(GTK_ACCESSIBLE(b), GTK_ACCESSIBLE_PROPERTY_LABEL, tooltip, -1);
  }
  if (cssClass)
    gtk_widget_add_css_class(b, cssClass);
  return b;
}

}  // namespace gtk
