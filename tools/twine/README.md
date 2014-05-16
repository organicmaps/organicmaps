# Twine

Twine is a command line tool for managing your strings and their translations. These strings are all stored in a master text file and then Twine uses this file to import and export strings in a variety of file types, including iOS and Mac OS X `.strings` files, Android `.xml` files, gettext `.po` files, and [jquery-localize][jquerylocalize] `.json` files. This allows individuals and companies to easily share strings across multiple projects, as well as export strings in any format the user wants.

## Install

### As a Gem

Twine is most easily installed as a Gem.

	$ gem install twine

### From Source

You can also run Twine directly from source. However, it requires [rubyzip][rubyzip] in order to create and read standard zip files.

	$ gem install rubyzip
	$ git clone git://github.com/mobiata/twine.git
	$ cd twine
	$ ./twine --help

Make sure you run the `twine` executable at the root of the project as it properly sets up your Ruby library path. The `bin/twine` executable does not.

## String File Format

Twine stores all of its strings in a single file. The format of this file is a slight variant of the [Git][git] config file format, which itself is based on the old [Windows INI file][INI] format. The entire file is broken up into sections, which are created by placing the section name between two pairs of square brackets. Sections are optional, but they are a recommended way of breaking your strings into smaller, more manageable chunks.

Each grouping section contains N string definitions. These string definitions start with the string key placed within a single pair of square brackets. This string definition then contains a number of key-value pairs, including a comment, a comma-separated list of tags (which are used by Twine to select a subset of strings), and all of the translations.

### Tags

Tags are used by Twine as a way to only work with a subset of your strings at any given point in time. Each string can be assigned zero or more tags which are separated by commas. Tags are optional, though highly recommended. You can get a list of all strings currently missing tags by executing the `generate-report` command.

### Whitespace

Whitepace in this file is mostly ignored. If you absolutely need to put spaces at the beginning or end of your translated string, you can wrap the entire string in a pair of `` ` `` characters. If your actual string needs to start *and* end with a grave accent, you can wrap it in another pair of `` ` `` characters. See the example, below.

### Example

	[[General]]
		[yes]
			en = Yes
			es = Sí
			fr = Oui
			ja = はい
		[no]
			en = No
			fr = Non
			ja = いいえ
	
	[[Errors]]
		[path_not_found_error]
			en = The file '%@' could not be found.
			tags = app1,app6
			comment = An error describing when a path on the filesystem could not be found.
		[network_unavailable_error]
			en = The network is currently unavailable.
			tags = app1
			comment = An error describing when the device can not connect to the internet.
	
	[[Escaping Example]]
		[list_item_separator]
			en = `, `
			tags = mytag
			comment = A string that should be placed between multiple items in a list. For example: Red, Green, Blue
		[grave_accent_quoted_string]
			en = ``%@``
			tags = myothertag
			comment = This string will evaluate to `%@`.

## Supported Output Formats

Twine currently supports the following formats for outputting strings:

* [iOS and OS X String Resources][applestrings] (format: apple)
* [Android String Resources][androidstrings] (format: android)
* [Gettext PO Files][gettextpo] (format: gettext)
* [jquery-localize Language Files][jquerylocalize] (format: jquery)
* [Django PO Files][djangopo] (format: django)

If you would like to enable twine to create language files in another format, create an appropriate formatter in `lib/twine/formatters`.

## Usage

	Usage: twine COMMAND STRINGS_FILE [INPUT_OR_OUTPUT_PATH] [--lang LANG1,LANG2...] [--tags TAG1,TAG2,TAG3...] [--format FORMAT]
	
### Commands

#### `generate-string-file`

This command creates an Apple or Android strings file from the master strings data file.

	$ twine generate-string-file /path/to/strings.txt values-ja.xml --tags common,app1
	$ twine generate-string-file /path/to/strings.txt Localizable.strings --lang ja --tags mytag
	$ twine generate-string-file /path/to/strings.txt all-english.strings --lang en

#### `generate-all-string-files`

This command is a convenient way to call `generate-string-file` multiple times. It uses standard Mac OS X, iOS, and Android conventions to figure out exactly which files to create given a parent directory. For example, if you point it to a parent directory containing `en.lproj`, `fr.lproj`, and `ja.lproj` subdirectories, Twine will create a `Localizable.strings` file of the appropriate language in each of them. This is often the command you will want to execute during the build phase of your project.

	$ twine generate-all-string-files /path/to/strings.txt /path/to/project/locales/directory --tags common,app1

#### `consume-string-file`

This command slurps all of the strings from a `.strings` or `.xml` file and incorporates the translated text into the master strings data file. This is a simple way to incorporate any changes made to a single file by one of your translators. It will only identify strings that already exist in the master data file.

	$ twine consume-string-file /path/to/strings.txt fr.strings
	$ twine consume-string-file /path/to/strings.txt Localizable.strings --lang ja
	$ twine consume-string-file /path/to/strings.txt es.xml

#### `consume-all-string-files`

This command reads in a folder containing many `.strings` or `.xml` files. These files should be in a standard folder hierarchy so that twine knows the language of each file. When combined with the `--developer-language`, `--consume-comments`, and `--consume-all` flags, this command is a great way to create your initial strings data file from an existing iOS or Android project. Just make sure that you create a blank strings.txt file, first!

	$ twine consume-all-string-files strings.txt Resources/Locales --developer-language en --consume-all --consume-comments

#### `generate-loc-drop`

This command is a convenient way to generate a zip file containing files created by the `generate-string-file` command. It is often used for creating a single zip containing a large number of strings in all languages which you can then hand off to your translation team.

	$ twine generate-loc-drop /path/to/strings.txt LocDrop1.zip
	$ twine generate-loc-drop /path/to/strings.txt LocDrop2.zip --lang en,fr,ja,ko --tags common,app1

#### `consume-loc-drop`

This command is a convenient way of taking a zip file and executing the `consume-string-file` command on each file within the archive. It is most often used to incorporate all of the changes made by the translation team after they have completed work on a localization drop.

	$ twine consume-loc-drop /path/to/strings.txt LocDrop2.zip

#### `generate-report`

This command gives you useful information about your strings. It will tell you how many strings you have, how many have been translated into each language, and whether your master strings data file has any duplicate string keys.

	$ twine generate-report /path/to/strings.txt

## Creating Your First strings.txt File

The easiest way to create your first strings.txt file is to run the `consume-all-string-files` command. The one caveat is to first create a blank strings.txt file to use as your starting point. Then, just point the `consume-all-string-files` command at a directory in your project containing all of your iOS, OS X, or Android strings files.

	$ touch strings.txt
	$ twine consume-all-string-files strings.txt Resources/Locales --developer-language en --consume-all --consume-comments

## Twine and Your Build Process

It is easy to incorporate Twine right into your iOS and OS X app build processes.

1. In your project folder, create all of the `.lproj` directories that you need. It does not really matter where they are. We tend to put them in `Resources/Locales/`.
2. Run the `generate-all-string-files` command to create all of the string files you need in these directories. For example,

		$ twine generate-all-string-files strings.txt Resources/Locales/ --tags tag1,tag2

	Make sure you point Twine at your strings data file, the directory that contains all of your `.lproj` directories, and the tags that describe the strings you want to use for this project.
3. Drag the `Resources/Locales/` directory to the Xcode project navigator so that Xcode knows to include all of these strings files in your build.
4. In Xcode, navigate to the "Build Phases" tab of your target.
5. Click on the "Add Build Phase" button and select "Add Run Script".
6. Drag the new "Run Script" build phase up so that it runs earlier in the build process. It doesn't really matter where, as long as it happens before the resources are copied to your bundle.
7. Edit your script to run the exact same command you ran in step (2) above.

Now, whenever you build your application, Xcode will automatically invoke Twine to make sure that your `.strings` files are up-to-date.

## User Interface

* [Twine TextMate 2 Bundle](https://github.com/mobiata/twine.tmbundle) — This [TextMate 2](https://github.com/textmate/textmate) bundle will make it easier for you to work with Twine strings files. In particular, it lets you use code folding to easily collapse and expand both strings and sections.
* [twine_ui](https://github.com/Daij-Djan/twine_ui) — A user interface for Twine written by [Dominik Pich](https://github.com/Daij-Djan/). Consider using this if you would prefer to use Twine without dropping to a command line.

## Contributors

Many thanks to all of the contributors to the Twine project, including:

* [Blake Watters](https://github.com/blakewatters)
* [Ishitoya Kentaro](https://github.com/kent013)
* [Joseph Earl](https://github.com/JosephEarl)
* [Kevin Everets](https://github.com/keverets)
* [Kevin Wood](https://github.com/kwood)
* [Mohammad Hejazi](https://github.com/MohammadHejazi)
* [Robert Guo](http://www.robertguo.me/)
* [Shai Shamir](https://github.com/pichirichi)


[rubyzip]: http://rubygems.org/gems/rubyzip
[git]: http://git-scm.org/
[INI]: http://en.wikipedia.org/wiki/INI_file
[applestrings]: http://developer.apple.com/documentation/Cocoa/Conceptual/LoadingResources/Strings/Strings.html
[androidstrings]: http://developer.android.com/guide/topics/resources/string-resource.html
[gettextpo]: http://www.gnu.org/savannah-checkouts/gnu/gettext/manual/html_node/PO-Files.html
[jquerylocalize]: https://github.com/coderifous/jquery-localize
[djangopo]: https://docs.djangoproject.com/en/dev/topics/i18n/translation/
