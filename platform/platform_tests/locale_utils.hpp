namespace platform
{
std::string GetSystemDecimalSeparator();
std::string GetSystemGroupSeparator();
std::string ReplaceGroupingSeparators(std::string const & valueString, std::string const & groupingSeparator);
std::string ReplaceDecimalSeparator(std::string const & valueString, std::string const & decimalSeparator);
std::string LocalizeValueString(std::string const & valueString, Locale const & loc);
}

