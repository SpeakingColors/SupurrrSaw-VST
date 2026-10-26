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
      P("Run the installer for your system (Windows: the <b>setup .exe</b>; macOS: the <b>.pkg</b>; Linux: the <b>.deb</b> for DAWs installed normally, or the <b>.flatpak</b> for the standalone app). It puts the plugin where your DAW looks, so there is nothing to copy. The first time, Windows and macOS warn about an unsigned app because the builds are not signed yet: Windows, click More info then Run anyway; macOS, open System Settings, Privacy and Security, and click Open Anyway. "
        "Then restart your DAW, load <b>SupurrSaw</b> as an instrument and play. The standalone app does the same without a DAW."),
      P("It opens on <b>Trance 90s</b>. Press the keys, then use the arrows beside the preset name to step through sounds. "
        "In a DAW, draw MIDI notes as usual. The mod wheel, pitch wheel and MPE are described in section 2a. Your DAW's own tempo is not read yet: set <b>BPM</b> at the bottom for the synced delay and the drawn envelopes.")]

S += [Paragraph("2. The window", H2),
      T([("Top bar", "Preset arrows and name (click to pick, double-click to rename). <b>A/B</b> compares your edits with the sound as loaded. The disk button saves; it always asks before overwriting. "
                     "<b>Loud</b>, <b>Build</b> and <b>Dyn</b> are three macro knobs: Loud makes the whole sound denser at the same peak level, Build is a one-knob riser (high pass up, reverb and delay up, clipper in), "
                     "Dyn is the dry/wet of the Opto, OTT and Squeeze section."),
          ("Lamps", "Voices playing / voices allowed, then a CPU lamp with a bar (green is fine, red is close to the limit; a star means the engine is thinning supersaw voices to keep playing). "
                    "<b>LIM</b> lights when the limiter works, <b>CLIP</b> when the soft clipper bends the output. Click either to open the FX page."),
          ("Pages", "<b>Sound</b>, <b>Kit</b>, <b>Mod</b>, <b>FX</b>, <b>Presets</b>."),
          ("Bottom bar", "Keyboard with Oct - / Oct +, mod wheel and pitch wheel, bend range boxes, the MPE switch and MPE range, Panic (stops every note), a volume that only changes this window, window size (Fit, 100%, 85%, 70%; double-click the wooden side to go back to 100%), "
                         "demo lines with BPM and Play, and <b>Drag last note</b> in the standalone and browser versions."),
          ("Tips", "Hover any control for about a second to see what it does. Drag a knob up or down (hold Shift for fine), use the mouse wheel, click the value to type it, double-click to reset it. "
                   "Pink dots show what you changed.")], [26 * mm, W - 26 * mm])]

S += [Paragraph("2a. Pitch wheel, mod wheel and MPE", H2),
      T([("Pitch wheel", "Your keyboard's pitch wheel (or the pitch wheel left of the on-screen keyboard, which springs back to the middle when you let go) bends every note. The two boxes beside it set how far it goes up (+) and down (-) at full travel, 0 to 24 semitones each (12 is an octave, good for risers and falls). They are saved with the preset; the default is +2 and -2."),
          ("Mod wheel", "CC 1, and the second wheel left of the on-screen keyboard (it stays where you leave it; double-click to zero it). Send it to anything in the <b>Mod matrix</b> (Mod wheel row), or use the <b>Wheel</b> knob beside the cutoff for a quick filter opening, or the vibrato depth and sync wheel controls."),
          ("MPE (per-note expression)", "With <b>MPE</b> on, a note sent on MIDI channels 2 to 16 can bend, slide and press on its own, so you can bend one note of a chord while the others stay still. Channel 1 is the main channel and moves every note. MPE keyboards and the piano roll of DAWs that support MPE send this automatically. <b>MPE range</b> is how far a single note bends (48 semitones is the MPE standard; change it only if your controller uses another range). Off, every channel is treated the same. The MPE switch and its range belong to you, not to the preset."),
          ("Drag to modulate", "Press <b>Mod</b> next to the keyboard to open a tray of sources (LFOs, envelopes, mod wheel, pressure and more). Drag a source onto a knob that has a Mod matrix column, let go, then set the amount in the small box that opens. A colored ring on the knob shows the range, and the number on it opens the list again. Click the cross to remove a source, double-click a slider to reset it. It edits the same cells as the Mod matrix."),
          ("Pressure, Slide, Bend", "Three more rows in the Mod matrix. <b>Pressure</b> is aftertouch (channel pressure, or polyphonic key pressure), <b>Slide</b> is CC 74 (the finger sliding on an MPE key), <b>Bend</b> is the pitch wheel plus the note's own bend in octaves. With MPE on, each note has its own pressure, slide and bend. Example: Pressure to cutoff opens the filter as you press harder; Slide to cutoff or to detune adds a second dimension to each key.")], [30 * mm, W - 30 * mm])]

S += [Paragraph("3. Sound page", H2),
      T([("Layers 1 to 3", "Each layer has an <b>On</b> switch (off mutes it but keeps its wave and settings) and is Off, Super saw, Super square, Saw, Pulse, Triangle, Sine or Custom wave (drawn in the Wave lab; your saved waves are listed under My waves, and a factory Aah stab wave is included). Detune 0 gives one clean oscillator. "
                           "Under the wave there is hard sync (with its own envelope and LFO), FM and AM from another layer, the sub or noise, and a sample option (clicks, kicks, snares, hats, plucks and more) that can play instead of or beside the wave. The small pitch menu next to the sample decides whether it follows the keys: <b>Auto</b> keeps clicks and drums at one pitch while plucks and chimes follow, <b>Keys</b> always follows, <b>1 pitch</b> never does."),
          ("Filter 1 to 3", "A type menu (LP6 to LP24, Moog, Jupiter, MS-20, SEM, Acid, HP, BP, Notch, Comb, Formant and more), run in series or parallel. The curve is draggable: pull the points for cutoff and low cut. The <b>Slope</b> menu sets the side filter (low cut or high cut) from 6 up to 96 dB per octave. Each filter has its own envelope."),
          ("Amp and filter envelopes", "Three of each. On the amp envelopes, <b>Link</b> on means envelope 1 shapes everything; off means envelope 2 and 3 shape layers 2 and 3."),
          ("Voice", "Mono, Poly or Stack, polyphony, glide, drift (slow tiny pitch movement that keeps sounds alive), voice spread and the bass mono frequency below which everything is summed to mono. <b>Legato</b> (Mono): a key played over a held key only changes pitch, the envelopes keep running, and letting go of the top key falls back to the key still held. <b>Glide</b> sets the slide time; the glide menu chooses Always or Held only (slide only between overlapping notes), the curve menu chooses Exp (Glide is the time constant) or Linear (Glide is the full time), and <b>Poly glide</b> makes Poly and Stack notes slide in from the previous key."),
          ("Sub and noise", "A sub oscillator and high-passed noise, both with their own level; samples can be placed before or after the filter. The <b>Spread</b> tab adds Voices (1 to 5), Detune and Width to both the sub and the noise, for a thick moving sub or a wide airy hiss.")], [34 * mm, W - 34 * mm])]

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
      P("Mod FX: chorus, flanger and phaser each keep their own knob settings when you switch, and <b>Stack</b> runs all three at once (pick a type, set its knobs, then pick the next). Most effect cards (Mod FX, Delay, Reverb, Squeeze, Opto, OTT, Drive, Pitch envelope, and the Dance master and Width tabs) have a <b>Presets</b> menu with factory settings by sound type plus Save current for your own. Delay and Reverb show what they do to a click and, while you play, a live scrolling waveform of what they output (Wave switches the waveform on or off). The Dance master is a 3-band compressor, clipper and limiter with a dry/wet, a mid trim and a width section. The <b>Loud</b> knob in the top bar opens it up.")]

S += [Paragraph("7. Presets page", H2),
      P("<b>Browse</b> lists everything: filter by sound type (Lead, Bass, Keys, Pad, Pluck, Chord, Brass, Bell, Drum, FX) and by genre. Click the heart to make a favorite and use the <b>Favorites</b> chip to list only those (the arrow buttons then step through favorites only). <b>Re-order</b> shows up and down buttons to move presets inside their group; the arrows next to the preset name follow the same order. <b>Properties</b> renames a preset and sets its type, genres and tags. "
        "<b>Banks</b> A to G hold 128 slots each of your own and factory sounds; you can write, clear, and export or import them as <b>.syx</b> files to share. <b>Restore factory</b> puts bank A to D back to the factory sounds; the factory banks are also on GitHub as .syx files. "
        "The <b>Wave lab</b> lets you draw a single cycle, build one from the first 32 harmonics, or start from saw, square, triangle, pulse, vowel and organ shapes, then use it as a layer's Custom wave."),
      P("Saving: the disk button asks for a name, type, genres and an optional note, and never overwrites without asking. Your presets, saved waves, kits, exports and the log live in your user folder under <b>Studio 87/SupurrSaw</b>; the <b>Files</b> button opens them. The plugin comes with eight starter presets in <b>Your saved</b> (Daft Funk mod, DANCE SUPER AAHH, deep dirty bass, hoover amazing, hoover new meow made, Sandstorm Lead mod, suopersaw chords, Trance 90s Pro 2); they are copied to your folder the first time you open the plugin and are yours to edit or delete.")]

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
