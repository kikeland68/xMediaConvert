#!/bin/sh
install_name_tool -change \
 "@loader_path/../Frameworks/Sparkle.framework/Versions/A/Sparkle" \
 "@executable_path/../Frameworks/Sparkle.framework/Versions/A/Sparkle" \
 "$BUILT_PRODUCTS_DIR/$EXECUTABLE_PATH"
