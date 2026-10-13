require 'minitest/autorun'
require 'tempfile'
require_relative './check_consistency'

class CategoryConsistencyTest < Minitest::Test
  def with_file(content)
    Tempfile.create('category_consistency') do |file|
      file.write(content)
      file.flush
      yield file.path
    end
  end

  def test_cpp_keys_use_the_supplied_file
    with_file('m_keys = {"category_food", "category_secondhand"};') do |path|
      assert_equal %w[category_food category_secondhand], load_categories_from_cpp(path)
    end
  end

  def test_missing_cpp_keys_fail
    with_file('m_keys = {};') do |path|
      assert_raises(ArgumentError) { load_categories_from_cpp(path) }
    end
  end

  def test_welsh_multiword_translations_and_group_keys
    keys = %w[category_nightlife]
    search = nil
    strings = nil
    with_file("@category_nightlife\nen:Nightlife\ncy:3Bywyd nos|clybiau nos\n\n") do |path|
      search = OmimParsers::CategoriesParser.new(keys).parse_file(path)
    end
    with_file("[category_nightlife]\nen = Nightlife\ncy = Bywyd nos\n\n") do |path|
      strings = OmimParsers::StringsParser.new(keys).parse_file(path)
    end
    assert_equal ['Bywyd nos', 'clybiau nos'], search.fetch('category_nightlife').fetch('cy')
    assert compare_categories(strings, search, keys)

    strings['category_nightlife']['cy'] = 'Bywyd Nos'
    capture_io { refute compare_categories(strings, search, keys) }
  end

  def test_locale_codes_do_not_require_a_language_list
    parser = OmimParsers::CategoriesParser.new([])
    assert_equal ['cy-GB', ['Bywyd nos']], parser.parse_line('cy-GB:Bywyd nos')
    assert_equal ['fil', ['Mga tindahan']], parser.parse_line('fil:Mga tindahan')
    assert_equal ['zh-Hant', ['商店']], parser.parse_line('zh-Hant:商店')
    assert_nil OmimParsers::StringsParser.new([]).parse_line('tags = ios, android')
  end

  def test_ignored_groups_do_not_leak_translations
    text = "@category_food\nen:4Groceries\n\n@shop\nen:Shop\n\n"
    with_file(text) do |path|
      assert_equal({'category_food' => {'en' => ['Groceries']}},
                   OmimParsers::CategoriesParser.new(['category_food']).parse_file(path))
    end
  end

  def test_missing_definitions_and_empty_parses_fail
    keys = ['category_food']
    strings = {'category_food' => {'cy' => 'Nwyddau groser'}}
    search = {'category_food' => {'cy' => ['Nwyddau groser']}}
    capture_io do
      refute compare_categories(strings, {}, keys)
      refute compare_categories({}, search, keys)
      refute compare_categories({}, {}, keys)
      refute compare_categories({'category_food' => {}}, {'category_food' => {}}, keys)
      refute compare_categories({}, {})
    end
  end
end
