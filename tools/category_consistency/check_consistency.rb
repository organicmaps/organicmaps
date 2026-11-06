#!/usr/bin/env ruby

require_relative './omim_parsers'

OMIM_ROOT = File.expand_path('../..', __dir__)
CPP_CATEGORIES_FILENAME = File.join(OMIM_ROOT, 'libs', 'search', 'displayed_categories.cpp')
CATEGORIES_FILENAME = File.join(OMIM_ROOT, 'data', 'categories.txt')
STRINGS_FILENAME = File.join(OMIM_ROOT, 'data', 'strings', 'strings.txt')
CATEGORIES_MATCHER = /\bm_keys\s*=\s*\{(.*?)\}\s*;/m

def load_categories_from_cpp(filename)
  raw_categories = File.read(filename, encoding: 'UTF-8')
  match = CATEGORIES_MATCHER.match(raw_categories)
  keys = match ? match[1].scan(/"([^"]+)"/).flatten : []
  raise ArgumentError, "No displayed category keys found in #{filename}" if keys.empty?

  keys
end

def compare_categories(string_cats, search_cats, keys = string_cats.keys | search_cats.keys)
  inconsistent_strings = {}
  consistent = !keys.empty?

  keys.each do |category_name|
    missing = []
    missing << 'strings.txt' unless string_cats.key? category_name
    missing << 'categories.txt' unless search_cats.key? category_name
    unless missing.empty?
      puts "Category '#{category_name}' not found in #{missing.join(' and ')}"
      consistent = false
      next
    end

    # Locales without search translations fall back to English; compare the translations both files provide.
    languages = string_cats[category_name].keys & search_cats[category_name].keys
    if languages.empty?
      puts "Category '#{category_name}' has no shared translation languages"
      consistent = false
      next
    end
    languages.each do |lang|
      translation = string_cats[category_name][lang]
      synonyms = search_cats[category_name][lang]
      unless synonyms.include? translation
        (inconsistent_strings[category_name] ||= {})[lang] = [translation, synonyms]
      end
    end
  end

  inconsistent_strings.each do |name, languages|
    puts "\nInconsistent category \"#{name}\""
    languages.each do |lang, values|
      string_value, category_value = values
      puts "\t#{lang} : \"#{string_value}\" is not matched by #{category_value}"
    end
  end
  consistent && inconsistent_strings.empty?
end

def check_search_categories_consistent
  cpp_categories = load_categories_from_cpp(CPP_CATEGORIES_FILENAME)
  categories_txt_parser = OmimParsers::CategoriesParser.new cpp_categories
  strings_txt_parser = OmimParsers::StringsParser.new cpp_categories

  search_categories = categories_txt_parser.parse_file(CATEGORIES_FILENAME)
  string_categories = strings_txt_parser.parse_file(STRINGS_FILENAME)

  compare_categories(string_categories, search_categories, cpp_categories) ? 0 : 1
end


if __FILE__ == $0
  exit check_search_categories_consistent()
end
