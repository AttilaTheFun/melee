# App icon

The editable vector source is `AppIcon.svg`. `Assets.xcassets/AppIcon.appiconset/AppIcon.png`
is its opaque 1024×1024 RGB export; Xcode generates the iPhone/iPad sizes.
The artwork fills the square, leaving corner masking to the operating system.

To regenerate from the repository root (requires librsvg and Pillow):

```sh
rsvg-convert native/apple/AppIcon.svg -o native/apple/Assets.xcassets/AppIcon.appiconset/AppIcon.png
python3 - <<'PY'
from PIL import Image
path = 'native/apple/Assets.xcassets/AppIcon.appiconset/AppIcon.png'
with Image.open(path) as image:
    image.convert('RGB').save(path)
PY
```
