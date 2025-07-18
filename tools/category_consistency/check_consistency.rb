#!/usr/bin/env ruby

require_relative './omim_parsers'

ROOT = File.expand_path(File.dirname(__FILE__))
OMIM_ROOT = File.join(ROOT, '..', '..', '..')
CPP_CATEGORIES_FILENAME = File.join(OMIM_ROOT, 'search', 'displayed_categories.cpp')
CATEGORIES_FILENAME = File.join(OMIM_ROOT, 'data', 'categories.txt')
STRINGS_FILENAME = File.join(OMIM_ROOT, 'data', 'strings', 'strings.txt')
CATEGORIES_MATCHER = /m_keys = \{(.*)\};/m

def load_categories_from_cpp(filename)
  raw_categories = File.read(CPP_CATEGORIES_FILENAME)
  match = CATEGORIES_MATCHER.match(raw_categories)
  if match
    cpp_categories = match[1].split(/,\s+/)
    # Delete quotes
    cpp_categories.map { |cat| cat.gsub!(/^"|"$/, '') }
    cpp_categories
  end
end

def compare_categories(string_cats, search_cats)
  inconsistent_strings = {}

  string_cats.each do |category_name, category|
    if !search_cats.include? category_name
      puts "Category '#{category_name}' not found in categories.txt"
      next
    end
    category.each do |lang, translation|
      if search_cats[category_name].include? lang
        if !search_cats[category_name][lang].include? translation
          not_found_cats_list = search_cats[category_name][lang]
          (inconsistent_strings[category_name] ||= {})[lang] = [translation, not_found_cats_list]
        end
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
  inconsistent_strings.empty?
end

def check_search_categories_consistent
  cpp_categories = load_categories_from_cpp(CPP_CATEGORIES_FILENAME)
  categories_txt_parser = OmimParsers::CategoriesParser.new cpp_categories
  strings_txt_parser = OmimParsers::StringsParser.new cpp_categories

  search_categories = categories_txt_parser.parse_file(CATEGORIES_FILENAME)
  string_categories = strings_txt_parser.parse_file(STRINGS_FILENAME)

  compare_categories(string_categories, search_categories) ? 0 : 1
end


if __FILE__ == $0
  exit check_search_categories_consistent()
end
