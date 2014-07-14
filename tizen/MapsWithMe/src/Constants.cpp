#include "Constants.hpp"

using namespace Tizen::Graphics;

namespace consts
{
Color const white = Color(0xFF,0xFF,0xFF);
Color const gray = Color(0xB0,0xB0,0xB0);
Color const green = Color(21, 199, 131);
Color const blue = Color(0, 186, 255);
Color const black = Color(0x00,0x00,0x00);
Color const red = Color(0xFF,0x00,0x00);
Color const mainMenuGray = Color(65,68,81);

int const topHght = 27; //margin from top to text
int const btwWdth = 20; //margin between texts
int const imgWdth = 60; //left img width
int const imgHght = 60; //left img height
int const lstItmHght = 120; //list item height
int const backWdth = 150; //back txt width
int const mainFontSz = 45; //big font
int const mediumFontSz = 33; //medium font
int const minorFontSz = 25; //small font
int const searchBarHeight = 112; //search bar on main form

int const markPanelHeight = 1.5 * lstItmHght;
int const btnSz = 55;

int const editBtnSz = btnSz - 15;
int const headerItemHeight = markPanelHeight;
int const settingsItemHeight = 2 * lstItmHght;
int const groupItemHeight = lstItmHght;
int const messageItemHeight = 2 * lstItmHght;
int const headerSettingsHeight = headerItemHeight + settingsItemHeight;
int const allItemsHeight = headerItemHeight + settingsItemHeight + groupItemHeight + messageItemHeight;

const char * BM_COLOR_RED = "placemark-red";
const char * BM_COLOR_YELLOW = "placemark-yellow";
const char * BM_COLOR_BLUE = "placemark-blue";
const char * BM_COLOR_GREEN = "placemark-green";
const char * BM_COLOR_PURPLE = "placemark-purple";
const char * BM_COLOR_ORANGE = "placemark-orange";
const char * BM_COLOR_BROWN = "placemark-brown";
const char * BM_COLOR_PINK = "placemark-pink";

int const distanceWidth = 200;
// bookmark categories
int const deleteWidth = 200;

int const DoubleClickTimeout = 200;

const char * SETTINGS_MAP_LICENSE = "map_license_agreement";
}
