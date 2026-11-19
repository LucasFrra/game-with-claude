# Procedural leaf-cluster texture (BOTW-like soft canopy cards): white leaves with a value variation in RGB,
# leaf shapes in alpha. Colour comes from the material tint. Output: SourceArt/Generated/Foliage/T_LeafCluster.png
import math, os, random
from PIL import Image, ImageDraw, ImageFilter
random.seed(7)
S = 1024
img = Image.new("RGBA", (S, S), (0, 0, 0, 0))
d = ImageDraw.Draw(img)

def leaf(cx, cy, length, width, angle, shade):
    pts = []
    for i in range(24):
        t = i / 23.0
        # pointed ellipse: width follows sin, tip sharpened
        w = width * math.sin(math.pi * t) ** 0.8
        pts.append((t * length, w))
    pts += [(x, -y) for x, y in reversed(pts)]
    ca, sa = math.cos(angle), math.sin(angle)
    poly = [(cx + x * ca - y * sa, cy + x * sa + y * ca) for x, y in pts]
    v = int(255 * shade)
    d.polygon(poly, fill=(v, v, v, 255))

# a round cluster of leaves, darker towards the centre, pointing outwards
cx = cy = S / 2
for i in range(260):
    r = (random.random() ** 0.6) * S * 0.36
    a = random.random() * math.tau
    x, y = cx + math.cos(a) * r, cy + math.sin(a) * r
    ang = a + random.uniform(-0.6, 0.6)
    shade = 0.72 + 0.28 * (r / (S * 0.36)) + random.uniform(-0.06, 0.06)
    leaf(x, y, random.uniform(70, 120), random.uniform(20, 32), ang, min(shade, 1.0))
img = img.filter(ImageFilter.SMOOTH)
out = os.path.join(os.path.dirname(__file__), "..", "Generated", "Foliage", "T_LeafCluster.png")
img.save(os.path.normpath(out))
print("saved", os.path.normpath(out))
