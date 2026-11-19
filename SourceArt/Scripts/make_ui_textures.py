# Heart icons (0..4 quarters filled, filled clockwise from the top like Zelda) for the HUD.
import math, os
from PIL import Image, ImageDraw, ImageFilter
OUT = os.path.normpath(os.path.join(os.path.dirname(__file__), "..", "Generated", "UI"))
S = 128
def heart_mask():
    m = Image.new("L", (S, S), 0)
    d = ImageDraw.Draw(m)
    pts = []
    for i in range(400):
        t = i / 400 * math.tau
        x = 16 * math.sin(t) ** 3
        y = 13 * math.cos(t) - 5 * math.cos(2 * t) - 2 * math.cos(3 * t) - math.cos(4 * t)
        pts.append((S / 2 + x * 3.4, S / 2 - y * 3.4 + 4))
    d.polygon(pts, fill=255)
    return m
mask = heart_mask()
outline = mask.filter(ImageFilter.MaxFilter(9))
for q in range(5):
    img = Image.new("RGBA", (S, S), (0, 0, 0, 0))
    img.paste((255, 248, 232, 255), (0, 0), outline)                  # cream outline
    img.paste((40, 22, 22, 230), (0, 0), mask)                        # empty: dark
    if q:
        wedge = Image.new("L", (S, S), 0)
        ImageDraw.Draw(wedge).pieslice((-40, -40, S + 40, S + 40), -90, -90 + 90 * q, fill=255)
        fill = Image.fromarray(__import__("numpy").minimum(__import__("numpy").array(mask), __import__("numpy").array(wedge)))
        img.paste((214, 38, 52, 255), (0, 0), fill)                   # red
        hi = Image.new("L", (S, S), 0)
        ImageDraw.Draw(hi).ellipse((30, 30, 58, 50), fill=110)
        img.paste((255, 170, 170, 255), (0, 0), Image.fromarray(__import__("numpy").minimum(__import__("numpy").array(fill), __import__("numpy").array(hi))))
    img.save(os.path.join(OUT, "T_UI_Heart_%d.png" % q))
print("hearts saved to", OUT)
