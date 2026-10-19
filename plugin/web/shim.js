/* Studio 87 SupurrSaw: plugin bridge. Runs before the page script. Inside the plugin it replaces the browser
   audio engine with the native one, so the page itself stays exactly the mockup. */
(function () {
  "use strict";
  if (!window.__JUCE__ || !window.__JUCE__.backend) return;
  const B = window.__JUCE__.backend, ID = window.__JUCE__.initialisationData || {};
  const one = k => { const a = ID[k]; return Array.isArray(a) ? a[0] : a; };
  window.__SUPURR_PLUGIN = true;

  /* ---- native calls (promise based) ---- */
  let pid = 0; const pend = new Map();
  B.addEventListener("__juce__complete", o => { const r = pend.get(o.promiseId); if (r) { pend.delete(o.promiseId); r(o.result); } });
  function call(name, ...params) { return new Promise(res => { const id = ++pid; pend.set(id, res); B.emitEvent("__juce__invoke", { name, params, resultId: id }); }); }
  const emit = (id, o) => { try { B.emitEvent(id, o); } catch (e) {} };
  const log = t => { call("log", String(t)); };
  window.addEventListener("error", e => log("error: " + e.message + " (" + (e.filename || "").split("/").pop() + ":" + e.lineno + ")"));
  window.addEventListener("unhandledrejection", e => log("rejected: " + (e.reason && e.reason.message || e.reason)));

  const SR = +one("sampleRate") || 48000;
  try { window.__plugState = JSON.parse(one("state") || "null"); } catch (e) { window.__plugState = null; }
  log("page started, plugin " + one("version") + ", " + SR + " Hz, charset " + document.characterSet + ", type " + document.contentType);

  /* ---- fake Web Audio: just what the page touches ---- */
  const RING = 1 << 15, MASK = RING - 1, ring = new Float32Array(RING); let wp = 0;
  function pushScope(b64) {
    const bin = atob(b64), n = bin.length >> 1;
    for (let i = 0; i < n; i++) { let v = bin.charCodeAt(2 * i) | (bin.charCodeAt(2 * i + 1) << 8); if (v & 0x8000) v -= 0x10000; ring[wp++ & MASK] = v / 32767; }
  }
  B.addEventListener("scope", pushScope);

  class FakeAna {
    constructor() { this.fftSize = 2048; this.smoothingTimeConstant = 0.8; this._sm = null; this._lastWp = -1; this._N = 0; }
    get frequencyBinCount() { return this.fftSize >> 1; }
    getFloatTimeDomainData(a) { const n = a.length; for (let i = 0; i < n; i++) a[i] = ring[(wp - n + i) & MASK]; }
    _prep(N) {
      if (this._N === N) return; this._N = N;
      this._win = new Float32Array(N); for (let i = 0; i < N; i++) this._win[i] = 0.42 - 0.5 * Math.cos(2 * Math.PI * i / N) + 0.08 * Math.cos(4 * Math.PI * i / N);
      this._rev = new Uint32Array(N); const bits = Math.log2(N); for (let i = 0; i < N; i++) { let r = 0, x = i; for (let b = 0; b < bits; b++) { r = (r << 1) | (x & 1); x >>= 1; } this._rev[i] = r; }
      this._cos = new Float32Array(N / 2); this._sin = new Float32Array(N / 2); for (let i = 0; i < N / 2; i++) { this._cos[i] = Math.cos(2 * Math.PI * i / N); this._sin[i] = -Math.sin(2 * Math.PI * i / N); }
      this._re = new Float32Array(N); this._im = new Float32Array(N); this._sm = new Float32Array(N / 2).fill(0);
    }
    getFloatFrequencyData(a) {
      const N = this.fftSize; this._prep(N);
      if (wp !== this._lastWp) {
        this._lastWp = wp; const re = this._re, im = this._im, rev = this._rev;
        for (let i = 0; i < N; i++) { re[rev[i]] = ring[(wp - N + i) & MASK] * this._win[i]; im[rev[i]] = 0; }
        for (let len = 2; len <= N; len <<= 1) {
          const half = len >> 1, stp = N / len;
          for (let i = 0; i < N; i += len) for (let k = 0; k < half; k++) {
            const c = this._cos[k * stp], s = this._sin[k * stp], a1 = i + k, b1 = a1 + half;
            const tr = re[b1] * c - im[b1] * s, ti = re[b1] * s + im[b1] * c;
            re[b1] = re[a1] - tr; im[b1] = im[a1] - ti; re[a1] += tr; im[a1] += ti;
          }
        }
        const t = this.smoothingTimeConstant, sm = this._sm;
        for (let k = 0; k < N / 2; k++) sm[k] = t * sm[k] + (1 - t) * Math.hypot(re[k], im[k]) / N;
      }
      const n = Math.min(a.length, N / 2);
      for (let k = 0; k < n; k++) a[k] = 20 * Math.log10(this._sm[k] + 1e-9);
    }
  }
  class Param { constructor(v) { this._v = v; this.onset = null; } get value() { return this._v; } set value(x) { this._v = x; if (this.onset) this.onset(x); } }
  let node = null, gains = 0;
  class FakeCtx {
    constructor() { this.sampleRate = SR; this.state = "running"; this.destination = {}; this.currentTime = 0; this.audioWorklet = { addModule: () => Promise.resolve() }; }
    createGain() { const g = { gain: new Param(1), connect() { return g; }, disconnect() {} }; if (!gains++) g.gain.onset = v => emit("vol", { v: +v }); return g; }
    createAnalyser() { return new FakeAna(); }
    suspend() { this.state = "suspended"; emit("mute", { on: true }); return Promise.resolve(); }
    resume() { this.state = "running"; emit("mute", { on: false }); return Promise.resolve(); }
  }
  window.AudioContext = FakeCtx; window.webkitAudioContext = FakeCtx;
  window.AudioWorkletNode = class {
    constructor() { node = this; this.port = { onmessage: null, postMessage: send }; }
    connect() { return this; } disconnect() {}
  };
  function send(m) {
    if (m && m.t === "cyc" && m.d) m = { t: "cyc", ly: m.ly, d: Array.from(m.d) };
    emit("msg", m);
  }
  B.addEventListener("meters", d => { if (node && node.port.onmessage) node.port.onmessage({ data: d }); });

  /* ---- db and downloads: files in the Studio 87 folder ---- */
  const mkDoc = (path, json) => ({ id: path.split("/").pop(), data: () => JSON.parse(json) });
  const DB = {
    doc: path => ({
      set: async data => {
        const j = JSON.stringify(data);
        if (path === "state/current") emit("uistate", { name: data.name || "", base: data.base || "", page: data.page || "" });
        if (!(await call("dbSet", path, j))) throw { code: "write_failed" };
      },
      delete: async () => { await call("dbDelete", path); }
    }),
    collection: name => ({
      get: async () => { const l = (await call("dbList", name)) || []; const docs = []; l.forEach(x => { try { docs.push(mkDoc(name + "/" + x.id, x.json)); } catch (e) {} }); return { docs }; }
    })
  };
  const DL = {
    save: async ({ filename, data }) => {
      let buf;
      if (typeof data === "string") buf = new TextEncoder().encode(data); else if (data && data.arrayBuffer) buf = new Uint8Array(await data.arrayBuffer()); else buf = new Uint8Array(data);
      let s = ""; for (let i = 0; i < buf.length; i += 8192) s += String.fromCharCode.apply(null, buf.subarray(i, i + 8192));
      const p = await call("saveFile", filename, btoa(s));
      if (!p) throw { code: "write_failed" };
      toast("Saved to Exports: " + filename);
    }
  };
  window.claude = { use: async n => (n === "db" ? DB : n === "downloads" ? DL : null) };

  /* ---- little UI extras: Files menu and a toast ---- */
  function toast(t) {
    const d = document.createElement("div"); d.textContent = t;
    d.style.cssText = "position:fixed;left:50%;bottom:18px;transform:translateX(-50%);background:var(--panel2,#2c2c62);color:var(--fg,#fff);border:1px solid var(--line,#774BF9);border-radius:999px;padding:7px 16px;font:13px Fredoka,sans-serif;z-index:99999;box-shadow:0 4px 18px #0006";
    document.body.appendChild(d); setTimeout(() => d.remove(), 2600);
  }
  window.addEventListener("DOMContentLoaded", () => {
    const power = document.getElementById("power"); if (!power) return;
    const b = document.createElement("button"); b.className = "btn"; b.id = "filesBtn"; b.textContent = "Files"; b.dataset.tip = "Open the folders where presets, waves, exports and the log are kept, or copy a diagnostics report.";
    power.after(b);
    const menu = document.createElement("div"); menu.hidden = true;
    menu.style.cssText = "position:fixed;z-index:99998;background:var(--panel,#1B0264);border:1px solid var(--line,#774BF9);border-radius:12px;padding:6px;display:flex;flex-direction:column;gap:4px;box-shadow:0 8px 28px #0008";
    [["Open presets folder", () => call("openFolder", "presets")], ["Open waves folder", () => call("openFolder", "waves")], ["Open exports folder", () => call("openFolder", "exports")],
     ["Open log folder", () => call("openFolder", "log")], ["Copy diagnostics", async () => { await call("copyDiagnostics"); toast("Diagnostics copied"); }]].forEach(([t, f]) => {
      const x = document.createElement("button"); x.className = "btn sm"; x.textContent = t; x.style.textAlign = "left"; x.onclick = () => { menu.hidden = true; f(); }; menu.appendChild(x);
    });
    document.body.appendChild(menu);
    b.onclick = e => { e.stopPropagation(); const r = b.getBoundingClientRect(); menu.style.left = r.left + "px"; menu.style.top = r.bottom + 6 + "px"; menu.hidden = !menu.hidden; };
    document.addEventListener("pointerdown", e => { if (!menu.hidden && !menu.contains(e.target) && e.target !== b) menu.hidden = true; }, true);
    // the plugin has no browser autoplay rule: start the engine at once
    setTimeout(() => power.click(), 80);
  });
})();
