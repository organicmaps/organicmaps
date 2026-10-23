#pragma once

namespace gtk
{
inline constexpr char kAppCss[] = R"css(
.map-view { background-color: #ece9e2; }

button.map-button {
  min-width: 48px;
  min-height: 48px;
  padding: 0;
  border-radius: 999px;
  background-color: alpha(var(--window-bg-color), 0.96);
  color: var(--window-fg-color);
  box-shadow: 0 1px 4px alpha(black, 0.30);
}
button.map-button:hover { background-color: mix(var(--window-bg-color), var(--window-fg-color), 0.05); }
button.map-button:active { background-color: mix(var(--window-bg-color), var(--window-fg-color), 0.12); }
button.map-button.following { color: var(--accent-color); }
button.map-button.pending { color: alpha(var(--window-fg-color), 0.5); }

.map-attribution {
  background-color: alpha(var(--window-bg-color), 0.96);
  color: var(--window-fg-color);
  padding: 2px 6px;
  border-radius: 6px 6px 0 0;
}

)css";
}  // namespace gtk
