.. _tutorial:

********
Tutorial
********

.. highlight:: c

In this tutorial, we create a program that fetches the latest commits
of a repository in GitHub_ over the web. One of the response formats
supported by `GitHub API`_ is JSON, so the result can be parsed using
Jansson.

To stick to the the scope of this tutorial, we will only cover the the
parts of the program related to handling JSON data. For the best user
experience, the full source code is available:
:download:`github_commits.c`. To compile it (on Unix-like systems with
gcc), use the following command::

    gcc -o github_commits github_commits.c -ljansson -lcurl

libcurl_ is used to communicate over the web, so it is required to
compile the program.

The command line syntax is::

    github_commits USER REPOSITORY

``USER`` is a GitHub user ID and ``REPOSITORY`` is the repository
name. Please note that the GitHub API is rate limited, so if you run
the program too many times within a short period of time, the sever
starts to respond with an error.

.. _GitHub: http://github.com/
.. _GitHub API: http://develop.github.com/
.. _libcurl: http://curl.haxx.se/


.. _tutorial-github-commits-api:

The GitHub Commits API
======================

The `GitHub commits API`_ is used by sending HTTP requests to URLs
starting with ``http://github.com/api/v2/json/commits/``. Our program
only lists the latest commits, so the rest of the URL is
``list/USER/REPOSITORY/BRANCH``, where ``USER``, ``REPOSITORY`` and
``BRANCH`` are the GitHub user ID, the name of the repository, and the
name of the branch whose commits are to be listed, respectively.

GitHub responds with a JSON object of the following form:

.. code-block:: none

    {
        "commits": [
            {
                "id": "<the commit ID>",
                "message": "<the commit message>",
                <more fields, not important to this tutorial>
            },
            {
                "id": "<the commit ID>",
                "message": "<the commit message>",
                <more fields, not important to this tutorial>
            },
            <more commits...>
        ]
    }

In our program, the HTTP request is sent using the following
function::

    static char *request(const char *url);

It takes the URL as a parameter, preforms a HTTP GET request, and
returns a newly allocated string that contains the response body. If
the request fails, an error message is printed to stderr and the
return value is *NULL*. For full details, refer to :download:`the code
<github_commits.c>`, as the actual implementation is not important
here.

.. _GitHub commits API: http://develop.github.com/p/commits.html

.. _tutorial-the-program:

The Program
===========

First the includes::

    #include <string.h>
    #include <jansson.h>

Like all the programs using Jansson, we need to include
:file:`jansson.h`.

The following definitions are used to build the GitHub commits API
request URL::

   #define URL_FORMAT   "http://github.com/api/v2/json/commits/list/%s/%s/master"
   #define URL_SIZE     256

The following function is used when formatting the result to find the
first newline in the commit message::

    /* Return the offset of the first newline in text or the length of
       text if there's no newline */
    static int newline_offset(const char *text)
    {
        const char *newline = strchr(text, '\n');
        if(!newline)
            return strlen(text);
        else
            return (int)(newline - text);
    }

The main function follows. In the beginning, we first declare a bunch
of variables and check the command line parameters::

    size_t i;
    char *text;
    char url[URL_SIZE];

    json_t *root;
    json_error_t error;
    json_t *commits;

    if(argc != 3)
    {
        fprintf(stderr, "usage: %s USER REPOSITORY\n\n", argv[0]);
        fprintf(stderr, "List commits at USER's REPOSITORY.\n\n");
        return 2;
    }

Then we build the request URL using the user and repository names
given as command line parameters::

    snprintf(url, URL_SIZE, URL_FORMAT, argv[1], argv[2]);

This uses the ``URL_SIZE`` and ``URL_FORMAT`` constants defined above.
Now we're ready to actually request the JSON data over the web::

    text = request(url);
    if(!text)
        return 1;

If an error occurs, our function ``request`` prints the error and
returns *NULL*, so it's enough to just return 1 from the main
function.

Next we'll call :func:`json_loads()` to decode the JSON text we got
as a response::

    root = json_loads(text, 0, &error);
    free(text);

    if(!root)
    {
        fprintf(stderr, "error: on line %d: %s\n", error.line, error.text);
        return 1;
    }

We don't need the JSON text anymore, so we can free the ``text``
variable right after decoding it. If :func:`json_loads()` fails, it
returns *NULL* and sets error information to the :type:`json_error_t`
structure given as the second parameter. In this case, our program
prints the error information out and returns 1 from the main function.

Now we're ready to extract the data out of the decoded JSON response.
The structure of the response JSON was explained in section
:ref:`tutorial-github-commits-api`.

First, we'll extract the ``commits`` array from the JSON response::

    commits = json_object_get(root, "commits");
    if(!json_is_array(commits))
    {
        fprintf(stderr, "error: commits is not an array\n");
        return 1;
    }

This is the array that contains objects describing latest commits in
the repository. We check that the returned value really is an array.
If the key ``commits`` doesn't exist, :func:`json_object_get()`
returns *NULL*, but :func:`json_is_array()` handles this case, too.

Then we proceed to loop over all the commits in the array::

    for(i = 0; i < json_array_size(commits); i++)
    {
        json_t *commit, *id, *message;
        const char *message_text;

        commit = json_array_get(commits, i);
        if(!json_is_object(commit))
        {
            fprintf(stderr, "error: commit %d is not an object\n", i + 1);
            return 1;
        }
    ...

The function :func:`json_array_size()` returns the size of a JSON
array. First, we again declare some variables and then extract the
i'th element of the ``commits`` array using :func:`json_array_get()`.
We also check that the resulting value is a JSON object.

Next we'll extract the commit ID and commit message, and check that
they both are JSON strings::

        id = json_object_get(commit, "id");
        if(!json_is_string(id))
        {
            fprintf(stderr, "error: commit %d: id is not a string\n", i + 1);
            return 1;
        }

        message = json_object_get(commit, "message");
        if(!json_is_string(message))
        {
            fprintf(stderr, "error: commit %d: message is not a string\n", i + 1);
            return 1;
        }
    ...

And finally, we'll print the first 8 characters of the commit ID and
the first line of the commit message. A C-style string is extracted
from a JSON string using :func:`json_string_value()`::

        message_text = json_string_value(message);
        printf("%.8s %.*s\n",
               json_string_value(id),
               newline_offset(message_text),
               message_text);
    }

After sending the HTTP request, we decoded the JSON text using
:func:`json_loads()`, remember? It returns a *new reference* to the
JSON value it decodes. When we're finished with the value, we'll need
to decrease the reference count using :func:`json_decref()`. This way
Jansson can release the resources::

    json_decref(root);
    return 0;

For a detailed explanation of reference counting in Jansson, see
:ref:`apiref-reference-count` in :ref:`apiref`.

The program's ready, let's test it and view the latest commits in
Jansson's repository::

    $ ./github_commits akheron jansson
    86dc1d62 Fix indentation
    b67e130f json_dumpf: Document the output shortage on error
    4cd77771 Enhance handling of circular references
    79009e62 json_dumps: Close the strbuffer if dumping fails
    76999799 doc: Fix a small typo in apiref
    22af193a doc/Makefile.am: Remove *.pyc in clean
    951d091f Make integer, real and string mutable
    185e107d Don't use non-portable asprintf()
    ca7703fb Merge branch '1.0'
    12cd4e8c jansson 1.0.4
    <etc...>


Conclusion
==========

In this tutorial, we implemented a program that fetches the latest
commits of a GitHub repository using the GitHub commits API. Jansson
was used to decode the JSON response and to extract the commit data.

This tutorial only covered a small part of Jansson. For example, we
did not create or manipulate JSON values at all. Proceed to
:ref:`apiref` to explore all features of Jansson.
