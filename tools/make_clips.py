#!/usr/bin/env python3
"""Writes the MIDI for the docs/audio demo clips and renders them with clip_render + ffmpeg.
Usage: python3 tools/make_clips.py <clip_render binary> <out dir for mp3>
Patterns follow common dance writing: one rhythm repeated every bar, a short rising call and a falling answer,
chord changes carrying the variation, bass locked to the hits. Original notes only."""
import struct, subprocess, sys, os, tempfile
def vlq(n):
    b=[n&0x7f]; n>>=7
    while n: b.append((n&0x7f)|0x80); n>>=7
    return bytes(reversed(b))
def write_mid(path, notes):  # (start_beat, dur_beats, note, vel)
    ev=[]
    for s,d,n,v in notes: ev+= [(round(s*480),1,n,v),(round((s+d)*480),0,n,0)]
    ev.sort(key=lambda e:(e[0],e[1])); tr=b''; t=0
    for tick,on,n,v in ev: tr+=vlq(tick-t)+bytes([0x90 if on else 0x80,n,v]); t=tick
    tr+=vlq(0)+b'\xff\x2f\x00'
    open(path,'wb').write(b'MThd'+struct.pack('>IHHH',6,0,1,480)+b'MTrk'+struct.pack('>I',len(tr))+tr)
S=lambda bar,slot: bar*4+slot/4
# 1) hoover riff, A Phrygian, 160 BPM: rave stab grid X--X--X-X--X--X- (slots 0,3,6,8,11,14), power chords, long hold at the end
def hoover():
    grid=[0,3,6,8,11,14]
    bars=[[57,57,60,57,58,55],[57,57,60,57,55,53],[57,57,60,57,58,55],[57,57,60,57,58,60]]
    n=[]
    for b,pit in enumerate(bars):
        for sl,p in zip(grid,pit):
            v=112 if sl in(0,8) else 98
            n+=[(S(b,sl),0.30,p,v),(S(b,sl),0.30,p+7,v-12)]
    n+=[(S(4,0),3.8,57,112),(S(4,0),3.8,64,100),(S(5,0),0.5,57,115),(S(5,0),0.5,64,105),(S(5,2),1.0,45+12,100)]
    return n
# 2) uplifting trance hook, A minor, Am F C G Am: same rhythm each bar (3,3,2,8 sixteenths), rising call, falling answer, offbeat bass
def trance():
    rhy=[(0,3),(3,3),(6,2),(8,8)]
    mel=[[72,74,76,81],[81,79,77,84],[76,77,79,84],[83,81,79,86]]
    vel=[[90,94,98,108],[96,98,100,110],[100,102,104,114],[104,106,108,118]]
    n=[]
    for b in range(4):
        for (sl,ln),p,v in zip(rhy,mel[b],vel[b]): n.append((S(b,sl),ln/4*0.95,p,v))
    n.append((S(4,0),3.9,81,120)); n.append((S(4,0),3.9,69,95))
    roots=[33,29,36,31,33]
    for b in range(5):
        for sl in (2,6,10,14): n.append((S(b,sl),0.32,roots[b],105))
    return n
# 3) house organ, 124 BPM: stab rhythm 0,3,6,10,12,14 repeated every bar, top note melody via inversions, last hit anticipates the next chord, bass on the strong hits
def house():
    V={'Am7':[(60,64,67),(64,67,69),(67,69,72),(69,72,76)],'Dm7':[(60,65,69),(65,69,72),(69,72,74),(72,74,77)],
       'G7':[(59,62,65),(62,65,67),(65,67,71),(67,71,74)],'Cmaj7':[(60,64,67),(64,67,71),(67,71,72),(71,72,76)]}
    prog=['Am7','Dm7','G7','Cmaj7','Am7']; bass={'Am7':45,'Dm7':38,'G7':43,'Cmaj7':36}
    hits=[(0,2,1),(3,1,2),(6,2,1),(10,1,0),(12,1,2),(14,2,3)]; vel={0:110,3:84,6:100,10:88,12:92,14:96}
    n=[]
    for b in range(4):
        for sl,ln,vi in hits:
            ch=prog[b+1] if sl==14 else prog[b]
            for p in V[ch][vi]: n.append((S(b,sl),ln/4*0.9,p,vel[sl]))
        for sl in (0,6,10): n.append((S(b,sl),0.45,bass[prog[b]],72))
    for p in V['Am7'][1]: n.append((S(4,0),3.6,p,100))
    n.append((S(4,0),3.6,45,72))
    return n
CLIPS=[ # file, midi builder, bpm, split, low preset, high preset, overrides
 ('hoover-classic',hoover,160,0,'none','hoover',['dist=0.42','drive=0.8','reso=0.42','cutoff=4300','detuneRange=1.9','lfoDetune=0.6','chorusMix=0.5']),
 ('trance-90s-pro',trance,138,60,'trance_bass','trance_90s_pro',[]),
 ('organ',house,124,56,'funk_bass','organ',[])]
if __name__=='__main__':
    tool,out=sys.argv[1],sys.argv[2]; tmp=tempfile.mkdtemp()
    for name,fn,bpm,split,low,high,ov in CLIPS:
        mid=os.path.join(tmp,name+'.mid'); wav=os.path.join(tmp,name+'.wav'); write_mid(mid,fn())
        subprocess.run([tool,mid,wav,str(bpm),str(split),low,high,'0','0']+ov,check=True)
        subprocess.run(['ffmpeg','-y','-loglevel','error','-i',wav,'-af','loudnorm=I=-17:TP=-1.5:LRA=11,afade=t=in:d=0.03,afade=t=out:st=8.8:d=0.6','-t','10.4','-ar','44100','-c:a','libmp3lame','-b:a','112k','-map_metadata','-1',os.path.join(out,name+'.mp3')],check=True)
        print('rendered',name)
