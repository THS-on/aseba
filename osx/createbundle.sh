#/bin/bash -ex

DEST=$1
BUILD_DIR=$2
IDENTITY="$3"
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

if [[ $DEST =~ \.dmg$ ]]; then
    DMG="$1"
    DMG_DIR="$(mktemp -d)"
    DEST="$DMG_DIR/ThymioSuite.app"
fi


#hdiutil create -size 50m -fs HFS+ -volname Widget /path/to/save/widget.dmg




# Make the top Level Bundle Out of the Launcher Bundle
mkdir -p "$DEST"
cp -R "$BUILD_DIR"/thymio-launcher.app/* "$DEST"

realpath() {
    [[ $1 = /* ]] && echo "$1" || echo "$PWD/${1#./}"
}

add_to_group() {
    defaults write "$1" "com.apple.security.application-groups" -array "P97H86YL8K.ThymioSuite"
}

sign() {
    if [ -z "$IDENTITY" ]; then
        echo "Identity not provided, not signing"
    else
        codesign --verify --verbose -f -s "$IDENTITY" "$@"
    fi
}

#Make sure the launcher is retina ready
defaults write $(realpath "$DEST/Contents/Info.plist") NSPrincipalClass -string NSApplication
defaults write $(realpath "$DEST/Contents/Info.plist") NSHighResolutionCapable -string True
add_to_group $(realpath "$DEST/Contents/Info.plist")

APPS_DIR="$DEST/Contents/Applications"
BINUTILS_DIR="$DEST/Contents/MacOs"

#Copy the binaries we need
mkdir -p "$BINUTILS_DIR"
for binary in "thymio-device-manager" "asebacmd" "thymiownetconfig-cli"
do
    cp -r "$BUILD_DIR/$binary" "$BINUTILS_DIR"
done

#Copy the Applications
mkdir -p "$APPS_DIR/"
cp -R "${BUILD_DIR}/AsebaStudio.app" "$APPS_DIR/"
cp -R "${BUILD_DIR}/AsebaPlayground.app" "$APPS_DIR/"
cp -R "${BUILD_DIR}/ThymioVPLClassic.app" "$APPS_DIR/"

for app in "AsebaStudio" "AsebaPlayground" "ThymioVPLClassic"
do
    cp -r "${BUILD_DIR}/$app.app" "$APPS_DIR/"
    defaults write $(realpath "$APPS_DIR/$app.app/Contents/Info.plist") NSPrincipalClass -string NSApplication
    defaults write $(realpath "$APPS_DIR/$app.app/Contents/Info.plist") NSHighResolutionCapable -string True
    add_to_group $(realpath "$APPS_DIR/$app.app/Contents/Info.plist")
done

for app in "AsebaStudio" "AsebaPlayground" "ThymioVPLClassic"
do
    echo "Signing $APPS_DIR/$app.app/ with $DIR/inherited.entitlements"
    sign --deep $(realpath "$APPS_DIR/$app.app/")
    #--entitlements "$DIR/inherited.entitlements"
done

for fw in $(ls "$DEST/Contents/Frameworks")
do
    echo "Signing $DEST/Contents/Frameworks/$fw"
    sign --deep $(realpath "$DEST/Contents/Frameworks/$fw")
done

for plugin in $(find $DEST/Contents/PlugIns -name '*.dylib')
do
    echo "Signing $plugin"
    sign --deep $(realpath "$plugin")
done

for binary in "thymio-device-manager" "asebacmd" "thymiownetconfig-cli"
do
    echo "Signing $BINUTILS_DIR/$binary with $DIR/inherited.entitlements"
    sign --deep $(realpath "$BINUTILS_DIR/$binary")
    #--entitlements "$DIR/inherited.entitlements"
done

echo "Signing $DEST with $DIR/launcher.entitlements"
sign $(realpath "$BINUTILS_DIR/thymio-launcher")
#--entitlements "$DIR/launcher.entitlements"

if [ -n "$DMG" ]; then
    test -f "$1" && rm "$DMG"
    "$DIR/dmg/create-dmg" \
    --volname "Thymio Suite" \
    --volicon "$DIR/../menu/osx/launcher.icns" \
    --background "$DIR/background.png" \
    --window-pos 200 120 \
    --window-size 620 470 \
    --icon-size 100 \
    --icon "ThymioSuite.app" 100 300 \
    --hide-extension "ThymioSuite.app" \
    --app-drop-link 500 300 \
    "$DMG" \
    "$DMG_DIR/ThymioSuite.app"

    sign -f "$1"
fi