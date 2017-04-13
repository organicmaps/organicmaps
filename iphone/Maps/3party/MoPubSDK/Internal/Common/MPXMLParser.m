//
//  MPXMLParser.m
//  MoPub
//
//  Copyright (c) 2015 MoPub. All rights reserved.
//

#import "MPXMLParser.h"

@interface MPXMLParser () <NSXMLParserDelegate>

@property (nonatomic) NSMutableArray *elementStack;
@property (nonatomic) NSMutableString *currentTextContent;
@property (nonatomic) NSError *parseError;

@end

////////////////////////////////////////////////////////////////////////////////////////////////////

@implementation MPXMLParser

- (instancetype)init
{
    if (self = [super init]) {
        _elementStack = [NSMutableArray array];

        // Create a "root" dictionary.
        [_elementStack addObject:[NSMutableDictionary dictionary]];

        _currentTextContent = [NSMutableString string];
    }
    return self;
}

- (NSDictionary *)dictionaryWithData:(NSData *)data error:(NSError *__autoreleasing *)error
{
    NSXMLParser *parser = [[NSXMLParser alloc] initWithData:data];
    [parser setDelegate:self];
    [parser parse];
    return self.elementStack[0];
}

#pragma mark - <NSXMLParserDelegate>

- (void)parser:(NSXMLParser *)parser didStartElement:(NSString *)elementName namespaceURI:(NSString *)namespaceURI qualifiedName:(NSString *)qName attributes:(NSDictionary *)attributeDict
{
    NSMutableDictionary *parentElement = [self.elementStack lastObject];
    NSMutableDictionary *currentElement = [NSMutableDictionary dictionary];
    [currentElement addEntriesFromDictionary:attributeDict];

    if (parentElement[elementName] && [parentElement[elementName] isKindOfClass:[NSArray class]]) {
        [parentElement[elementName] addObject:currentElement];
    } else if (parentElement[elementName]) {
        NSMutableDictionary *previousElement = parentElement[elementName];
        NSMutableArray *elementsArray = [NSMutableArray array];
        [elementsArray addObject:previousElement];
        [elementsArray addObject:currentElement];
        parentElement[elementName] = elementsArray;
    } else {
        parentElement[elementName] = currentElement;
    }

    [self.elementStack addObject:currentElement];
}

- (void)parser:(NSXMLParser *)parser didEndElement:(NSString *)elementName namespaceURI:(NSString *)namespaceURI qualifiedName:(NSString *)qName
{
    NSMutableDictionary *currentElement = [self.elementStack lastObject];
    NSString *trimmedContent = [self.currentTextContent stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceAndNewlineCharacterSet]];
    if ([trimmedContent length]) {
        currentElement[@"text"] = trimmedContent;
    }

    self.currentTextContent = [NSMutableString string];
    [self.elementStack removeLastObject];
}

- (void)parser:(NSXMLParser *)parser foundCharacters:(NSString *)string
{
    [self.currentTextContent appendString:string];
}

- (void)parser:(NSXMLParser *)parser parseErrorOccurred:(NSError *)parseError
{
    self.parseError = parseError;
}

@end
