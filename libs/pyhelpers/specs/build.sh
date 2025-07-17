#!/bin/bash
PROJECT=$1
VERSION=$2
RELEASE=$3
if [ -z "$PROJECT" ]; then
	echo "Specify project as first parameter"
	exit 1
fi
if [ -z "$VERSION" ]; then
	echo "Specify version as second parameter"
	exit 1
fi
if [ -z "$RELEASE" ]; then
	RELEASE=1
fi

basedir=$(dirname "$0")

PROJECT=$PROJECT VERSION=$VERSION RELEASE=$RELEASE rpmbuild -ba python35-mapsme-modules.spec
if [ $? -ne 0 ]; then
	echo "Build failed!"
	exit
fi

rsync -av $HOME/rpmbuild/RPMS/x86_64/python35-$PROJECT-$VERSION-$RELEASE.portal.el6.x86_64.rpm mapsme-team@pkg.corp.mail.ru::c6-mapsme-x64

echo "c6-mapsme-x64" | nc pkg.corp.mail.ru 12222 | grep -v '^* c'
echo
echo "$PROJECT packages version $VERSION-$RELEASE build done, ready to deploy"
