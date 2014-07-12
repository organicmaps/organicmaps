/* Copyright (c) 2013 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

//
//  GTLQueryPlus.h
//

// ----------------------------------------------------------------------------
// NOTE: This file is generated from Google APIs Discovery Service.
// Service:
//   Google+ API (plus/v1)
// Description:
//   The Google+ API enables developers to build on top of the Google+ platform.
// Documentation:
//   https://developers.google.com/+/api/
// Classes:
//   GTLQueryPlus (12 custom class methods, 15 custom properties)

#if GTL_BUILT_AS_FRAMEWORK
  #import "GTL/GTLQuery.h"
#else
  #import "GTLQuery.h"
#endif

@class GTLPlusMoment;

@interface GTLQueryPlus : GTLQuery

//
// Parameters valid on all methods.
//

// Selector specifying which fields to include in a partial response.
@property (copy) NSString *fields;

//
// Method-specific parameters; see the comments below for more information.
//
@property (copy) NSString *activityId;
@property (copy) NSString *collection;
@property (copy) NSString *commentId;
@property (assign) BOOL debug;
// identifier property maps to 'id' in JSON (to avoid Objective C's 'id').
@property (copy) NSString *identifier;
@property (copy) NSString *language;
@property (assign) NSUInteger maxResults;
@property (copy) NSString *orderBy;
@property (copy) NSString *pageToken;
@property (copy) NSString *query;
@property (copy) NSString *sortOrder;
@property (copy) NSString *targetUrl;
@property (copy) NSString *type;
@property (copy) NSString *userId;

#pragma mark -
#pragma mark "activities" methods
// These create a GTLQueryPlus object.

// Method: plus.activities.get
// Get an activity.
//  Required:
//   activityId: The ID of the activity to get.
//  Authorization scope(s):
//   kGTLAuthScopePlusLogin
//   kGTLAuthScopePlusMe
// Fetches a GTLPlusActivity.
+ (id)queryForActivitiesGetWithActivityId:(NSString *)activityId;

// Method: plus.activities.list
// List all of the activities in the specified collection for a particular user.
//  Required:
//   userId: The ID of the user to get activities for. The special value "me"
//     can be used to indicate the authenticated user.
//   collection: The collection of activities to list.
//      kGTLPlusCollectionPublic: All public activities created by the specified
//        user.
//  Optional:
//   maxResults: The maximum number of activities to include in the response,
//     which is used for paging. For any response, the actual number returned
//     might be less than the specified maxResults. (1..100, default 20)
//   pageToken: The continuation token, which is used to page through large
//     result sets. To get the next page of results, set this parameter to the
//     value of "nextPageToken" from the previous response.
//  Authorization scope(s):
//   kGTLAuthScopePlusLogin
//   kGTLAuthScopePlusMe
// Fetches a GTLPlusActivityFeed.
+ (id)queryForActivitiesListWithUserId:(NSString *)userId
                            collection:(NSString *)collection;

// Method: plus.activities.search
// Search public activities.
//  Required:
//   query: Full-text search query string.
//  Optional:
//   language: Specify the preferred language to search with. See search
//     language codes for available values. (Default en-US)
//   maxResults: The maximum number of activities to include in the response,
//     which is used for paging. For any response, the actual number returned
//     might be less than the specified maxResults. (1..20, default 10)
//   orderBy: Specifies how to order search results. (Default
//     kGTLPlusOrderByRecent)
//      kGTLPlusOrderByBest: Sort activities by relevance to the user, most
//        relevant first.
//      kGTLPlusOrderByRecent: Sort activities by published date, most recent
//        first.
//   pageToken: The continuation token, which is used to page through large
//     result sets. To get the next page of results, set this parameter to the
//     value of "nextPageToken" from the previous response. This token can be of
//     any length.
//  Authorization scope(s):
//   kGTLAuthScopePlusMe
// Fetches a GTLPlusActivityFeed.
+ (id)queryForActivitiesSearchWithQuery:(NSString *)query;

#pragma mark -
#pragma mark "comments" methods
// These create a GTLQueryPlus object.

// Method: plus.comments.get
// Get a comment.
//  Required:
//   commentId: The ID of the comment to get.
//  Authorization scope(s):
//   kGTLAuthScopePlusMe
// Fetches a GTLPlusComment.
+ (id)queryForCommentsGetWithCommentId:(NSString *)commentId;

// Method: plus.comments.list
// List all of the comments for an activity.
//  Required:
//   activityId: The ID of the activity to get comments for.
//  Optional:
//   maxResults: The maximum number of comments to include in the response,
//     which is used for paging. For any response, the actual number returned
//     might be less than the specified maxResults. (0..500, default 20)
//   pageToken: The continuation token, which is used to page through large
//     result sets. To get the next page of results, set this parameter to the
//     value of "nextPageToken" from the previous response.
//   sortOrder: The order in which to sort the list of comments. (Default
//     kGTLPlusSortOrderAscending)
//      kGTLPlusSortOrderAscending: Sort oldest comments first.
//      kGTLPlusSortOrderDescending: Sort newest comments first.
//  Authorization scope(s):
//   kGTLAuthScopePlusMe
// Fetches a GTLPlusCommentFeed.
+ (id)queryForCommentsListWithActivityId:(NSString *)activityId;

#pragma mark -
#pragma mark "moments" methods
// These create a GTLQueryPlus object.

// Method: plus.moments.insert
// Record a moment representing a user's activity such as making a purchase or
// commenting on a blog.
//  Required:
//   userId: The ID of the user to record activities for. The only valid values
//     are "me" and the ID of the authenticated user.
//   collection: The collection to which to write moments.
//      kGTLPlusCollectionVault: The default collection for writing new moments.
//  Optional:
//   debug: Return the moment as written. Should be used only for debugging.
//  Authorization scope(s):
//   kGTLAuthScopePlusLogin
// Fetches a GTLPlusMoment.
+ (id)queryForMomentsInsertWithObject:(GTLPlusMoment *)object
                               userId:(NSString *)userId
                           collection:(NSString *)collection;

// Method: plus.moments.list
// List all of the moments for a particular user.
//  Required:
//   userId: The ID of the user to get moments for. The special value "me" can
//     be used to indicate the authenticated user.
//   collection: The collection of moments to list.
//      kGTLPlusCollectionVault: All moments created by the requesting
//        application for the authenticated user.
//  Optional:
//   maxResults: The maximum number of moments to include in the response, which
//     is used for paging. For any response, the actual number returned might be
//     less than the specified maxResults. (1..100, default 20)
//   pageToken: The continuation token, which is used to page through large
//     result sets. To get the next page of results, set this parameter to the
//     value of "nextPageToken" from the previous response.
//   targetUrl: Only moments containing this targetUrl will be returned.
//   type: Only moments of this type will be returned.
//  Authorization scope(s):
//   kGTLAuthScopePlusLogin
// Fetches a GTLPlusMomentsFeed.
+ (id)queryForMomentsListWithUserId:(NSString *)userId
                         collection:(NSString *)collection;

// Method: plus.moments.remove
// Delete a moment.
//  Required:
//   identifier: The ID of the moment to delete.
//  Authorization scope(s):
//   kGTLAuthScopePlusLogin
+ (id)queryForMomentsRemoveWithIdentifier:(NSString *)identifier;

#pragma mark -
#pragma mark "people" methods
// These create a GTLQueryPlus object.

// Method: plus.people.get
// Get a person's profile. If your app uses scope
// https://www.googleapis.com/auth/plus.login, this method is guaranteed to
// return ageRange and language.
//  Required:
//   userId: The ID of the person to get the profile for. The special value "me"
//     can be used to indicate the authenticated user.
//  Authorization scope(s):
//   kGTLAuthScopePlusLogin
//   kGTLAuthScopePlusMe
// Fetches a GTLPlusPerson.
+ (id)queryForPeopleGetWithUserId:(NSString *)userId;

// Method: plus.people.list
// List all of the people in the specified collection.
//  Required:
//   userId: Get the collection of people for the person identified by the ID or
//     use "me" to indiciated the authenticated user.
//   collection: The collection of people to list.
//      kGTLPlusCollectionVisible: The list of people who this user has added to
//        one or more circles, limited to the circles visible to the requesting
//        application.
//  Optional:
//   maxResults: The maximum number of people to include in the response, which
//     is used for paging. For any response, the actual number returned might be
//     less than the specified maxResults. (1..100, default 100)
//   orderBy: The order to return people in.
//      kGTLPlusOrderByAlphabetical: Order the people by their display name.
//      kGTLPlusOrderByBest: Order people based on the relevence to the viewer.
//   pageToken: The continuation token, which is used to page through large
//     result sets. To get the next page of results, set this parameter to the
//     value of "nextPageToken" from the previous response.
//  Authorization scope(s):
//   kGTLAuthScopePlusLogin
// Fetches a GTLPlusPeopleFeed.
+ (id)queryForPeopleListWithUserId:(NSString *)userId
                        collection:(NSString *)collection;

// Method: plus.people.listByActivity
// List all of the people in the specified collection for a particular activity.
//  Required:
//   activityId: The ID of the activity to get the list of people for.
//   collection: The collection of people to list.
//      kGTLPlusCollectionPlusoners: List all people who have +1'd this
//        activity.
//      kGTLPlusCollectionResharers: List all people who have reshared this
//        activity.
//  Optional:
//   maxResults: The maximum number of people to include in the response, which
//     is used for paging. For any response, the actual number returned might be
//     less than the specified maxResults. (1..100, default 20)
//   pageToken: The continuation token, which is used to page through large
//     result sets. To get the next page of results, set this parameter to the
//     value of "nextPageToken" from the previous response.
//  Authorization scope(s):
//   kGTLAuthScopePlusMe
// Fetches a GTLPlusPeopleFeed.
+ (id)queryForPeopleListByActivityWithActivityId:(NSString *)activityId
                                      collection:(NSString *)collection;

// Method: plus.people.search
// Search all public profiles.
//  Required:
//   query: Specify a query string for full text search of public text in all
//     profiles.
//  Optional:
//   language: Specify the preferred language to search with. See search
//     language codes for available values. (Default en-US)
//   maxResults: The maximum number of people to include in the response, which
//     is used for paging. For any response, the actual number returned might be
//     less than the specified maxResults. (1..20, default 10)
//   pageToken: The continuation token, which is used to page through large
//     result sets. To get the next page of results, set this parameter to the
//     value of "nextPageToken" from the previous response. This token can be of
//     any length.
//  Authorization scope(s):
//   kGTLAuthScopePlusMe
// Fetches a GTLPlusPeopleFeed.
+ (id)queryForPeopleSearchWithQuery:(NSString *)query;

@end
