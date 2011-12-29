# Send given file in the body of email
# Call syntax: <script> <to> <subject> <file>

import smtplib
import sys
import string

if len(sys.argv) < 4:
  print "Usage: " + sys.argv[0] + " <to> <subject> <file>"
  exit(1)

FROM = 'Mona Lisa <mapswithme@gmail.com>'
TO = sys.argv[1]
SUBJECT = sys.argv[2]
FILE_TEXT = open(sys.argv[3], 'r').read()

BODY = string.join((
        "From: %s" % FROM,
        "To: %s" % TO,
        "Subject: %s" % SUBJECT ,
        "",
        FILE_TEXT
        ), "\r\n")

USER = 'mapswithme'
PASS = 'MapsWithMeP@ssw0rd'

# The actual mail send
server = smtplib.SMTP('smtp.gmail.com:587')
server.starttls()
server.login(USER, PASS)
server.sendmail(FROM, TO, BODY)
server.quit()
