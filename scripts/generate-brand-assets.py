"""Generate the vector logo and raster app icons from one simple mark."""

from pathlib import Path

from PIL import Image, ImageDraw


ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "assets" / "brand"
OUT.mkdir(parents=True, exist_ok=True)

NAVY = "#142943"
CYAN = "#39c6d6"
WHITE = "#f6fbfd"

mark = f'''<rect x="8" y="8" width="496" height="496" rx="92" fill="{NAVY}"/>
<path d="M104 211 L210 139 L306 196 L409 112" fill="none" stroke="{CYAN}" stroke-width="25" stroke-linecap="round" stroke-linejoin="round"/>
<g fill="{WHITE}"><circle cx="104" cy="211" r="22"/><circle cx="210" cy="139" r="22"/><circle cx="306" cy="196" r="22"/></g>
<circle cx="409" cy="112" r="26" fill="{CYAN}"/>
<g fill="{WHITE}"><rect x="93" y="285" width="59" height="143" rx="9"/><rect x="160" y="285" width="59" height="143" rx="9"/><rect x="227" y="285" width="59" height="143" rx="9"/><rect x="294" y="285" width="59" height="143" rx="9"/><rect x="361" y="285" width="59" height="143" rx="9"/></g>
<rect x="227" y="285" width="59" height="143" rx="9" fill="{CYAN}"/>
<g fill="{NAVY}"><rect x="143" y="285" width="35" height="76" rx="5"/><rect x="277" y="285" width="35" height="76" rx="5"/><rect x="344" y="285" width="35" height="76" rx="5"/></g>'''

(OUT / "icon.svg").write_text(
    f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 512 512" role="img" aria-label="NSeqArpKeys app icon">\n{mark}\n</svg>\n',
    encoding="utf-8",
)
(OUT / "logo.svg").write_text(
    f'''<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 1390 512" role="img" aria-label="NSeqArpKeys logo">
<g>{mark}</g>
<text x="563" y="297" fill="{NAVY}" font-family="Arial,Helvetica,sans-serif" font-size="126" font-weight="700" letter-spacing="-4">NSeqArpKeys</text>
<text x="569" y="366" fill="#547088" font-family="Arial,Helvetica,sans-serif" font-size="38" letter-spacing="4">PATTERN INSTRUMENT</text>
</svg>\n''',
    encoding="utf-8",
)


def raster(size):
    scale = size / 512
    image = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    draw = ImageDraw.Draw(image)

    def box(coords):
        return tuple(round(value * scale) for value in coords)

    def radius(value):
        return round(value * scale)

    draw.rounded_rectangle(box((8, 8, 504, 504)), radius(92), fill=NAVY)
    points = [(104, 211), (210, 139), (306, 196), (409, 112)]
    draw.line([tuple(round(v * scale) for v in p) for p in points], fill=CYAN,
              width=radius(25), joint="curve")
    for x, y in points:
        r = 26 if x == 409 else 22
        draw.ellipse(box((x-r, y-r, x+r, y+r)), fill=CYAN if x == 409 else WHITE)
    for x in (93, 160, 227, 294, 361):
        draw.rounded_rectangle(box((x, 285, x+59, 428)), radius(9),
                               fill=CYAN if x == 227 else WHITE)
    for x in (143, 277, 344):
        draw.rounded_rectangle(box((x, 285, x+35, 361)), radius(5), fill=NAVY)
    return image


large = raster(2048).resize((512, 512), Image.Resampling.LANCZOS)
small = raster(512).resize((128, 128), Image.Resampling.LANCZOS)
large.save(OUT / "icon-512.png")
small.save(OUT / "icon-128.png")
large.save(OUT / "icon.ico", sizes=[(16, 16), (32, 32), (48, 48), (64, 64),
                                    (128, 128), (256, 256)])
