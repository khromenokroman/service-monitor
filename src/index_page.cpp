#include "index_page.hpp"

namespace {

constexpr std::string_view PAGE = R"HTML(<!doctype html>
<html lang="ru">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Состояние служб</title>
<style>
:root {
  --bg: #f4f6fa; --panel: #ffffff; --text: #1b2130; --muted: #6b7385; --border: #e2e6ee;
  --ok: #1f9d55; --ok-bg: #e5f6ec; --warn: #c27c0e; --warn-bg: #fdf1dc;
  --fail: #d23f3f; --fail-bg: #fbe7e7; --unknown: #7a8194; --unknown-bg: #eceef3;
  --accent: #3d63dd; --shadow: 0 1px 2px rgba(20,30,50,.06), 0 4px 16px rgba(20,30,50,.05);
}
@media (prefers-color-scheme: dark) {
  :root {
    --bg: #0f131b; --panel: #171c27; --text: #e6e9f0; --muted: #8b93a7; --border: #262d3b;
    --ok: #3ccf7a; --ok-bg: #13301f; --warn: #f0b340; --warn-bg: #33270f;
    --fail: #ff6b6b; --fail-bg: #3a1a1c; --unknown: #9aa1b3; --unknown-bg: #232937;
    --accent: #7b9bff; --shadow: none;
  }
}
* { box-sizing: border-box; }
body { margin: 0; background: var(--bg); color: var(--text);
  font: 14px/1.45 system-ui, -apple-system, "Segoe UI", Roboto, "Noto Sans", sans-serif; }
.wrap { max-width: 1200px; margin: 0 auto; padding: 24px 16px 48px; }
header { display: flex; flex-wrap: wrap; align-items: baseline; gap: 8px 16px; margin-bottom: 20px; }
h1 { font-size: 22px; margin: 0; font-weight: 650; letter-spacing: -.01em; }
.host { color: var(--muted); font-family: ui-monospace, SFMono-Regular, Menlo, monospace; }
.conn { margin-left: auto; display: flex; align-items: center; gap: 8px; color: var(--muted); font-size: 13px; }
.dot { width: 8px; height: 8px; border-radius: 50%; background: var(--ok); }
.conn.lost .dot { background: var(--fail); }
.conn.lost { color: var(--fail); }
.summary { display: grid; grid-template-columns: repeat(4, minmax(0, 1fr)); gap: 12px; margin-bottom: 20px; }
.tile { background: var(--panel); border: 1px solid var(--border); border-radius: 12px; padding: 14px 16px;
  box-shadow: var(--shadow); cursor: pointer; text-align: left; color: inherit; font: inherit; }
.tile.active { outline: 2px solid var(--accent); outline-offset: -1px; }
.tile .n { font-size: 28px; font-weight: 700; font-variant-numeric: tabular-nums; line-height: 1.1; }
.tile .l { color: var(--muted); font-size: 13px; }
.tile.ok .n { color: var(--ok); } .tile.warn .n { color: var(--warn); } .tile.fail .n { color: var(--fail); }
.toolbar { display: flex; gap: 12px; margin-bottom: 16px; }
.toolbar input { flex: 1; min-width: 0; padding: 9px 12px; border-radius: 10px; border: 1px solid var(--border);
  background: var(--panel); color: var(--text); font: inherit; }
.toolbar input:focus { outline: 2px solid var(--accent); outline-offset: -1px; }
.grid { display: grid; grid-template-columns: repeat(auto-fill, minmax(320px, 1fr)); gap: 12px; }
.card { background: var(--panel); border: 1px solid var(--border); border-left: 4px solid var(--c);
  border-radius: 12px; padding: 14px 16px; box-shadow: var(--shadow); }
.card.ok { --c: var(--ok); --cb: var(--ok-bg); } .card.warn { --c: var(--warn); --cb: var(--warn-bg); }
.card.fail { --c: var(--fail); --cb: var(--fail-bg); } .card.unknown { --c: var(--unknown); --cb: var(--unknown-bg); }
.head { display: flex; align-items: flex-start; gap: 12px; }
.names { flex: 1; min-width: 0; }
.title { font-weight: 600; font-size: 15px; overflow-wrap: anywhere; }
.unit { color: var(--muted); font-family: ui-monospace, SFMono-Regular, Menlo, monospace; font-size: 12px; overflow-wrap: anywhere; }
.badge { flex: none; padding: 3px 10px; border-radius: 999px; font-size: 12px; font-weight: 600;
  color: var(--c); background: var(--cb); white-space: nowrap; }
.desc { color: var(--muted); margin-top: 8px; font-size: 13px; }
.meta { display: grid; grid-template-columns: minmax(0, 1.7fr) minmax(0, 1fr) minmax(0, .8fr) minmax(0, .9fr); gap: 8px;
  border-top: 1px solid var(--border); padding-top: 10px; margin-top: 10px; }
.meta div { min-width: 0; }
.meta .k { color: var(--muted); font-size: 11px; text-transform: uppercase; letter-spacing: .04em; }
.meta .v { font-variant-numeric: tabular-nums; white-space: nowrap; overflow: hidden; text-overflow: ellipsis; }
.err { margin-top: 8px; color: var(--fail); font-size: 13px; overflow-wrap: anywhere; }
.empty { color: var(--muted); padding: 32px; text-align: center; grid-column: 1 / -1; }
.group { margin-bottom: 20px; }
.group-head { display: flex; align-items: center; gap: 10px; width: 100%; padding: 10px 14px; margin-bottom: 12px;
  background: var(--panel); border: 1px solid var(--border); border-left: 4px solid var(--c); border-radius: 12px;
  box-shadow: var(--shadow); color: inherit; font: inherit; cursor: pointer; text-align: left; }
.group-head.ok { --c: var(--ok); } .group-head.warn { --c: var(--warn); }
.group-head.fail { --c: var(--fail); } .group-head.unknown { --c: var(--unknown); }
.group-head .chev { flex: none; width: 16px; color: var(--muted); transition: transform .15s; }
.group.collapsed .chev { transform: rotate(-90deg); }
.group.collapsed .group-head { margin-bottom: 0; }
.group.collapsed .grid { display: none; }
.group-head .gt { flex: 1; min-width: 0; font-weight: 650; font-size: 16px; overflow-wrap: anywhere; }
.group-head .gs { flex: none; display: flex; gap: 6px; font-size: 12px; font-weight: 600; font-variant-numeric: tabular-nums; }
.pill { padding: 2px 8px; border-radius: 999px; white-space: nowrap; }
.pill.ok { color: var(--ok); background: var(--ok-bg); } .pill.warn { color: var(--warn); background: var(--warn-bg); }
.pill.fail { color: var(--fail); background: var(--fail-bg); }
.pill.total { color: var(--muted); background: var(--unknown-bg); }
footer { margin-top: 24px; color: var(--muted); font-size: 12px; text-align: center; }
.sysinfo { display: flex; flex-wrap: wrap; gap: 4px 18px; color: var(--muted); font-size: 13px; margin-bottom: 10px; }
.sysinfo b { color: var(--text); font-weight: 600; }
.system { display: grid; grid-template-columns: repeat(auto-fill, minmax(250px, 1fr)); gap: 12px; }
.group.collapsed .system { display: none; }
.group-head.sys { --c: var(--accent); }
.metric { background: var(--panel); border: 1px solid var(--border); border-radius: 12px; padding: 14px 16px; box-shadow: var(--shadow); min-width: 0; }
.metric .mh { display: flex; align-items: baseline; justify-content: space-between; gap: 8px; }
.metric .mt { color: var(--muted); font-size: 13px; }
.metric .mv { font-size: 24px; font-weight: 700; font-variant-numeric: tabular-nums; line-height: 1.2; }
.metric .ms { color: var(--muted); font-size: 12px; font-variant-numeric: tabular-nums; margin-top: 6px; }
.bar { height: 6px; border-radius: 3px; background: var(--unknown-bg); overflow: hidden; margin-top: 8px; }
.bar > i { display: block; height: 100%; border-radius: 3px; background: var(--ok); }
.bar.warn > i { background: var(--warn); } .bar.fail > i { background: var(--fail); }
.bar.neutral > i { background: var(--accent); }
.row { margin-top: 10px; }
.row:first-of-type { margin-top: 6px; }
.row .rh { display: flex; justify-content: space-between; gap: 8px; font-size: 12px; font-variant-numeric: tabular-nums; }
.row .rh span:first-child { min-width: 0; font-family: ui-monospace, SFMono-Regular, Menlo, monospace; overflow-wrap: anywhere; }
.row .rh span:last-child { color: var(--muted); white-space: nowrap; }
.row .bar { margin-top: 4px; }
.cores { display: flex; align-items: flex-end; gap: 2px; height: 40px; margin-top: 10px; }
.cores > i { flex: 1; min-width: 2px; border-radius: 2px 2px 0 0; background: var(--ok); }
.cores > i.warn { background: var(--warn); } .cores > i.fail { background: var(--fail); }
.cores-bg { background: var(--unknown-bg); border-radius: 3px; }
.merr { color: var(--fail); font-size: 12px; margin-top: 4px; overflow-wrap: anywhere; }
@media (max-width: 640px) {
  .summary { grid-template-columns: repeat(2, minmax(0, 1fr)); }
  .grid { grid-template-columns: 1fr; }
  .meta { grid-template-columns: repeat(2, minmax(0, 1fr)); }
}
</style>
</head>
<body>
<div class="wrap">
  <header>
    <h1>Состояние служб</h1>
    <span class="host" id="host"></span>
    <span class="conn" id="conn"><span class="dot"></span><span id="conn-text">Подключение…</span></span>
  </header>
  <div class="sysinfo" id="sysinfo"></div>
  <section class="group collapsed" id="sys-group">
    <button class="group-head sys" id="sys-head" type="button" aria-expanded="false">
      <span class="chev">&#9662;</span><span class="gt">Ресурсы системы</span><span class="gs" id="sys-pills"></span>
    </button>
    <div class="system" id="system"></div>
  </section>
  <div class="summary" id="summary">
    <button class="tile active" data-filter="all"><div class="n" id="n-all">–</div><div class="l">Всего</div></button>
    <button class="tile ok" data-filter="ok"><div class="n" id="n-ok">–</div><div class="l">Работают</div></button>
    <button class="tile warn" data-filter="warn"><div class="n" id="n-warn">–</div><div class="l">В переходе</div></button>
    <button class="tile fail" data-filter="fail"><div class="n" id="n-fail">–</div><div class="l">Проблемы</div></button>
  </div>
  <div class="toolbar"><input id="search" type="search" placeholder="Поиск по названию, unit'у или группе…" autocomplete="off"></div>
  <div id="content"></div>
  <footer id="footer"></footer>
</div>
<script>
"use strict";
const LABELS = { ok: "Работает", warn: "Переход", fail: "Не работает", unknown: "Неизвестно" };
let data = null, filter = "all", refreshSec = 5, timer = null;

const $ = id => document.getElementById(id);
const el = (tag, cls, text) => { const e = document.createElement(tag); if (cls) e.className = cls; if (text !== undefined) e.textContent = text; return e; };

function fmtDuration(sec) {
  if (sec < 60) return Math.floor(sec) + " с";
  const d = Math.floor(sec / 86400), h = Math.floor(sec % 86400 / 3600), m = Math.floor(sec % 3600 / 60);
  if (d) return d + " д " + h + " ч";
  if (h) return h + " ч " + m + " мин";
  return m + " мин";
}
function fmtBytes(b) {
  if (!b) return "—";
  const u = ["Б", "КБ", "МБ", "ГБ", "ТБ"]; let i = 0;
  while (b >= 1024 && i < u.length - 1) { b /= 1024; i++; }
  return (i ? String(+b.toFixed(b < 10 ? 1 : 0)) : b) + " " + u[i];
}
function isProblem(s) { return s.level === "fail" || s.level === "unknown"; }

function metaItem(k, v) { const d = el("div"); d.append(el("div", "k", k), el("div", "v", v)); d.title = v; return d; }

function card(s, nowUs) {
  const c = el("div", "card " + s.level);
  const head = el("div", "head"), names = el("div", "names");
  names.append(el("div", "title", s.title));
  if (s.title !== s.name) names.append(el("div", "unit", s.name));
  head.append(names, el("span", "badge", LABELS[s.level] || s.level));
  c.append(head);
  if (s.description && s.description !== s.name) c.append(el("div", "desc", s.description));
  const meta = el("div", "meta");
  const state = s.active_state ? s.active_state + (s.sub_state ? " / " + s.sub_state : "") : "—";
  const since = s.level === "ok" && s.active_enter_us ? fmtDuration((nowUs - s.active_enter_us) / 1e6) : "—";
  meta.append(metaItem("Состояние", state), metaItem("Работает", since),
              metaItem("PID", s.main_pid ? String(s.main_pid) : "—"), metaItem("Память", fmtBytes(s.memory_bytes)));
  c.append(meta);
  if (s.load_state && s.load_state !== "loaded") c.append(el("div", "err", "Unit: " + s.load_state));
  if (s.n_restarts) c.append(el("div", "err", "Автоматических перезапусков: " + s.n_restarts));
  if (s.error) c.append(el("div", "err", s.error));
  return c;
}

function pct(used, total) { return total ? Math.min(100, Math.max(0, 100 * used / total)) : 0; }
function sev(p) { return p >= 90 ? "fail" : p >= 70 ? "warn" : ""; }
function bar(p, neutral) { const b = el("div", "bar " + (neutral ? "neutral" : sev(p))); const i = el("i"); i.style.width = p.toFixed(1) + "%"; b.append(i); return b; }
function fmtPct(p) { return (p < 10 ? p.toFixed(1) : Math.round(p)) + "%"; }

function metric(title, value, sub) {
  const m = el("div", "metric"), h = el("div", "mh");
  h.append(el("span", "mt", title), el("span", "mv", value));
  m.append(h);
  if (sub) m.append(el("div", "ms", sub));
  return m;
}
function usageRow(label, used, total, text) {
  const r = el("div", "row"), h = el("div", "rh"), p = pct(used, total);
  h.append(el("span", "", label), el("span", "", text || fmtBytes(used) + " / " + fmtBytes(total) + " · " + fmtPct(p)));
  r.append(h, bar(p));
  return r;
}

function renderSystem(sys) {
  if (!sys) return;
  const info = $("sysinfo");
  const item = (k, v) => { const s = el("span"); s.append(k + " ", el("b", "", v)); return s; };
  const numa = sys.numa || [];
  info.replaceChildren(item("Ядро Linux", sys.kernel || "—"), item("CPU", sys.cpu_model || "—"),
                       item("Аптайм", fmtDuration(sys.uptime_sec)));

  const cards = [];
  const cpu = sys.cpu, la = sys.load;
  const c = metric("Процессор", fmtPct(cpu.usage),
                   cpu.cores + " " + plural(cpu.cores, "ядро", "ядра", "ядер") + " · NUMA: " + (numa.length || 1) +
                   " · LA " + la.map(x => x.toFixed(2)).join(" / "));
  c.insertBefore(bar(cpu.usage), c.children[1]);
  const cores = el("div", "cores cores-bg");
  cpu.per_core.forEach((u, i) => {
    const b = el("i", sev(u));
    b.style.height = Math.max(4, u).toFixed(1) + "%";
    b.title = "CPU " + i + ": " + fmtPct(u);
    cores.append(b);
  });
  c.append(cores);
  cards.push(c);

  const mem = sys.memory, memUsed = mem.total - mem.available;
  const m = metric("Память", fmtPct(pct(memUsed, mem.total)), fmtBytes(memUsed) + " из " + fmtBytes(mem.total) +
                   " · доступно " + fmtBytes(mem.available));
  m.insertBefore(bar(pct(memUsed, mem.total)), m.children[1]);
  cards.push(m);

  const sw = sys.swap, swUsed = sw.total - sw.free;
  const w = metric("Swap", sw.total ? fmtPct(pct(swUsed, sw.total)) : "—",
                   sw.total ? fmtBytes(swUsed) + " из " + fmtBytes(sw.total) : "не настроен");
  if (sw.total) w.insertBefore(bar(pct(swUsed, sw.total)), w.children[1]);
  cards.push(w);

  const hp = sys.hugepages;
  if (hp && hp.total) {
    const used = hp.total - hp.free;
    const h = metric("Hugepages", fmtPct(pct(used, hp.total)),
                     used + " / " + hp.total + " × " + fmtBytes(hp.size) + " · " + fmtBytes(used * hp.size) + " из " +
                     fmtBytes(hp.total * hp.size));
    h.insertBefore(bar(pct(used, hp.total), true), h.children[1]);
    cards.push(h);
  }

  if (numa.length > 1) {
    const n = metric("NUMA", numa.length + " " + plural(numa.length, "узел", "узла", "узлов"));
    for (const node of numa) {
      const r = usageRow("node" + node.node, node.mem_total - node.mem_free, node.mem_total);
      r.append(el("div", "ms", "CPU " + node.cpus));
      n.append(r);
    }
    cards.push(n);
  }

  const d = metric("Диски", "");
  for (const disk of sys.disks || []) {
    if (disk.error) {
      const r = el("div", "row");
      r.append(el("div", "rh"), el("div", "merr", disk.path + ": " + disk.error));
      d.append(r);
    } else {
      d.append(usageRow(disk.path, disk.used, disk.used + disk.avail));
    }
  }
  cards.push(d);

  $("system").replaceChildren(...cards);

  const pill = (label, p) => el("span", "pill " + (sev(p) || "total"), label + " " + fmtPct(p));
  const pills = [pill("CPU", cpu.usage), pill("RAM", pct(memUsed, mem.total))];
  if (sw.total) pills.push(pill("Swap", pct(swUsed, sw.total)));
  const diskMax = Math.max(0, ...(sys.disks || []).filter(x => !x.error).map(x => pct(x.used, x.used + x.avail)));
  if ((sys.disks || []).some(x => !x.error)) pills.push(pill("Диск", diskMax));
  $("sys-pills").replaceChildren(...pills);
}

function plural(n, one, few, many) {
  const m10 = n % 10, m100 = n % 100;
  if (m10 === 1 && m100 !== 11) return one;
  if (m10 >= 2 && m10 <= 4 && (m100 < 12 || m100 > 14)) return few;
  return many;
}

function render() {
  if (!data) return;
  const svc = data.services;
  const cnt = { ok: 0, warn: 0, fail: 0 };
  for (const s of svc) cnt[s.level === "unknown" ? "fail" : s.level]++;
  $("n-all").textContent = svc.length; $("n-ok").textContent = cnt.ok;
  $("n-warn").textContent = cnt.warn; $("n-fail").textContent = cnt.fail;
  $("host").textContent = data.hostname;
  renderSystem(data.system);
  document.title = (cnt.fail ? "(" + cnt.fail + ") " : "") + "Состояние служб — " + data.hostname;

  const q = $("search").value.trim().toLowerCase();
  const match = s =>
    (filter === "all" || (filter === "fail" ? isProblem(s) : s.level === filter)) &&
    (!q || s.title.toLowerCase().includes(q) || s.name.toLowerCase().includes(q) || s.description.toLowerCase().includes(q) ||
     s.group.toLowerCase().includes(q));
  const content = $("content");
  const groups = data.groups || [];

  if (!groups.length) {
    const shown = svc.filter(match);
    content.replaceChildren(grid(shown));
    if (!shown.length) content.firstChild.append(el("div", "empty", "Нет служб, подходящих под фильтр"));
    return;
  }

  const sections = [];
  for (const title of [...groups, ""]) {
    const all = svc.filter(s => s.group === title);
    const shown = all.filter(match);
    if (!shown.length) continue;
    sections.push(section(title, all, shown, !!q || filter !== "all"));
  }
  content.replaceChildren(...sections);
  if (!sections.length) content.append(el("div", "empty", "Нет служб, подходящих под фильтр"));
}

function grid(list) {
  const g = el("div", "grid");
  g.append(...list.map(s => card(s, data.time_us)));
  return g;
}

const RANK = { ok: 0, warn: 1, unknown: 2, fail: 3 };
function worst(list) { return list.reduce((w, s) => (RANK[s.level] > RANK[w] ? s.level : w), "ok"); }

function section(title, all, shown, forceOpen) {
  const key = title || "\u0000other";
  const sec = el("section", "group" + (collapsed.has(key) && !forceOpen ? " collapsed" : ""));
  const head = el("button", "group-head " + worst(all));
  head.type = "button";
  head.setAttribute("aria-expanded", String(!sec.classList.contains("collapsed")));
  const chev = el("span", "chev", "\u25BE");
  const stats = el("span", "gs");
  const ok = all.filter(s => s.level === "ok").length;
  const warn = all.filter(s => s.level === "warn").length;
  const bad = all.filter(isProblem).length;
  stats.append(el("span", "pill total", ok + " / " + all.length));
  if (warn) stats.append(el("span", "pill warn", "в переходе: " + warn));
  if (bad) stats.append(el("span", "pill fail", "проблем: " + bad));
  head.append(chev, el("span", "gt", title || "Прочие службы"), stats);
  head.addEventListener("click", () => {
    const now = sec.classList.toggle("collapsed");
    head.setAttribute("aria-expanded", String(!now));
    if (now) collapsed.add(key); else collapsed.delete(key);
    saveCollapsed();
  });
  sec.append(head, grid(shown));
  return sec;
}

function setSystemExpanded(on) {
  $("sys-group").classList.toggle("collapsed", !on);
  $("sys-head").setAttribute("aria-expanded", String(on));
}
try { setSystemExpanded(localStorage.getItem("system-expanded") === "1"); } catch (e) {}
$("sys-head").addEventListener("click", () => {
  const on = $("sys-group").classList.contains("collapsed");
  setSystemExpanded(on);
  try { localStorage.setItem("system-expanded", on ? "1" : "0"); } catch (e) {}
});

const collapsed = new Set();
try { for (const k of JSON.parse(localStorage.getItem("collapsed-groups") || "[]")) collapsed.add(k); } catch (e) {}
function saveCollapsed() { try { localStorage.setItem("collapsed-groups", JSON.stringify([...collapsed])); } catch (e) {} }

function setConn(ok, text) { $("conn").classList.toggle("lost", !ok); $("conn-text").textContent = text; }

async function refresh() {
  try {
    const r = await fetch("api/status", { cache: "no-store" });
    if (!r.ok) throw new Error("HTTP " + r.status);
    data = await r.json();
    refreshSec = data.refresh_sec || refreshSec;
    setConn(true, "Обновлено " + new Date().toLocaleTimeString("ru-RU"));
    $("footer").textContent = "Автообновление каждые " + refreshSec + " с";
    render();
  } catch (e) {
    setConn(false, "Нет связи с сервером (" + e.message + ")");
  }
  clearTimeout(timer);
  timer = setTimeout(refresh, refreshSec * 1000);
}

for (const t of document.querySelectorAll(".tile")) {
  t.addEventListener("click", () => {
    filter = t.dataset.filter;
    for (const o of document.querySelectorAll(".tile")) o.classList.toggle("active", o === t);
    render();
  });
}
$("search").addEventListener("input", render);
document.addEventListener("visibilitychange", () => { if (!document.hidden) refresh(); });
refresh();
</script>
</body>
</html>
)HTML";

} // namespace

std::string_view index_page() { return PAGE; }
