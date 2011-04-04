#!/opt/local/bin/python
import hashlib
import json
import os
import re
import string
import sys
from BeautifulSoup import BeautifulSoup

def RemoveEmptyTags(soup):
  # Removing free tags can make other tags free, so we do it several times in a loop.
  for i in range(1, 5):
    [x.extract() for x in soup.findAll(lambda tag: tag.name in ['p', 'div', 'h2']
                                       and tag.find(True) is None
                                       and (tag.string is None or tag.string.strip() == ''))]
    
def ProcessArticle(article):
  soup = BeautifulSoup(article)
  [x.extract() for x in soup.findAll(id = 'top1')]
  [x.extract() for x in soup.findAll(id = 'toolbar_top')]
  [x.extract() for x in soup.findAll(id = 'siteNotice')]
  [x.extract() for x in soup.findAll(id = 'p-toc')]
  [x.extract() for x in soup.findAll(id = 'catlinks')]
  [x.extract() for x in soup.findAll('div', 'search-container')]
  [x.extract() for x in soup.findAll('div', 'printfooter')]
  [x.extract() for x in soup.findAll('div', 'visualClear')]
  [x.extract() for x in soup.findAll('script')]
  [x.extract() for x in soup.findAll('ul', 'individual')]
  
  for notice in soup.findAll('a', href='http://m.wikitravel.org/en/Wikitravel:Plunge_forward'):
    noticeDiv = notice.findParent('div')
    if noticeDiv:
      noticeDiv.extract()

  # Remove empty tags. This is especially needed for Get_out section, since it containts the footer.
  RemoveEmptyTags(soup)
  sections = [tag['id'][8:] for tag in soup.findAll(id = re.compile('section-.*'))]
  for section in sections:
    if soup.find(id = 'section-' + section) is None:
      [x.extract() for x in soup.find(id = 'button-' + section).findParent('h2')]
  RemoveEmptyTags(soup)

  s = str(soup)
  s = s.replace('toggleShowHide', 'tg')
  s = re.search('<body>(.*)</body>', s, re.UNICODE | re.MULTILINE | re.DOTALL).group(1)
  return s

for i, line in enumerate(sys.stdin):
  (url, title, fileName) = json.loads(line)
  outFileName = fileName + '.article'
  if os.path.exists(outFileName):
    sys.stderr.write('Skipping existing {0} {1}\n'.format(i, fileName))
  else:
    sys.stderr.write('Processing {0} {1}\n'.format(i, fileName))
    fin = open(fileName, 'r')
    article = ProcessArticle(fin.read())
    fin.close()

    fout = open(outFileName, 'w')
    fout.write(article)
    fout.close()


