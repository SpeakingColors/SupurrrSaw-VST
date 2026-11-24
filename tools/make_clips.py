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
# 4) acid bass, A minor with a Phrygian b2, 132 BPM: 16 step pattern, accents on the strong steps, octave jumps, one altered bar
def acid():
    P1=[(0,33,120,.22),(2,33,88,.2),(3,45,105,.2),(5,33,88,.2),(6,36,110,.2),(8,33,120,.22),(10,43,100,.2),(11,33,88,.2),(13,45,108,.2),(14,48,118,.4)]
    P2=[(sl,(34 if sl==6 else n),v,l) for sl,n,v,l in P1]
    P3=P1[:8]+[(10,43,100,.2),(11,36,90,.2),(12,38,100,.2),(13,40,106,.2),(14,43,112,.2),(15,45,118,.2)]
    n=[]
    for b,pat in enumerate([P1,P2,P1,P3]):
        for sl,p,v,l in pat: n.append((S(b,sl),l,p,v))
    n.append((S(4,0),2.5,33,120)); return n
# 5) e-piano house, C major ii V I vi, 122 BPM: stab rhythm 0,3,6,10,12,14, melody on top, last hit anticipates the next chord
def epiano():
    C={'Dm9':([50,57,60],[64,65,69],38),'G7':([53,59,62],[64,67,71],43),'Cmaj9':([52,59,62],[67,64,71],36),'Am9':([55,60,64],[71,69,67],45)}
    prog=['Dm9','G7','Cmaj9','Am9','Cmaj9']; hits=[(0,2,0),(3,1,1),(6,2,0),(10,1,2),(12,2,1),(14,2,0)]; vel={0:104,3:80,6:94,10:82,12:90,14:92}
    n=[]
    for b in range(4):
        for sl,ln,ti in hits:
            body,tones,_=C[prog[b+1] if sl==14 else prog[b]]
            for p in body: n.append((S(b,sl),ln/4*0.9,p,vel[sl]-10))
            n.append((S(b,sl),ln/4*0.9,tones[ti],vel[sl]+6))
        for sl in (0,6,10): n.append((S(b,sl),0.4,C[prog[b]][2],74))
    body,tones,r=C['Cmaj9']
    for p in body+[tones[0]]: n.append((S(4,0),3.5,p,96))
    n.append((S(4,0),3.5,r,74)); return n
# 6) pluck arp over a pad, A minor, 138 BPM: 16ths with accents on the 3-3-3-3-2-2 grid, one chord per bar, up and back down contour
def pluck():
    ch=[[69,72,76,81],[65,69,72,77],[67,72,76,79],[67,71,74,79]]; pad=[[48,52,57],[48,53,57],[48,52,55],[47,50,55]]
    seq=[0,1,2,3,2,1,0,1,2,3,2,1,2,3,3,2]; acc={0,3,6,9,12,14}; n=[]
    for b in range(4):
        for sl in range(16): n.append((S(b,sl),0.2,ch[b][seq[sl]],112 if sl in acc else 76))
        for p in pad[b]: n.append((S(b,0),3.9,p,84))
    for p in (69,72,76,81): n.append((S(4,0),0.22,p,112))
    n+= [(S(4,0),3.5,x,84) for x in (45,52,57)]; return n
# 7) strings, A minor, 108 BPM: Am F C G Am, rising top line E F G B then A, root and chord underneath
def strings():
    ch=[[45,57,60,64],[41,53,57,60],[48,55,60,64],[43,55,59,62],[45,57,60,64]]; top=[76,77,79,83,81]; n=[]
    for b in range(5):
        L=3.9 if b<4 else 3.5
        for p in ch[b]: n.append((S(b,0),L,p,86))
        n.append((S(b,0),L,top[b],104))
    return n
# 8) brass stabs, A minor, 128 BPM: rave stab grid 0,3,6,8,11,14 with a funk bass locked to the hits
def brass():
    tri={'Am':[(57,60,64),(60,64,69)],'F':[(53,57,60),(57,60,65)],'G':[(55,59,62),(59,62,67)]}
    prog=['Am','Am','F','G','Am']; root={'Am':45,'F':41,'G':43}; grid=[0,3,6,8,11,14]; n=[]
    for b in range(4):
        for k,sl in enumerate(grid):
            ch=prog[b+1] if sl==14 else prog[b]; inv=tri[ch][0 if sl<8 else 1]; v=112 if sl in (0,8) else 96
            for p in inv: n.append((S(b,sl),0.3,p,v))
            n.append((S(b,sl),0.25,root[prog[b]],84))
    for p in tri['Am'][0]: n.append((S(4,0),3.2,p,112))
    n.append((S(4,0),3.2,45,84)); return n
# 9) bell tune over a glass pad, A minor, 112 BPM: call and answer, a rest-free rhythm q e e h each bar, resolves on A
def bell():
    mel=[[(1,76),(.5,79),(.5,81),(2,84)],[(1,81),(.5,79),(.5,77),(2,76)],[(1,79),(.5,76),(.5,79),(2,84)],[(1,83),(.5,81),(.5,79),(2,81)]]
    pad=[[48,52,57],[48,53,57],[48,52,55],[47,50,55]]; n=[]
    for b in range(4):
        t=0
        for d,p in mel[b]: n.append((b*4+t,d*0.95,p,100 if t==0 else 88)); t+=d
        for p in pad[b]: n.append((b*4,3.9,p,70))
    n.append((16,3.5,81,100)); n+= [(16,3.5,x,70) for x in (45,52,57)]; return n
# 10) choir aah, A minor, 112 BPM: Am F C G Am, smooth voice leading with the top voice moving by step
def choir():
    ch=[[52,57,60,64],[53,57,60,65],[52,55,60,64],[50,55,59,62],[52,57,60,64]]; root=[45,41,48,43,45]; n=[]
    for b in range(5):
        L=3.9 if b<4 else 3.5
        for p in ch[b]: n.append((S(b,0),L,p,90))
        n.append((S(b,0),L,root[b],70))
    return n
# 11) clav funk, E dorian, 104 BPM: one syncopated bar repeated, ghost notes, a chord hit, bass on the strong steps, a climb in the last bar
def clav():
    R=[(0,52,112,.3),(3,52,80,.15),(4,55,100,.25),(6,57,105,.25),(8,59,110,.3),(10,57,85,.15),(11,55,100,.25),(13,52,95,.25),(14,50,90,.2)]
    R4=R[:7]+[(13,55,98,.2),(14,57,104,.2),(15,59,112,.25)]; n=[]
    for b in range(4):
        for sl,p,v,l in (R4 if b==3 else R): n.append((S(b,sl),l,p,v))
        for p in (64,67,71): n.append((S(b,8),0.2,p,88))
        for sl in (0,6,8,13): n.append((S(b,sl),0.3,45 if b==2 else 40,90))
    n.append((S(4,0),2.5,52,112)); n.append((S(4,0),2.5,40,92)); return n
HOOVER=['dist=0','drive=0.12','reso=0.15','cutoff=3800','detuneRange=1.4','lfoDetune=0.3','chorusMix=0.4','lfoPwm=0.5']
SOFTBASS=['lo:drive=0.05','lo:dist=0.0','lo:reso=0.25']
CLIPS=[ # file, midi builder, bpm, split, low preset, high preset, overrides
 ('hoover-classic',hoover,160,0,'none','hoover',HOOVER),
 ('trance-90s-pro',trance,138,60,'trance_bass','trance_90s_pro',[]),
 ('organ',house,124,56,'funk_bass','organ',SOFTBASS),
 ('acid-bass',acid,132,0,'none','nboy_303',['dist=0.35']),
 ('epiano',epiano,122,0,'none','epiano_dx',[]),
 ('pluck-arp',pluck,138,60,'warm_pad','pluck',[]),
 ('strings',strings,108,0,'none','strings',[]),
 ('brass',brass,128,0,'none','brass',[]),
 ('bell',bell,112,60,'glass_pad','bell',[]),
 ('choir',choir,112,0,'none','choir',[]),
 ('clav',clav,104,48,'funk_bass','clav',SOFTBASS)]
if __name__=='__main__':
    tool,out=sys.argv[1],sys.argv[2]; only=set(sys.argv[3:]); tmp=tempfile.mkdtemp()
    import json
    for name,fn,bpm,split,low,high,ov in CLIPS:
        if only and name not in only: continue
        mid=os.path.join(tmp,name+'.mid'); wav=os.path.join(tmp,name+'.wav'); write_mid(mid,fn())
        subprocess.run([tool,mid,wav,str(bpm),str(split),low,high,'0','0']+ov,check=True)
        # two-pass linear loudnorm: plain gain, no dynamic pumping that can sound like distortion on short clips
        r=subprocess.run(['ffmpeg','-hide_banner','-i',wav,'-af','loudnorm=I=-17:TP=-1.5:LRA=11:print_format=json','-f','null','-'],capture_output=True,text=True).stderr
        d=json.loads(r[r.rindex('{'):r.rindex('}')+1])
        ln='loudnorm=I=-17:TP=-1.5:LRA=11:linear=true:measured_I=%s:measured_TP=%s:measured_LRA=%s:measured_thresh=%s:offset=%s'%(d['input_i'],d['input_tp'],d['input_lra'],d['input_thresh'],d['target_offset'])
        import wave as _w; w=_w.open(wav); dur=min(10.4,w.getnframes()/w.getframerate()); w.close()
        subprocess.run(['ffmpeg','-y','-loglevel','error','-i',wav,'-af',ln+',afade=t=in:d=0.03,afade=t=out:st=%.2f:d=0.6'%(dur-0.6),'-t','%.2f'%dur,'-ar','44100','-c:a','libmp3lame','-b:a','112k','-map_metadata','-1',os.path.join(out,name+'.mp3')],check=True)
        print('rendered',name,'%.1fs'%dur)
