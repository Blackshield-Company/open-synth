#!/usr/bin/env bash
# Open Synth installer for Linux (per-user, no root needed).
#   ./install.sh            install VST3 -> ~/.vst3, CLAP -> ~/.clap,
#                           standalone -> ~/.local/opt/open-synth (+ launcher)
#   ./install.sh --uninstall
set -euo pipefail
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
VST3_DIR="${VST3_DIR:-$HOME/.vst3}"
CLAP_DIR="${CLAP_DIR:-$HOME/.clap}"
APP_DIR="${APP_DIR:-$HOME/.local/opt/open-synth}"
BIN_DIR="$HOME/.local/bin"
DESKTOP_DIR="${XDG_DATA_HOME:-$HOME/.local/share}/applications"
ICON_DIR="${XDG_DATA_HOME:-$HOME/.local/share}/icons/hicolor/512x512/apps"

if [[ "${1:-}" == "--uninstall" ]]; then
  rm -rf "$VST3_DIR/Open Synth.vst3" "$CLAP_DIR/Open Synth.clap" "$CLAP_DIR/samples" "$APP_DIR"
  rm -f "$BIN_DIR/open-synth" "$DESKTOP_DIR/open-synth.desktop" "$ICON_DIR/open-synth.png"
  echo "Open Synth uninstalled."
  exit 0
fi

mkdir -p "$VST3_DIR" "$CLAP_DIR" "$APP_DIR" "$BIN_DIR" "$DESKTOP_DIR" "$ICON_DIR"

echo "Installing VST3 -> $VST3_DIR"
rm -rf "$VST3_DIR/Open Synth.vst3"
cp -R "$HERE/VST3/Open Synth.vst3" "$VST3_DIR/"

echo "Installing CLAP -> $CLAP_DIR"
cp -f "$HERE/CLAP/Open Synth.clap" "$CLAP_DIR/"
if [[ -d "$HERE/CLAP/samples" ]]; then
  # The CLAP looks for samples/ beside its binary.
  rm -rf "$CLAP_DIR/samples"; cp -R "$HERE/CLAP/samples" "$CLAP_DIR/samples"
fi

echo "Installing standalone -> $APP_DIR"
rm -rf "$APP_DIR"; mkdir -p "$APP_DIR"
cp -R "$HERE/Standalone/." "$APP_DIR/"
ln -sf "$APP_DIR/Open Synth" "$BIN_DIR/open-synth"
[[ -f "$HERE/open-synth.png" ]] && cp -f "$HERE/open-synth.png" "$ICON_DIR/open-synth.png"
cat > "$DESKTOP_DIR/open-synth.desktop" <<DESK
[Desktop Entry]
Type=Application
Name=Open Synth
Comment=Open-source synthesizer
Exec="$APP_DIR/Open Synth"
Icon=open-synth
Categories=AudioVideo;Audio;Music;
DESK
echo "Done. Rescan plugins in your DAW; run 'open-synth' for the standalone."
