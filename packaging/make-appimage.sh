#!/usr/bin/env bash
#
# Builds an AppImage from an installed enzo tree.
#
#   make-appimage.sh <install-prefix> <output-file>
#
# The install tree is already self contained, with every library beside the
# binary and a relative rpath reaching it, so the AppDir is that tree plus the
# three files the format expects at its root.

set -euo pipefail

PREFIX=$1
OUTPUT=$2

APPDIR=$(mktemp -d)
trap 'rm -rf "$APPDIR"' EXIT

cp -a "$PREFIX/." "$APPDIR/"

cp "$APPDIR/share/applications/org.enzosoftware.Enzo.desktop" "$APPDIR/"
cp "$APPDIR/share/icons/hicolor/scalable/apps/org.enzosoftware.Enzo.svg" "$APPDIR/"

cat > "$APPDIR/AppRun" <<'RUN'
#!/usr/bin/env bash
HERE=$(dirname "$(readlink -f "$0")")
exec "$HERE/enzo/bin/enzoGui" "$@"
RUN
chmod +x "$APPDIR/AppRun"

mkdir -p "$(dirname "$OUTPUT")"
appimagetool --no-appstream "$APPDIR" "$OUTPUT"
