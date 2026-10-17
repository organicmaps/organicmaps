# -*- mode: sh -*-
# Sailjail profile of Organic Maps, applied with the permissions of organicmaps.desktop. Not in the Harbour
# package, which may have no profile of its own.

# Voice instructions: Speech Note when installed, which speaks with natural voices.
dbus-user.talk org.mkiol.Speech
# Opens Speech Note to download voices.
dbus-user.talk org.mkiol.dsnote
# Speech Note writes the audio there, to be played at the voice volume.
mkdir ${HOME}/.cache/org.mkiol/dsnote
whitelist ${HOME}/.cache/org.mkiol/dsnote
read-only ${HOME}/.cache/org.mkiol/dsnote
# Media players, Android apps too, pause while a voice instruction is spoken.
dbus-user.talk org.mpris.MediaPlayer2.*

# Otherwise speech synthesizers, used when one is installed. Sailfish OS has none of its own.
private-bin espeak-ng,espeak,mimic,flite,pico2wave
whitelist /usr/share/espeak-ng-data
read-only /usr/share/espeak-ng-data
whitelist /usr/share/espeak-data
read-only /usr/share/espeak-data
whitelist /usr/share/mimic
read-only /usr/share/mimic
whitelist /usr/share/pico
read-only /usr/share/pico
