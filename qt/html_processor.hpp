#pragma once

#include <string>

#include <QString>

/// Function takes contents of a "copyright.html" file and removes all blocks 
/// from it which "lang" attribute doesn't match the "lang" parameter of the function.
QString ProcessCopyrightHtml(std::string const & html, std::string const & lang);
