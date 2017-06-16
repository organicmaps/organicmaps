# Twine

Twine is a command line tool for managing your strings and their translations. These are all stored in a master text file and then Twine uses this file to import and export localization files in a variety of types, including iOS and Mac OS X `.strings` files, Android `.xml` files, gettext `.po` files, and [jquery-localize][jquerylocalize] `.json` files. This allows individuals and companies to easily share translations across multiple projects, as well as export localization files in any format the user wants.

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

## Twine File Format

Twine stores everything in a single file, the Twine data file. The format of this file is a slight variant of the [Git][git] config file format, which itself is based on the old [Windows INI file][INI] format. The entire file is broken up into sections, which are created by placing the section name between two pairs of square brackets. Sections are optional, but they are the recommended way of grouping your definitions into smaller, more manageable chunks.

Each grouping section contains N definitions. These definitions start with the key placed within a single pair of square brackets. It then contains a number of key-value pairs, including a comment, a comma-separated list of tags and all of the translations.

### Placeholders

Twine supports [`printf` style placeholders][printf] with one peculiarity: `@` is used for strings instead of `s`. This is because Twine started out as a tool for iOS and OS X projects.

### Tags

Tags are used by Twine as a way to only work with a subset of your definitions at any given point in time. Each definition can be assigned zero or more tags which are separated by commas. Tags are optional, though highly recommended. You can get a list of all definitions currently missing tags by executing the [`validate-twine-file`](#validate-twine-file) command with the `--pedantic` option.

When generating a localization file, you can specify which definitions should be included using the `--tags` option. Provide a comma separated list of tags to match all definitions that contain any of the tags (`--tags tag1,tag2` matches all definitions tagged with `tag1` _or_ `tag2`). Provide multiple `--tags` options to match defintions containing all specified tags (`--tags tag1 --tags tag2` matches all definitions tagged with `tag1` _and_ `tag2`). You can match definitions _not_ containing a tag by prefixing the tag with a tilde (`--tags ~tag1` matches all definitions _not_ tagged with `tag1`). All three options are combinable.

### Whitespace

Whitepace in this file is mostly ignored. If you absolutely need to put spaces at the beginning or end of your translated string, you can wrap the entire string in a pair of `` ` `` characters. If your actual string needs to start *and* end with a grave accent, you can wrap it in another pair of `` ` `` characters. See the example, below.

### References

If you want a definition to inherit the values of another definition, you can use a reference. Any property not specified for a definition will be taken from the reference.

### Example

```ini
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
		[dismiss_error]
			ref = yes
			en = Dismiss

	[[Escaping Example]]
		[list_item_separator]
			en = `, `
			tags = mytag
			comment = A string that should be placed between multiple items in a list. For example: Red, Green, Blue
		[grave_accent_quoted_string]
			en = ``%@``
			tags = myothertag
			comment = This string will evaluate to `%@`.
```

## Supported Output Formats

Twine currently supports the following output formats:

* [iOS and OS X String Resources][applestrings] (format: apple)
* [Android String Resources][androidstrings] (format: android)
* [Gettext PO Files][gettextpo] (format: gettext)
* [jquery-localize Language Files][jquerylocalize] (format: jquery)
* [Django PO Files][djangopo] (format: django)
* [Tizen String Resources][tizen] (format: tizen)
* [Flash/Flex Properties][flash] (format: flash)

If you would like to enable Twine to create localization files in another format, read the wiki page on how to create an appropriate formatter.

## Usage

	Usage: twine COMMAND TWINE_FILE [INPUT_OR_OUTPUT_PATH] [--lang LANG1,LANG2...] [--tags TAG1,TAG2,TAG3...] [--format FORMAT]

### Commands

#### `generate-localization-file`

This command creates a localization file from the Twine data file. If the output file would not contain any translations, Twine will exit with an error.

	$ twine generate-localization-file /path/to/twine.txt values-ja.xml --tags common,app1
	$ twine generate-localization-file /path/to/twine.txt Localizable.strings --lang ja --tags mytag
	$ twine generate-localization-file /path/to/twine.txt all-english.strings --lang en

#### `generate-all-localization-files`

This command is a convenient way to call [`generate-localization-file`](#generate-localization-file) multiple times. It uses standard conventions to figure out exactly which files to create given a parent directory. For example, if you point it to a parent directory containing `en.lproj`, `fr.lproj`, and `ja.lproj` subdirectories, Twine will create a `Localizable.strings` file of the appropriate language in each of them. However, files that would not contain any translations will not be created; instead warnings will be logged to `stderr`. This is often the command you will want to execute during the build phase of your project.

	$ twine generate-all-localization-files /path/to/twine.txt /path/to/project/locales/directory --tags common,app1

#### `consume-localization-file`

This command slurps all of the translations from a localization file and incorporates the translated strings into the Twine data file. This is a simple way to incorporate any changes made to a single file by one of your translators. It will only identify definitions that already exist in the data file.

	$ twine consume-localization-file /path/to/twine.txt fr.strings
	$ twine consume-localization-file /path/to/twine.txt Localizable.strings --lang ja
	$ twine consume-localization-file /path/to/twine.txt es.xml

#### `consume-all-localization-files`

This command reads in a folder containing many localization files. These files should be in a standard folder hierarchy so that Twine knows the language of each file. When combined with the `--developer-language`, `--consume-comments`, and `--consume-all` flags, this command is a great way to create your initial Twine data file from an existing project. Just make sure that you create a blank Twine data file first!

	$ twine consume-all-localization-files twine.txt Resources/Locales --developer-language en --consume-all --consume-comments

#### `generate-localization-archive`

This command is a convenient way to generate a zip file containing files created by the [`generate-localization-file`](#generate-localization-file) command. If a file would not contain any translated strings, it is skipped and a warning is logged to `stderr`. This command can be used to create a single zip containing a large number of translations in all languages which you can then hand off to your translation team.

	$ twine generate-localization-archive /path/to/twine.txt LocDrop1.zip
	$ twine generate-localization-archive /path/to/twine.txt LocDrop2.zip --lang en,fr,ja,ko --tags common,app1

#### `consume-localization-archive`

This command is a convenient way of taking a zip file and executing the [`consume-localization-file`](#consume-localization-file) command on each file within the archive. It is most often used to incorporate all of the changes made by the translation team after they have completed work on a localization archive.

	$ twine consume-localization-archive /path/to/twine.txt LocDrop2.zip

#### `validate-twine-file`

This command validates that the Twine data file can be parsed, contains no duplicate keys, and that no key contains invalid characters. It will exit with a non-zero status code if any of those criteria are not met.

	$ twine validate-twine-file /path/to/twine.txt

## Creating Your First Twine Data File

The easiest way to create your first Twine data file is to run the [`consume-all-localization-files`](#consume-all-localization-files) command. The one caveat is to first create a blank file to use as your starting point. Then, just point the `consume-all-localization-files` command at a directory in your project containing all of your localization files.

	$ touch twine.txt
	$ twine consume-all-localization-files twine.txt Resources/Locales --developer-language en --consume-all --consume-comments

## Twine and Your Build Process

### Xcode

It is easy to incorporate Twine right into your iOS and OS X app build processes.

1. In your project folder, create all of the `.lproj` directories that you need. It does not really matter where they are. We tend to put them in `Resources/Locales/`.
2. Run the [`generate-all-localization-files`](#generate-all-localization-files) command to create all of the `.strings` files you need in these directories. For example,

		$ twine generate-all-localization-files twine.txt Resources/Locales/ --tags tag1,tag2

	Make sure you point Twine at your data file, the directory that contains all of your `.lproj` directories, and the tags that describe the definitions you want to use for this project.
3. Drag the `Resources/Locales/` directory to the Xcode project navigator so that Xcode knows to include all of these `.strings` files in your build.
4. In Xcode, navigate to the "Build Phases" tab of your target.
5. Click on the "Add Build Phase" button and select "Add Run Script".
6. Drag the new "Run Script" build phase up so that it runs earlier in the build process. It doesn't really matter where, as long as it happens before the resources are copied to your bundle.
7. Edit your script to run the exact same command you ran in step (2) above.

Now, whenever you build your application, Xcode will automatically invoke Twine to make sure that your `.strings` files are up-to-date.

### Android Studio/Gradle

Add the following task at the top level in app/build.gradle:
```
task generateLocalizations {
    String script = 'if hash twine 2>/dev/null; then twine generate-localization-file twine.txt ./src/main/res/values/generated_strings.xml; fi'
    exec {
        executable "sh"
        args '-c', script
    }
}
```

Now every time you build your app the localization files are generated from the Twine file.


## User Interface

* [Twine TextMate 2 Bundle](https://github.com/mobiata/twine.tmbundle) — This [TextMate 2](https://github.com/textmate/textmate) bundle will make it easier for you to work with Twine files. In particular, it lets you use code folding to easily collapse and expand both definitions and sections.
* [twine_ui](https://github.com/Daij-Djan/twine_ui) — A user interface for Twine written by [Dominik Pich](https://github.com/Daij-Djan/). Consider using this if you would prefer to use Twine without dropping to a command line.

## Extending Twine

If there's a format Twine does not yet support and you're keen to change that, check out the [documentation](documentation/formatters.md).

## Contributors

Many thanks to all of the contributors to the Twine project, including:

* [Blake Watters](https://github.com/blakewatters)
* [bootstraponline](https://github.com/bootstraponline)
* [Ishitoya Kentaro](https://github.com/kent013)
* [Joseph Earl](https://github.com/JosephEarl)
* [Kevin Everets](https://github.com/keverets)
* [Kevin Wood](https://github.com/kwood)
* [Mohammad Hejazi](https://github.com/MohammadHejazi)
* [Robert Guo](http://www.robertguo.me/)
* [Sebastian Ludwig](https://github.com/sebastianludwig)
* [Sergey Pisarchik](https://github.com/SergeyPisarchik)
* [Shai Shamir](https://github.com/pichirichi)


[rubyzip]: http://rubygems.org/gems/rubyzip
[git]: http://git-scm.org/
[INI]: http://en.wikipedia.org/wiki/INI_file
[applestrings]: http://developer.apple.com/documentation/Cocoa/Conceptual/LoadingResources/Strings/Strings.html
[androidstrings]: http://developer.android.com/guide/topics/resources/string-resource.html
[gettextpo]: http://www.gnu.org/savannah-checkouts/gnu/gettext/manual/html_node/PO-Files.html
[jquerylocalize]: https://github.com/coderifous/jquery-localize
[djangopo]: https://docs.djangoproject.com/en/dev/topics/i18n/translation/
[tizen]: https://developer.tizen.org/documentation/articles/localization
[flash]: http://help.adobe.com/en_US/FlashPlatform/reference/actionscript/3/mx/resources/IResourceManager.html#getString()
[printf]: https://en.wikipedia.org/wiki/Printf_format_string
