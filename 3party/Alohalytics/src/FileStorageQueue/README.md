# ClientFileStorage

Appends messages to the current file, maintains certain file size and max. age, renames the current file to a finalized one, notifies file listener thread upon newly added finalized files available.

The receiving end of the events queue (see CachingMessageQueue), that collects raw events to be sent to the server.
