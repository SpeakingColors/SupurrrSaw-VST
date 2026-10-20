#!/usr/bin/env python3
"""Studio 87 SupurrSaw: turns the mockup page (ui/supurrsaw-playground.html) into the page the plugin shows.

- adds a doctype (the page is published without one)
- replaces the Google Fonts links with the bundled font files (offline, nothing is fetched at run time)
- adds the plugin bridge (plugin/web/shim.js) before the page script
Usage: make_plugin_page.py <page.html> <out.html>
"""
import base64, pathlib, re, sys

root = pathlib.Path(__file__).resolve().parent.parent
src = pathlib.Path(sys.argv[1]).read_text(encoding="utf-8")
out = pathlib.Path(sys.argv[2])

faces = [
    ("Fredoka", 500, "fredoka-latin-500-normal.woff2"), ("Fredoka", 600, "fredoka-latin-600-normal.woff2"),
    ("IBM Plex Mono", 400, "ibm-plex-mono-latin-400-normal.woff2"), ("IBM Plex Mono", 500, "ibm-plex-mono-latin-500-normal.woff2"),
    ("IBM Plex Mono", 600, "ibm-plex-mono-latin-600-normal.woff2"),
]
css = ""
for fam, w, fn in faces:
    b = base64.b64encode((root / "web" / "fonts" / fn).read_bytes()).decode()
    css += "@font-face{font-family:'%s';font-weight:%d;font-style:normal;font-display:block;src:url(data:font/woff2;base64,%s) format('woff2')}\n" % (fam, w, b)

n_before = len(src)
src, k = re.subn(r'<link rel="preconnect" href="https://fonts\.googleapis\.com">\s*<link href="https://fonts\.googleapis\.com/[^"]*" rel="stylesheet">', "", src)
if k != 1:
    sys.exit("font links not found in the page; update make_plugin_page.py")
src = src.replace("<small>Playground</small>", "<small>by Studio 87</small>", 1)
src = re.sub(r"<title>[^<]*</title>", "<title>SupurrSaw by Studio 87</title>", src, count=1)
shim = (root / "web" / "shim.js").read_text(encoding="utf-8")
import json
seed = json.dumps(json.loads((root / "web" / "seed_presets.json").read_text(encoding="utf-8")), separators=(",", ":"))  # ascii only
shim = "window.__SEED_PRESETS=" + seed + ";\n" + shim
head = ('<!doctype html><html lang="en"><head><meta charset="utf-8">'
        '<meta name="viewport" content="width=device-width,initial-scale=1">'
        '<style>html,body{margin:0;padding:0;overflow:hidden}\n' + css + '</style>'
        '<script>' + shim + '</script>')
html = head + src + "</body></html>"
if "googleapis" in html or "gstatic" in html:
    sys.exit("page still points at a font host")
# WebKitGTK ignores the Content-Type header of custom scheme responses and falls back to windows-1252, so the page
# must be pure ASCII: entities in markup, \uXXXX in scripts, CSS escapes in styles.
def esc_js(t):
    return "".join(c if ord(c) < 128 else ("\\u%04x" % ord(c) if ord(c) < 0x10000 else "\\u%04x\\u%04x" % (0xD800 + ((ord(c) - 0x10000) >> 10), 0xDC00 + ((ord(c) - 0x10000) & 0x3FF))) for c in t)
def esc_css(t):
    return "".join(c if ord(c) < 128 else "\\%x " % ord(c) for c in t)
def esc_html(t):
    return "".join(c if ord(c) < 128 else "&#x%x;" % ord(c) for c in t)
parts, pos = [], 0
for m in re.finditer(r"(<script\b[^>]*>)(.*?)(</script>)|(<style\b[^>]*>)(.*?)(</style>)", html, re.S):
    parts.append(esc_html(html[pos:m.start()]))
    if m.group(1):
        parts.append(m.group(1) + esc_js(m.group(2)) + m.group(3))
    else:
        parts.append(m.group(4) + esc_css(m.group(5)) + m.group(6))
    pos = m.end()
parts.append(esc_html(html[pos:]))
html = "".join(parts)
assert all(ord(c) < 128 for c in html)
out.write_text(html, encoding="ascii")
print("wrote", out, len(html), "bytes")
