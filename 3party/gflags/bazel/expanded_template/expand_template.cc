#include <fstream>
#include <regex>
#include <streambuf>
#include <string>
#include <string.h>
#include <vector>

struct Substitution {
  std::basic_regex<char> regex;
  std::string replacement;
};

// Simple app that does a regex search-and-replace on a template
// and outputs the result.
//
// To invoke:
// expand_template
// --template PATH
// --output PATH
// regex0;replacement0
// regex1;replacement1
// ...
//
// Since it's only used as a private implementation detail of a rule and not
// user invoked we don't bother with error checking.
int main(int argc, const char** argv) {
  // Parse args.
  const char* template_path = nullptr;
  const char* output_path = nullptr;
  std::vector<Substitution> substitutions;
  for (int i = 1; i < argc; ++i) {
    const char* arg = argv[i];
    if (strcmp(arg, "--template") == 0) {
      template_path = argv[++i];
    } else if (strcmp(arg, "--output") == 0) {
      output_path = argv[++i];
    } else {
      const char* mid = strchr(arg, ';');
      if (mid != nullptr) {
        substitutions.push_back(Substitution{
            std::basic_regex<char>(arg, mid - arg),
            std::string(mid + 1),
        });
      }
    }
  }

  // Read template.
  std::ifstream ifs(template_path);
  std::string str(std::istreambuf_iterator<char>(ifs),
                  (std::istreambuf_iterator<char>()));

  // Apply regexes.
  for (const auto& subst : substitutions) {
    str = std::regex_replace(str, subst.regex, subst.replacement);
  }

  // Output file.
  std::ofstream file(output_path);
  file << str;

  return 0;
}
