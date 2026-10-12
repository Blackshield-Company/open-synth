#!/usr/bin/env bash
# Build "Open Synth-<ver>-macOS.dmg" containing a component .pkg that installs
# AU, VST3, CLAP (system plug-in folders) and the standalone app.
# Signing / notarisation run only when the matching env vars are set:
#   APPLE_DEVELOPER_ID_APPLICATION   e.g. "Developer ID Application: Name (TEAMID)"
#   APPLE_DEVELOPER_ID_INSTALLER     e.g. "Developer ID Installer: Name (TEAMID)"
#   APPLE_ID, APPLE_TEAM_ID, APPLE_APP_PASSWORD   (notarytool credentials)
# Usage: build_pkg.sh <version> <artefacts dir> <out dir>
set -euo pipefail
VERSION="$1"; ART="$2"; OUT="$3"
WORK="$(mktemp -d)"; mkdir -p "$OUT"
ID=com.synthalorian.opensynth
APP_SIGN="${APPLE_DEVELOPER_ID_APPLICATION:-}"
PKG_SIGN="${APPLE_DEVELOPER_ID_INSTALLER:-}"

sign_bundle() {
  if [[ -n "$APP_SIGN" ]]; then
    codesign --force --deep --options runtime --timestamp --sign "$APP_SIGN" "$1"
  else
    echo "  (unsigned) $1"
  fi
}

component() { # name src-bundle install-location
  local name="$1" src="$2" loc="$3"
  [[ -e "$src" ]] || { echo "skip $name (not built)"; return; }
  mkdir -p "$WORK/root-$name"
  cp -R "$src" "$WORK/root-$name/"
  sign_bundle "$WORK/root-$name/$(basename "$src")"
  pkgbuild --root "$WORK/root-$name" --identifier "$ID.$name" --version "$VERSION" \
    --install-location "$loc" "$WORK/$name.pkg"
  CHOICES+="<choice id=\"$name\" title=\"$4\"><pkg-ref id=\"$ID.$name\"/></choice>"
  OUTLINE+="<line choice=\"$name\"/>"
  REFS+="<pkg-ref id=\"$ID.$name\" version=\"$VERSION\">$name.pkg</pkg-ref>"
}

CHOICES=""; OUTLINE=""; REFS=""
component app  "$ART/Standalone/Open Synth.app" /Applications "Standalone app"
component au   "$ART/AU/Open Synth.component"   /Library/Audio/Plug-Ins/Components "Audio Unit"
component vst3 "$ART/VST3/Open Synth.vst3"      /Library/Audio/Plug-Ins/VST3 "VST3"
component clap "$ART/CLAP/Open Synth.clap"      /Library/Audio/Plug-Ins/CLAP "CLAP"

cat > "$WORK/distribution.xml" <<XML
<?xml version="1.0" encoding="utf-8"?>
<installer-gui-script minSpecVersion="2">
  <title>Open Synth $VERSION</title>
  <options customize="allow" require-scripts="false" hostArchitectures="arm64,x86_64"/>
  <choices-outline>$OUTLINE</choices-outline>
  $CHOICES
  $REFS
</installer-gui-script>
XML

PKG="$WORK/dmg/Open Synth $VERSION.pkg"; mkdir -p "$WORK/dmg"
if [[ -n "$PKG_SIGN" ]]; then
  productbuild --distribution "$WORK/distribution.xml" --package-path "$WORK" --sign "$PKG_SIGN" "$PKG"
else
  productbuild --distribution "$WORK/distribution.xml" --package-path "$WORK" "$PKG"
fi
cp "$(dirname "$0")/../../LICENSE" "$WORK/dmg/LICENSE.txt"

DMG="$OUT/open-synth-$VERSION-macos-universal.dmg"
hdiutil create -volname "Open Synth $VERSION" -srcfolder "$WORK/dmg" -ov -format UDZO "$DMG"
[[ -n "$APP_SIGN" ]] && codesign --force --timestamp --sign "$APP_SIGN" "$DMG"

if [[ -n "$APP_SIGN" && -n "${APPLE_ID:-}" && -n "${APPLE_TEAM_ID:-}" && -n "${APPLE_APP_PASSWORD:-}" ]]; then
  xcrun notarytool submit "$DMG" --apple-id "$APPLE_ID" --team-id "$APPLE_TEAM_ID" \
    --password "$APPLE_APP_PASSWORD" --wait
  xcrun stapler staple "$DMG"
else
  echo "Notarisation skipped (credentials not configured)."
fi
echo "Built $DMG"
