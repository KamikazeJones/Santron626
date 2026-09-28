/* Browser view: calculations live exclusively in cordic.js. */
'use strict';
(() => {
  const $ = id => document.getElementById(id);
  const ns = 'http://www.w3.org/2000/svg';
  let trace, bcdTrace, comparison, selected = 0, timer;
  const fmt = n => Number.isFinite(n) ? (n === 0 ? '0' : n.toPrecision(12)) : String(n);
  function node(tag, attrs, text) {
    const el = document.createElementNS(ns, tag);
    for (const [k, v] of Object.entries(attrs)) el.setAttribute(k, v);
    if (text !== undefined) el.textContent = text;
    return el;
  }
  function line(svg, x1, y1, x2, y2, cls) { svg.append(node('line', { x1, y1, x2, y2, class: cls })); }
  function path(svg, points, cls) { if (points.length) svg.append(node('polyline', { points: points.map(p => p.join(',')).join(' '), class: cls })); }
  function stop() { clearInterval(timer); timer = undefined; $('play').textContent = 'Abspielen'; }
  function play() {
    if (!trace) return;
    if (timer) { stop(); return; }
    if (selected === trace.steps.length - 1) selected = 0;
    $('play').textContent = 'Pause';
    timer = setInterval(() => { select(selected + 1); if (selected === trace.steps.length - 1) stop(); }, Number($('speed').value));
  }
  function plot(svg, series, target, current, yLabel, secondary = []) {
    svg.replaceChildren();
    const values = [...series, ...secondary].map(s => s.value).filter(Number.isFinite);
    if (!values.length && !Number.isFinite(target)) values.push(0);
    if (Number.isFinite(target)) values.push(target);
    let min = Math.min(...values), max = Math.max(...values);
    const margin = Math.max((max - min) * 0.12, Math.abs(max) * 1e-10, min === 0 && max === 0 ? 1e-12 : Number.MIN_VALUE);
    min -= margin; max += margin;
    const x = i => 80 + i / Math.max(trace.steps.length - 1, 1) * 490;
    const y = v => 275 - (v - min) / (max - min) * 230;
    line(svg, 80, 40, 80, 275, 'axis'); line(svg, 80, 275, 570, 275, 'axis');
    for (let i = 0; i <= 4; i++) {
      const v = min + (max - min) * i / 4;
      line(svg, 80, y(v), 570, y(v), 'axis');
      svg.append(node('text', { x: 73, y: y(v) + 4, 'text-anchor': 'end' }, v.toExponential(2)));
    }
    svg.append(node('text', { x: 80, y: 22 }, yLabel));
    svg.append(node('text', { x: 80, y: 300 }, '0'));
    svg.append(node('text', { x: 570, y: 300, 'text-anchor': 'end' }, trace.steps.length - 1));
    if (Number.isFinite(target)) line(svg, 80, y(target), 570, y(target), 'target');
    function drawSeries(data, cls) {
      let segment = [];
      for (const s of data.filter(s => s.index <= current)) {
        if (Number.isFinite(s.value)) segment.push([x(s.index), y(s.value)]);
        else { path(svg, segment, cls); segment = []; }
      }
      path(svg, segment, cls);
    }
    drawSeries(series, 'path'); drawSeries(secondary, 'bcd-path');
    const last = series.filter(s => s.index <= current && Number.isFinite(s.value)).at(-1);
    if (last) svg.append(node('circle', { cx: x(last.index), cy: y(last.value), r: 5, fill: 'var(--teal)' }));
    line(svg, x(current), 40, x(current), 275, 'target');
  }
  function geometry() {
    const svg = $('geometry');
    const circular = ['sin', 'cos', 'tan', 'atan'].includes(trace.fn);
    if (!circular) {
      $('geometry-title').textContent = trace.fn === 'ln' || trace.fn === 'log10' ? 'Mantisse nähert sich 1' : 'Wachstum des Teilprodukts';
      $('geometry-note').textContent = 'Jeder Punkt zeigt x nach einer akzeptierten Pseudo-Division bzw. Pseudo-Multiplikation.';
      const states = t => t.steps.filter(s => !s.carried && (s.phase === 'Reduktion' || s.phase.startsWith('Pseudo') || s.phase === 'Produktstart')).map(s => ({ index: s.index, value: s.x }));
      plot(svg, states(trace), trace.fn === 'ln' || trace.fn === 'log10' ? 1 : NaN, selected, 'x', states(bcdTrace));
      return;
    }
    $('geometry-title').textContent = 'CORDIC-Vektor: Addition und Stellenverschiebung';
    const s = trace.steps[selected], bs = bcdTrace.steps[selected];
    $('geometry-note').textContent = `Tatsächliche (x, y)-Werte: türkis = Double, rosa = BCD-Modell. |v| = ${fmt(Math.hypot(s.x, s.y))} / ${fmt(Math.hypot(bs.x, bs.y))}. ` +
      'Gelbe Teilstrecken zeigen Δx und Δy. Der Einheitskreis ist nur eine Referenz; der Arbeitsvektor bleibt auch bei der Funktionsauswertung unverändert.';
    svg.replaceChildren();
    // One fixed coordinate scale for the complete trace; no per-vector scaling.
    const states = [...trace.steps, ...bcdTrace.steps];
    const maxX = Math.max(1, ...states.map(s => s.x)), maxY = Math.max(1, ...states.map(s => s.y));
    const cx = 70, cy = 280, scale = Math.min(460 / (maxX * 1.12), 240 / (maxY * 1.12));
    const point = s => [cx + scale * s.x, cy - scale * s.y];
    line(svg, cx, cy, 565, cy, 'axis'); line(svg, cx, cy, cx, 20, 'axis');
    svg.append(node('text', { x: 575, y: cy + 4 }, 'x'));
    svg.append(node('text', { x: cx - 12, y: 18 }, 'y'));
    for (let v = 0; v <= maxX * 1.05; v += 0.5) {
      const x = cx + scale * v; line(svg, x, cy - 4, x, cy + 4, 'axis');
      svg.append(node('text', { x, y: cy + 20, 'text-anchor': 'middle' }, v));
    }
    for (let v = 0.5; v <= maxY * 1.05; v += 0.5) {
      const y = cy - scale * v; line(svg, cx - 4, y, cx + 4, y, 'axis');
      svg.append(node('text', { x: cx - 10, y: y + 4, 'text-anchor': 'end' }, v));
    }
    svg.append(node('path', { d: `M ${cx + scale} ${cy} A ${scale} ${scale} 0 0 0 ${cx} ${cy - scale}`, class: 'unit-circle' }));
    svg.append(node('text', { x: cx + scale * 0.6, y: cy - scale * 0.86 }, 'Einheitskreis (Referenz)'));
    const phases = ['Vektorstart', 'Rotation', 'Vektorisierung', 'Reduktion'];
    function draw(t, cls, color) {
      const rows = t.steps.filter(row => !row.carried && phases.includes(row.phase) && row.index <= selected);
      path(svg, rows.map(point), cls);
      for (const row of rows) { const [x, y] = point(row); svg.append(node('circle', { cx: x, cy: y, r: 2, fill: color })); }
      const now = t.steps[selected], [x, y] = point(now);
      line(svg, cx, cy, x, y, cls);
      svg.append(node('circle', { cx: x, cy: y, r: cls === 'path' ? 5 : 3, fill: color,
        'data-vector': t.arithmetic, 'data-x': now.x, 'data-y': now.y }));
    }
    draw(trace, 'path', 'var(--teal)'); draw(bcdTrace, 'bcd-path', '#f393c2');
    if (!s.carried && ['Rotation', 'Vektorisierung'].includes(s.phase)) {
      const previous = trace.steps[selected - 1];
      const p = point(previous), corner = point({ x: s.x, y: previous.y }), end = point(s);
      path(svg, [p, corner, end], 'addition-path');
      svg.append(node('text', { x: 300, y: 24 }, `Δx = ${fmt(s.x - previous.x)}`));
      svg.append(node('text', { x: 300, y: 42 }, `Δy = ${fmt(s.y - previous.y)}`));
    }
    const target = trace.fn === 'atan' ? 0 : trace.steps.find(s => s.phase === 'Reduktion').reducedInput;
    // Direction reference, not an endpoint to which every step is projected.
    const direction = point({ x: Math.cos(target), y: Math.sin(target) });
    line(svg, cx, cy, ...direction, 'target');
  }
  function select(i) {
    selected = Math.max(0, Math.min(trace.steps.length - 1, i));
    const s = trace.steps[selected];
    $('step').value = selected; $('position').textContent = `${selected} / ${trace.steps.length - 1}`;
    $('previous').disabled = selected === 0; $('next').disabled = selected === trace.steps.length - 1;
    $('phase').textContent = comparison.rows[selected].phase;
    $('operation').textContent = s.carried ? 'Nur BCD akzeptiert diesen Schritt; Double-Zustand bleibt stehen.' : s.operation;
    $('registers').replaceChildren();
    for (const key of ['x', 'y', 'z', 'approximation']) {
      const cell = document.createElement('div'), label = document.createElement('small'), value = document.createElement('code');
      label.textContent = 'Double · ' + (key === 'approximation' ? (s.approximationKind === 'component' ? 'unnormierte Komponente' : 'Näherung') : key);
      value.textContent = key === 'approximation' && s.approximationValid === false ? '—' : fmt(s[key]); cell.append(label, value); $('registers').append(cell);
    }
    const operationState = comparison.rows[selected].floating || comparison.rows[selected].bcd;
    $('step-detail').textContent = operationState.n === null ? 'Bereichsreduktion und Auswertung gehören zum Rechenweg.' : `Stufe n = ${operationState.n} · Verschiebung 10⁻ⁿ = ${fmt(operationState.shift)} · ROM-Konstante = ${fmt(operationState.constant)}`;
    for (const row of $('trace').children) row.classList.toggle('selected', Number(row.dataset.index) === selected);
    geometry();
    $('convergence-title').textContent = ['sin', 'cos'].includes(trace.fn) ? 'Unnormierte Komponente und Funktionsauswertung' : 'Näherung und Referenz';
    // A decomposition counts rotations, but does not yet produce an output.
    const approximations = t => t.steps.filter(s => !s.carried && s.phase !== 'Start' && s.phase !== 'Zerlegung').map(s => ({ index: s.index, value: s.approximationValid === false ? NaN : s.approximation }));
    plot($('convergence'), approximations(trace), trace.reference, selected, ['sin', 'cos'].includes(trace.fn) ? 'Komponente → ausgewerteter Funktionswert' : 'Funktionswert', approximations(bcdTrace));
    const differences = comparison.rows.map(row => ({ index: row.index,
      value: row.delta && row.floating.approximationValid !== false && row.bcd.approximationValid !== false && !['Start', 'Zerlegung'].includes(row.phase) ? row.delta.approximation : NaN }));
    plot($('arithmetic-difference'), differences, 0, selected, 'BCD − Double');
    showComparison();
  }
  function showComparison() {
    const row = comparison.rows[selected];
    $('alignment-note').textContent = !row.floating ? 'Dieser Schritt wurde nur im BCD-Modell akzeptiert.' : !row.bcd ? 'Dieser Schritt wurde nur mit Double akzeptiert.' : 'Beide Zustände stammen aus derselben Phase, Dezimalstufe und Wiederholung.';
    $('state-comparison').replaceChildren();
    for (const k of ['x', 'y', 'z', 'approximation']) {
      const tr = document.createElement('tr');
      const invalid = k === 'approximation' && (row.floating?.approximationValid === false || row.bcd?.approximationValid === false);
      for (const value of [k === 'approximation' ? 'Näherung' : k,
        row.floating && !invalid ? row.floating[k].toPrecision(17) : '—',
        row.bcd && !invalid ? row.bcd.decimal[k].text : '—',
        row.delta && !invalid && row.delta[k] !== null ? row.delta[k].toExponential(5) : '—']) {
        const td = document.createElement('td'); td.textContent = value; tr.append(td);
      }
      $('state-comparison').append(tr);
    }
    $('decimal-digits').textContent = row.bcd ? Object.entries(row.bcd.decimal).map(([k, d]) =>
      `${k}: ${d.sign < 0 ? '−' : '+'} ${d.digits} × 10^${d.exponent}\nXC3: ${d.xc3.map(v => v.toString(16).toUpperCase()).join(' ')}`).join('\n\n') : 'Kein BCD-Schritt an dieser Stelle.';
    $('losses').replaceChildren();
    if (!row.bcd || !row.bcd.losses.length) $('losses').textContent = 'Keine protokollierten Stellenverluste in diesem Schritt.';
    else for (const loss of row.bcd.losses) {
      const p = document.createElement('p');
      p.textContent = loss.operation + ': ' + (loss.amount ? `abgeschnitten ${loss.amount}` : loss.operation === 'Divisionsrest' ? `Rest ${loss.numerator} / ${loss.denominator} × 10^${loss.exponent}` : `nicht ganzzahlige Wurzel, abgeschnitten bei 10^${loss.exponent}`);
      $('losses').append(p);
    }
  }
  function calculate(event) {
    if (event) event.preventDefault(); stop();
    try {
      const inputText = $('input').value;
      if (!inputText.trim()) throw new Error('Bitte eine Eingabe eintragen.');
      comparison = Cordic.compare($('function').value, Number(inputText), { unit: $('unit').value, levels: Number($('levels').value), precision: Number($('precision').value) });
      // Carry forward only for the visual timeline. Comparison rows retain null
      // for rejected/absent operations, so missing steps are never invented.
      function timeline(original, side) {
        let last = original.steps[0];
        return { ...original, steps: comparison.rows.map(row => {
          if (row[side]) last = row[side];
          return { ...last, index: row.index, carried: !row[side] };
        }) };
      }
      trace = timeline(comparison.floating, 'floating'); bcdTrace = timeline(comparison.bcd, 'bcd');
      $('error').hidden = true; $('visualization').hidden = false;
      $('result').textContent = trace.result.toPrecision(17); $('reference').textContent = fmt(trace.reference); $('difference').textContent = trace.error.toExponential(4);
      $('bcd-result').textContent = comparison.bcd.resultDecimal.text;
      $('bcd-error').textContent = comparison.bcd.error.toExponential(4);
      $('bcd-difference').textContent = comparison.difference.toExponential(4);
      const first = comparison.firstDifference, branch = comparison.firstBranchDifference;
      $('comparison-summary').textContent = `BCD-Modell: ${comparison.bcd.precision} Mantissenstellen, Abschneiden gegen null. ` +
        (first < 0 ? 'Keine Zustandsabweichung.' : `Erste Zustandsabweichung: Vergleichsschritt ${first}. `) +
        (branch < 0 ? 'Gleiche akzeptierte Schritte.' : `Erste abweichende Schrittentscheidung: Vergleichsschritt ${branch}.`);
      $('first-difference').disabled = first < 0; $('first-branch').disabled = branch < 0;
      $('step').max = trace.steps.length - 1;
      $('counts').textContent = 'Akzeptierte Schritte pro Dezimalstufe (Double / BCD): ' + trace.counts.map((v, n) => `n=${n}: ${v} / ${bcdTrace.counts[n]}`).join(' · ');
      $('trace').replaceChildren();
      for (const aligned of comparison.rows) {
        const s = aligned.floating || aligned.bcd;
        const row = document.createElement('tr'); row.dataset.index = aligned.index;
        if (!aligned.floating || !aligned.bcd) row.classList.add('branch-row');
        for (const value of [aligned.index, s.phase + (!aligned.floating ? ' (nur BCD)' : !aligned.bcd ? ' (nur Double)' : ''), s.n === null ? '—' : s.n, ...['x', 'y', 'z', 'approximation'].flatMap(k => [aligned.floating && !(k === 'approximation' && aligned.floating.approximationValid === false) ? fmt(aligned.floating[k]) : '—', aligned.bcd && !(k === 'approximation' && aligned.bcd.approximationValid === false) ? aligned.bcd.decimal[k].text : '—'])]) {
          const cell = document.createElement('td'); cell.textContent = value; row.append(cell);
        }
        row.addEventListener('click', () => { stop(); select(aligned.index); }); $('trace').append(row);
      }
      select(0);
    } catch (error) { trace = undefined; $('error').textContent = error.message; $('error').hidden = false; $('visualization').hidden = true; }
  }
  $('parameters').addEventListener('submit', calculate);
  $('step').addEventListener('input', () => { stop(); select(Number($('step').value)); });
  $('previous').addEventListener('click', () => { stop(); select(selected - 1); });
  $('next').addEventListener('click', () => { stop(); select(selected + 1); });
  $('play').addEventListener('click', play);
  $('last').addEventListener('click', () => { stop(); select(trace.steps.length - 1); });
  $('first-difference').addEventListener('click', () => { stop(); select(comparison.firstDifference); });
  $('first-branch').addEventListener('click', () => { stop(); select(comparison.firstBranchDifference); });
  $('speed').addEventListener('change', () => { if (timer) { stop(); play(); } });
  $('function').addEventListener('change', () => { $('unit').disabled = !['sin', 'cos', 'tan', 'atan'].includes($('function').value); });
  $('export').addEventListener('click', () => {
    const url = URL.createObjectURL(new Blob([JSON.stringify(comparison, null, 2)], { type: 'application/json' }));
    const a = document.createElement('a'); a.href = url; a.download = `cordic-${trace.fn}.json`; a.click(); setTimeout(() => URL.revokeObjectURL(url), 1000);
  });
  calculate();
})();
