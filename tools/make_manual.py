#!/usr/bin/env python3
"""Builds docs/SupurrSaw_Manual.pdf (short manual). Run:  python3 tools/make_manual.py"""
import os
from reportlab.lib.pagesizes import A4
from reportlab.lib.styles import ParagraphStyle
from reportlab.lib.units import mm
from reportlab.lib import colors
from reportlab.platypus import BaseDocTemplate, PageTemplate, Frame, Paragraph, Spacer, Image, Table, TableStyle, KeepTogether, PageBreak

here = os.path.dirname(os.path.abspath(__file__))
root = os.path.dirname(here)
out = os.path.join(root, "docs", "SupurrSaw_Manual.pdf")

INK = colors.HexColor("#1b1538"); PINK = colors.HexColor("#d94995"); VIO = colors.HexColor("#4f10fd"); DIM = colors.HexColor("#5b5380")
H1 = ParagraphStyle("h1", fontName="Helvetica-Bold", fontSize=22, leading=26, textColor=INK, spaceAfter=4)
SUB = ParagraphStyle("sub", fontName="Helvetica", fontSize=11, leading=15, textColor=DIM, spaceAfter=10)
H2 = ParagraphStyle("h2", fontName="Helvetica-Bold", fontSize=13.5, leading=17, textColor=VIO, spaceBefore=12, spaceAfter=4)
B = ParagraphStyle("b", fontName="Helvetica", fontSize=9.8, leading=13.6, textColor=INK, spaceAfter=4)
LI = ParagraphStyle("li", parent=B, leftIndent=12, bulletIndent=2, spaceAfter=2)
SM = ParagraphStyle("sm", parent=B, fontSize=8.6, leading=11.5, textColor=DIM)
CELL = ParagraphStyle("cell", parent=B, fontSize=9, leading=12, spaceAfter=0)
CELLB = ParagraphStyle("cellb", parent=CELL, fontName="Helvetica-Bold")

def P(t): return Paragraph(t, B)
def L(items): return [Paragraph(t, LI, bulletText="•") for t in items]
def T(rows, widths):
    data = [[Paragraph(a, CELLB), Paragraph(b, CELL)] for a, b in rows]
    t = Table(data, colWidths=widths)
    t.setStyle(TableStyle([("VALIGN", (0, 0), (-1, -1), "TOP"), ("LINEBELOW", (0, 0), (-1, -1), 0.4, colors.HexColor("#d9d3ee")),
                           ("TOPPADDING", (0, 0), (-1, -1), 3), ("BOTTOMPADDING", (0, 0), (-1, -1), 3), ("LEFTPADDING", (0, 0), (-1, -1), 2)]))
    return t

def footer(c, d):
    c.saveState(); c.setFont("Helvetica", 8); c.setFillColor(DIM)
    c.drawString(18 * mm, 10 * mm, "SupurrSaw by Studio 87, free and open source (AGPL-3.0-or-later)")
    c.drawRightString(A4[0] - 18 * mm, 10 * mm, "Page %d" % d.page); c.restoreState()

doc = BaseDocTemplate(out, pagesize=A4, leftMargin=18 * mm, rightMargin=18 * mm, topMargin=16 * mm, bottomMargin=18 * mm,
                      title="SupurrSaw manual", author="Studio 87")
doc.addPageTemplates([PageTemplate(id="p", frames=[Frame(doc.leftMargin, doc.bottomMargin, doc.width, doc.height, id="f")], onPage=footer)])
W = doc.width
S = []
logo = os.path.join(root, "assets", "supurrsaw-cat-logo.png")
head = [[Image(logo, 22 * mm, 22 * mm) if os.path.exists(logo) else "", [Paragraph("SupurrSaw", H1), Paragraph("Supersaw, hoover and acid synth. Short manual.", SUB)]]]
ht = Table(head, colWidths=[28 * mm, W - 28 * mm]); ht.setStyle(TableStyle([("VALIGN", (0, 0), (-1, -1), "MIDDLE")])); S.append(ht)

S += [Paragraph("1. Install and first sound", H2),
      P("Read <b>INSTALL.txt</b> in the zip for your system: it says where the plugin folder goes and how to get past the first-run warning (the builds are not signed yet). "
        "Then restart your DAW, load <b>SupurrSaw</b> as an instrument and play. The standalone app does the same without a DAW."),
      P("It opens on <b>Trance 90s</b>. Press the keys, then use the arrows beside the preset name to step through sounds. "
        "In a DAW, draw MIDI notes as usual. The mod wheel (CC 1) is wired to the Mod page. Your DAW's own tempo is not read yet: set <b>BPM</b> at the bottom for the synced delay and the drawn envelopes.")]

S += [Paragraph("2. The window", H2),
      T([("Top bar", "Preset arrows and name (click to pick, double-click to rename). <b>A/B</b> compares your edits with the sound as loaded. The disk button saves; it always asks before overwriting. "
                     "<b>Loud</b>, <b>Build</b> and <b>Dyn</b> are three macro knobs: Loud makes the whole sound denser at the same peak level, Build is a one-knob riser (high pass up, reverb and delay up, clipper in), "
                     "Dyn is the dry/wet of the Opto, OTT and Squeeze section."),
          ("Lamps", "Voices playing / voices allowed, then a CPU lamp with a bar (green is fine, red is close to the limit; a star means the engine is thinning supersaw voices to keep playing). "
                    "<b>LIM</b> lights when the limiter works, <b>CLIP</b> when the soft clipper bends the output. Click either to open the FX page."),
          ("Pages", "<b>Sound</b>, <b>Kit</b>, <b>Mod</b>, <b>FX</b>, <b>Presets</b>."),
          ("Bottom bar", "Keyboard with Oct - / Oct +, mod wheel, Panic (stops every note), a volume that only changes this window, window size (Fit, 100%, 85%, 70%; double-click the wooden side to go back to 100%), "
                         "demo lines with BPM and Play, and <b>Drag last note</b> in the standalone and browser versions."),
          ("Tips", "Hover any control for about a second to see what it does. Drag a knob up or down (hold Shift for fine), use the mouse wheel, click the value to type it, double-click to reset it. "
                   "Pink dots show what you changed.")], [26 * mm, W - 26 * mm])]

S += [Paragraph("3. Sound page", H2),
      T([("Layers 1 to 3", "Each layer is Off, Super saw, Super square, Saw, Pulse, Triangle, Sine or Custom wave (drawn in the Wave lab). Detune 0 gives one clean oscillator. "
                           "Under the wave there is hard sync (with its own envelope and LFO), FM and AM from another layer, the sub or noise, and a sample option (clicks, kicks, snares, hats, plucks and more) that can play instead of or beside the wave."),
          ("Filter 1 to 3", "A type menu (LP6 to LP24, Moog, Jupiter, MS-20, SEM, Acid, HP, BP, Notch, Comb, Formant and more), run in series or parallel. The curve is draggable: pull the points for cutoff and low cut. Each filter has its own envelope."),
          ("Amp and filter envelopes", "Three of each. On the amp envelopes, <b>Link</b> on means envelope 1 shapes everything; off means envelope 2 and 3 shape layers 2 and 3."),
          ("Voice", "Mono, Poly or Stack, polyphony, glide, drift (slow tiny pitch movement that keeps sounds alive), voice spread and the bass mono frequency below which everything is summed to mono."),
          ("Sub and noise", "A sub oscillator and high-passed noise, both with their own level; samples can be placed before or after the filter.")], [34 * mm, W - 34 * mm])]

S += [Paragraph("4. Kit page: drums on the keyboard", H2),
      P("Choose a kit from the <b>Kit</b> menu at the bottom or on the Kit page: 909, Hard, Basic, Trance, House, Bass house, Hardstyle, Happy hardcore, UK hardcore, Reggaeton, Drum and bass, 808. "
        "With a kit on, the keys play drums instead of the synth. The standard layout is C1 kick, C#1 rim, D1 snare, D#1 clap, F#1 closed hat (it cuts the open hat), A#1 open hat, C#2 crash, D#2 ride, G#2 cowbell, "
        "F1 G1 A1 B1 C2 D2 toms and A#3 shaker. C3 is middle C (note 60). Use Oct - to reach the low keys. The demo menu has a drum groove for every genre and picks the matching kit."),
      P("On the Kit page each row is one drum. You can change its <b>preset</b>, the <b>keys</b> that play it (type notes like C1, B0), the <b>pitch</b> it plays at (or follow the key, good for toms), its <b>level</b> and which other drum it <b>cuts</b>. "
        "A key can belong to several rows, and they play together (layers). <b>Tweak</b> opens a drum in the normal synth: shape it, then press <b>Put back into kit</b>. "
        "<b>Save</b> and <b>Save as</b> keep your own kits; <b>Reset edits</b> undoes changes (an edited kit is marked with a star until you save or reset). The drum sounds are ordinary presets: oscillators with a pitch envelope for kicks, noise for snares and hats, plus a few samples.")]

S += [Paragraph("5. Mod page", H2),
      T([("Mod matrix", "Sources in the rows (envelopes, LFOs, velocity, key, wheel and more) and destinations in the columns (pitch, cutoff, resonance, drive, pulse width, detune, pan, volume, sync, sub, noise, high pass, filters 2 and 3, reverb, delay, drive stack, clipper, EQ cut filters). Type or drag an amount in a cell."),
          ("LFO 1 to 3", "Sine, triangle, square, saw up, saw down, random square (stepped, sample and hold) and random round (smooth). Rate, depth, fade-in and start phase. Send them anywhere in the matrix."),
          ("Drawn envelopes 1 to 3", "Draw any shape, free time or tempo-synced (4 bars down to 1/32, dotted and triplet), loop on or off, 14 base shapes, per-segment curves."),
          ("Pitch envelope", "Three draggable points for the pitch dive (hoovers, kicks, zaps).")], [34 * mm, W - 34 * mm])]

S += [Paragraph("6. FX page", H2),
      P("The chain, in order: Drive and Crush, Mod FX (chorus, flanger, phaser), the dynamics section (Opto leveller, OTT, Squeeze), Dance master, width, bass mono, then the <b>EQ</b>, master and limiter. "
        "<b>Delay</b> and <b>Reverb</b> have their own mix and tone controls (the reverb has a low cut so the tail stays clean). "
        "The <b>EQ</b> has a low cut, a low shelf, two bells, a high shelf and a high cut. Every band can be a dynamic band (it moves with the signal above a threshold) and can saturate, with 8 saturation types per band. "
        "The curve shows the live movement, and the main <b>Bypass</b> lets you hear the sound with and without the EQ."),
      P("Mod FX: chorus, flanger and phaser each keep their own knob settings when you switch. Delay and Reverb show what they do to a click (Wave switches the waveform on or off). The Dance master is a 3-band compressor, clipper and limiter with a dry/wet, a mid trim and a width section. The <b>Loud</b> knob in the top bar opens it up.")]

S += [Paragraph("7. Presets page", H2),
      P("<b>Browse</b> lists everything: filter by sound type (Lead, Bass, Keys, Pad, Pluck, Chord, Brass, Bell, Drum, FX) and by genre. Click the heart to make a favorite and use the <b>Favorites</b> chip to list only those (the arrow buttons then step through favorites only). <b>Re-order</b> shows up and down buttons to move presets inside their group; the arrows next to the preset name follow the same order. <b>Properties</b> renames a preset and sets its type, genres and tags. "
        "<b>Banks</b> A to G hold 128 slots each of your own and factory sounds; you can write, clear, and export or import them as <b>.syx</b> files to share. <b>Restore factory</b> puts bank A to D back to the factory sounds; the factory banks are also on GitHub as .syx files. "
        "The <b>Wave lab</b> lets you draw a single cycle, build one from the first 32 harmonics, or start from saw, square, triangle, pulse, vowel and organ shapes, then use it as a layer's Custom wave."),
      P("Saving: the disk button asks for a name, type, genres and an optional note, and never overwrites without asking. Your presets, saved waves, kits, exports and the log live in your user folder under <b>Studio 87/SupurrSaw</b>; the <b>Files</b> button opens them.")]

tips = ["For a quick start, load a preset close to what you want, then change only the cutoff, the envelopes and the drive.",
        "Keep the lowpass below about 12 kHz on leads. A fully open filter sounds too bright.",
        "A tiny slow drift (about 0.1 to 1 percent) keeps a sound from sounding static.",
        "Hoover: pitch envelope dive, pulse width movement and a little detune. Acid: Acid or Moog filter, high resonance, short filter envelope, and slides in the MIDI notes (overlap two notes).",
        "If the CPU lamp gets high, lower the voices of a layer or the polyphony, or turn off sections you are not using.",
        "Drum kits: put your own tweaked sounds in with Tweak, then Save as, so they come back in every project."]
S += [Paragraph("8. Tips", H2)] + L(tips)
S += [Spacer(1, 6), Paragraph("Made by one person. If it helps your music, a coffee on Ko-fi (ko-fi.com/studio87) keeps it coming. Source code and help: github.com/SpeakingColors/SupurrrSaw-VST", SM)]
doc.build(S)
print("wrote", out)
