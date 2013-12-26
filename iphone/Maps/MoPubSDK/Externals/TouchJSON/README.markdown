# TouchJSON HOWTO

## Introduction

TouchJSON is an Objective-C based parser and generator for JSON encoded data. TouchJSON compiles for Mac OS X and iOS devices (currently iPhone, iPad and iPod Touch).

It is based on Jonathan Wight's CocoaJSON code: <http://toxicsoftware.com/cocoajson/>

TouchJSON is part of the TouchCode "family" of open source software.

## Home

The main home page for touchcode is <http://touchcode.com/>

The main source repository for touchcode is on github at <http://github.com/TouchCode/TouchJSON>

## Author

The primary author is Jonathan Wight <http://toxicsoftware.com/> with several other people contributing bug fixes, patches and documentation. (Note: if you have contributed to TouchJSON and want to be listed here let Jonathan Wight know).

## What is JSON?

* <http://www.ietf.org/rfc/rfc4627.txt?number=4627>
* <http://www.json.org/>
* <http://en.wikipedia.org/wiki/JSON>

## License

TouchJSON (and all of TouchCode unless otherwise specified) is licensed under the MIT license.

## Support

There's a relatively low traffic mailing list hosted on Google Groups: <http://groups.google.com/group/touchcode-dev>

## Bug Reporting

File bugs on the github issue tracker <http://github.com/TouchCode/TouchJSON/issues> but please make sure that your JSON data is valid (see <http://www.jsonlint.com/> before filing bugs (of course if you've found a crash with TouchJSON's handling of invalid JSON feel free to file a bug or discuss on the mailing list).

## How to Help

There are many things you can do to help TouchJSON

* Find bugs and file issues
* Fix bugs
* File feature requests (We would _love_ to see more TouchJSON feature requests)
* Write more unit tests
* Help improve the documentation
* Help profile and optimise TouchJSON for speed and memory usage

<!--
## Donations

Click here to lend your support to TouchCode and make a donation:

[![Click here to lend your support to: TouchCode and make a donation at www.pledgie.com !][2]][1]

[1]: http://www.pledgie.com/campaigns/14660
[2]: http://www.pledgie.com/campaigns/14660.png?skin_name=chrome
-->

## How to use TouchJSON in your Cocoa or Cocoa Touch application.

TouchJSON is incredibly easy to use. Usually you can convert JSON data to and from a Cocoa representation in just a line of code.

### Dependencies

None! TouchJSON compiles on Mac OS X (note it does use ObjC-2) and iOS. It should compile on all versions of iOS to date.

Note that the demo, unit tests and bench-marking projects run on Mac OS X.

### Setup your project

Copy the source files within TouchJSON/Source to your project.
The easiest way is to open both projects in Xcode, then drag and drop.  Make sure to check "Copy items into destination groups folder (if needed)."

Be aware that the code in the Experimental subdirectory of Source is just that and may not have been extensively tested and/or have extra dependencies

### To transform JSON to objects

#### Basic usage

Put #import "CJSONDeserializer.h" in your source file.

	NSData *theJSONData = /* some JSON data */
	NSError *theError = nil;
	id theObject = [[CJSONDeserializer deserializer] deserialize:theJSONData error:&theError];}

This will convert an NSData object containing JSON into an object. The resulting object's class depends on the type of JSON data in question. If the object is NULL then deserialization has failed and you should check the error parameter.

The following, slightly more complex example shows how to convert an NSString containing a JSON dictionary into an NSDictionary:

	NSString *jsonString = @"yourJSONHere";
	NSData *jsonData = [jsonString dataUsingEncoding:NSUTF8StringEncoding];
	NSError *error = nil;
	NSDictionary *dictionary = [[CJSONDeserializer deserializer] deserializeAsDictionary:jsonData error:&error];}

This deserialization will fail if the JSON root object is not a dictionary. Again check the error.

#### Using an NSDictionary category

Put #include "NSDictionary_JSONExtensions.h" in your source file.

	NSString *theJSONString = @"{\"key\":\"value\"}";
	NSError *theError = NULL;
	NSDictionary *theDictionary = [NSDictionary dictionaryWithJSONString:theJSONString error:&theError];

This version of the code could be considered more convenient than the above former examples.

#### Avoiding NSNull values in output.

If your input JSON data contains null values these values will, by default, be represented by NSNull in your output ObjC objects. The following example shows you how to avoid NSNull values in your output:

	NSData *theJSONData = /* some JSON data */
	CJSONDeserializer *theDeserializer = [CJSONDeserializer deserializer];
	theDeserializer.nullObject = NULL;
	NSError *theError = nil;
	id theObject = [theDeserializer deserialize:theJSONData error:&theError];}

### To transform objects to JSON

Put #import "CJSONDataSerializer.h" in your file.

Here is a code sample:

	NSDictionary *dictionary = [NSDictionary dictionaryWithObject:@"b" forKey:@"a"];
	NSError *error = NULL;
	NSData *jsonData = [[CJSONSerializer serializer] serializeObject:dictionary error&error];

## Invalid JSON

If you think your JSON is valid but TouchJSON is failing to process it correctly (or if you think TouchJSON is producing invalid JSON) use the online JSON lint tool to validate your JSON: <http://www.jsonlint.com/>

It is especially important to validate your JSON before filing bugs.

## String encoding

TouchJSON will work with JSON with any string encoding supported by the Foundation framework. However internally TouchJSON prefers UTF8, so for performance purposes you should try to use UTF8 if at all possible.

## Date Formats

JSON doesn't specify a date encoding format. As such various methods are used. As such TouchJSON doesn't dictate which format you use. ISO 8601 style dates (with as much precession as needed) are recoemmended. See <http://en.wikipedia.org/wiki/ISO_8601>. You can use the CFilteringJSONSerializer class to automatically serialize Cocoa's NSDate objects into ISO-8601 strings
