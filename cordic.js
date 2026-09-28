/* MCS7529 014: decimal pseudo multiplication/division, algorithm-level model.
 * ROM constants: Dunkelwind / bITmASTER, https://www.richis-lab.de/MCS7529.htm
 * Double and decimal precision modes; this is NOT a microcode emulator.
 */
(function (root, factory) {
  if (typeof module === 'object' && module.exports) module.exports = factory(require('./cordic-bcd'));
  else root.Cordic = factory(root.CordicBCD);
})(typeof globalThis !== 'undefined' ? globalThis : this, function (BCD) {
  'use strict';
  const ROM = Object.freeze({
    atan: Object.freeze([0.7853981634, 0.09966865249, 0.009999666687,
      0.0009999996667, 0.00009999999967, 0.000009999999999,
      0.0000009999999999, 0.00000009999999999, 0.000000009999999999,
      0.0000000009999999999]),
    ln: Object.freeze([0.6931471806, 0.09531017980, 0.009950330853,
      0.0009995003331, 0.00009999500033, 0.000009999950000,
      0.0000009999995000, 0.00000009999999500, 0.000000009999999950,
      0.0000000009999999995]),
    pi: 3.141592654, halfPi: 1.570796327, twoPi: 6.283185308,
    ln10: 2.302585093, logE: 0.4342944819
  });
  const FUNCTIONS = Object.freeze(['sin', 'cos', 'tan', 'atan', 'ln', 'log10', 'exp', 'pow10']);

  function floatingArithmetic() {
    return { from: Number, add: (a, b) => a + b, sub: (a, b) => a - b,
      mul: (a, b) => a * b, div: (a, b) => a / b, shift: (a, n) => a * 10 ** -n,
      compare: (a, b) => a < b ? -1 : a > b ? 1 : 0, abs: Math.abs,
      negate: a => -a, mod: (a, b) => a % b, floor: Math.floor,
      hypot: Math.hypot, scale: (a, e) => a * 10 ** e, toNumber: a => a };
  }

  /** arithmetic: 'double' (default) or 'bcd'; precision: 10..16 significant digits.
   * The BCD mode models decimal precision, not register masks or microcode.
   */
  function calculate(fn, input, options = {}) {
    if (!FUNCTIONS.includes(fn)) throw new RangeError('Unbekannte Funktion: ' + fn);
    if (typeof input !== 'number' || !Number.isFinite(input)) throw new RangeError('Eingabe muss endlich sein.');
    const levels = options.levels === undefined ? 10 : options.levels;
    const unit = options.unit === undefined ? 'rad' : options.unit;
    const arithmetic = options.arithmetic || 'double';
    if (!['double', 'bcd'].includes(arithmetic)) throw new RangeError('Arithmetik: double oder bcd.');
    if (!Number.isInteger(levels) || levels < 1 || levels > 10) throw new RangeError('Stufen: 1 bis 10.');
    if (!['rad', 'deg'].includes(unit)) throw new RangeError('Winkeleinheit: rad oder deg.');
    if ((fn === 'ln' || fn === 'log10') && input <= 0) throw new RangeError('Logarithmus benötigt x > 0.');
    const A = arithmetic === 'bcd' ? BCD.createArithmetic(options.precision) : floatingArithmetic();
    const f = A.from, zero = f(0), one = f(1);
    const constants = { atan: ROM.atan.map(f), ln: ROM.ln.map(f), pi: f(ROM.pi),
      halfPi: f(ROM.halfPi), twoPi: f(ROM.twoPi), ln10: f(ROM.ln10), logE: f(ROM.logE) };
    const steps = [], counts = Array(levels).fill(0);
    let state = { x: one, y: zero, z: zero, approximation: zero };
    function record(phase, operation, n = null, extra = {}) {
      const values = Object.fromEntries(Object.entries(state).map(([k, v]) => [k, A.toNumber(v)]));
      const decimal = arithmetic === 'bcd' ? Object.freeze(Object.fromEntries(
        Object.entries(state).map(([k, v]) => [k, A.snapshot(v)]))) : undefined;
      steps.push(Object.freeze({ index: steps.length, phase, operation, n,
        shift: n === null ? null : 10 ** -n, constant: null, ...values, ...extra,
        ...(decimal ? { decimal, losses: A.drain() } : {}) }));
    }
    function decompose(value, table) {
      state.z = value;
      for (let n = 0; n < levels; n++) {
        while (A.compare(state.z, table[n]) >= 0) {
          if (counts[n] >= 20) throw new RangeError('Bereichsreduktion fehlgeschlagen.');
          state.z = A.sub(state.z, table[n]); counts[n]++;
          record('Zerlegung', 'z ← z − cₙ; Zähler[n] ← Zähler[n] + 1', n,
            { constant: A.toNumber(table[n]), count: counts[n] });
        }
      }
    }
    const value = f(input);
    record('Start', 'Eingabe übernehmen', null, { input });
    let result, reference;
    if (['sin', 'cos', 'tan'].includes(fn)) {
      const period = unit === 'deg' ? f(360) : constants.twoPi;
      const half = A.div(period, f(2)), quarter = A.div(period, f(4));
      let angle = A.mod(value, period);
      if (A.compare(angle, half) > 0) angle = A.sub(angle, period);
      if (A.compare(angle, A.negate(half)) < 0) angle = A.add(angle, period);
      const sinSign = A.compare(angle, zero) < 0 ? -1 : 1;
      let a = A.abs(angle), cosSign = 1;
      if (A.compare(a, quarter) > 0) { a = A.sub(half, a); cosSign = -1; }
      const pole = A.compare(a, quarter) === 0;
      const swap = A.compare(a, A.div(quarter, f(2))) > 0;
      if (swap) a = A.sub(quarter, a);
      const target = unit === 'deg' ? A.div(A.mul(a, constants.pi), f(180)) : a;
      state.z = target;
      record('Reduktion', 'Winkel auf 0 … π/4 reduzieren; Vorzeichen und Tausch merken', null,
        { reducedInput: A.toNumber(target), swap, sinSign, cosSign });
      decompose(target, constants.atan);
      state = { x: one, y: zero, z: target, approximation: fn === 'cos' ? f(cosSign) : zero };
      // During iteration the components retain the CORDIC gain. Only tan
      // can already be read as a quotient; sin/cos are evaluated separately at end.
      function intermediate(x, y) {
        const s = A.mul(swap ? x : y, f(sinSign));
        const c = A.mul(swap ? y : x, f(cosSign));
        if (fn === 'sin') return s;
        if (fn === 'cos') return c;
        if (arithmetic === 'bcd' && A.compare(c, zero) === 0) return zero;
        return A.div(s, c);
      }
      if (fn === 'tan' && pole) throw new RangeError('Tangens ist bei diesem Winkel nicht definiert.');
      state.approximation = intermediate(one, zero);
      record('Vektorstart', 'x ← 1; y ← 0; zerlegte Drehungen anwenden', null,
        { approximationValid: !(fn === 'tan' && swap), approximationKind: fn === 'tan' ? 'function' : 'component' });
      for (let n = 0; n < levels; n++) for (let j = 0; j < counts[n]; j++) {
        const { x, y } = state;
        state.x = A.sub(x, A.shift(y, n)); state.y = A.add(y, A.shift(x, n));
        state.z = A.sub(state.z, constants.atan[n]); state.approximation = intermediate(state.x, state.y);
        record('Rotation', 'x′ ← x − y·10⁻ⁿ; y′ ← y + x·10⁻ⁿ', n,
          { constant: ROM.atan[n], approximationKind: fn === 'tan' ? 'function' : 'component' });
      }
      if (fn === 'sin' || fn === 'cos') {
        const length = A.hypot(state.x, state.y);
        // Evaluate the selected function without modifying the working vector.
        state.approximation = A.div(intermediate(state.x, state.y), length);
        record('Auswertung', 'Funktionswert ← Komponente / √(x² + y²); x und y bleiben unverändert', null,
          { evaluationFactor: A.toNumber(length), approximationKind: 'function' });
      }
      // tan uses y/x (or x/y after reduction): no normalization is needed.
      result = state.approximation;
      const rad = unit === 'deg' ? input * Math.PI / 180 : input;
      reference = Math[fn](rad);
    } else if (fn === 'atan') {
      const sign = A.compare(value, zero) < 0 ? -1 : 1, reciprocal = A.compare(A.abs(value), one) > 0;
      state = { x: one, y: reciprocal ? A.div(one, A.abs(value)) : A.abs(value), z: zero, approximation: zero };
      const output = z => A.mul(A.mul(f(sign), reciprocal ? A.sub(constants.halfPi, z) : z),
        unit === 'deg' ? A.div(f(180), constants.pi) : one);
      state.approximation = output(zero);
      record('Reduktion', 'Vorzeichen merken; bei |x| > 1 den Kehrwert verwenden', null, { reciprocal });
      for (let n = 0; n < levels; n++) {
        while (A.compare(state.y, A.shift(state.x, n)) >= 0 && A.compare(state.y, zero) > 0) {
          if (counts[n] >= 20) throw new RangeError('Vektorisierung konvergiert nicht.');
          const { x, y } = state;
          state.x = A.add(x, A.shift(y, n)); state.y = A.sub(y, A.shift(x, n));
          state.z = A.add(state.z, constants.atan[n]);
          counts[n]++; state.approximation = output(state.z);
          record('Vektorisierung', 'x′ ← x + y·10⁻ⁿ; y′ ← y − x·10⁻ⁿ; z ← z + cₙ', n,
            { constant: ROM.atan[n] });
        }
      }
      result = output(state.z);
      reference = Math.atan(input) * (unit === 'deg' ? 180 / Math.PI : 1);
    } else if (fn === 'ln' || fn === 'log10') {
      const [mantissa, exponent] = input.toExponential(16).split('e');
      const power = arithmetic === 'bcd' ? value.e + A.precision - 1 : Number(exponent);
      const base = A.mul(f(power), constants.ln10);
      // Normalize the already parsed decimal input, without importing binary
      // toExponential artifacts (e.g. 7.3 becoming 7.2999999999999998).
      state = { x: arithmetic === 'bcd' ? A.scale(value, -power) : f(Number(mantissa)), y: zero, z: zero, approximation: zero };
      const output = z => A.mul(A.add(base, z), fn === 'log10' ? constants.logE : one);
      state.approximation = output(zero);
      record('Reduktion', 'x = Mantisse · 10ᵉ; ln(x) = ln(Mantisse) + e·ln(10)', null, { power });
      for (let n = 0; n < levels; n++) {
        const factor = A.add(one, f('1e-' + n));
        while (A.compare(state.x, factor) >= 0) {
          if (counts[n] >= 20) throw new RangeError('Pseudo-Division konvergiert nicht.');
          state.x = A.div(state.x, factor); state.z = A.add(state.z, constants.ln[n]); counts[n]++;
          state.approximation = output(state.z);
          record('Pseudo-Division', 'x ← x / (1 + 10⁻ⁿ); z ← z + ln(1 + 10⁻ⁿ)', n,
            { constant: ROM.ln[n] });
        }
      }
      result = output(state.z); reference = fn === 'ln' ? Math.log(input) : Math.log10(input);
    } else {
      // Check the public model domain before quantization can hide overflow.
      const rawExponent = fn === 'pow10' ? input * ROM.ln10 : input;
      if (!Number.isFinite(rawExponent) || rawExponent > 709 || rawExponent < -708)
        throw new RangeError('Exponential-Eingabe außerhalb des Modellbereichs (−708 … 709).');
      const exponent = fn === 'pow10' ? A.mul(value, constants.ln10) : value;
      const power = A.floor(A.div(exponent, constants.ln10));
      const residual = A.sub(exponent, A.mul(f(power), constants.ln10));
      state.z = residual; state.approximation = A.scale(one, power);
      record('Reduktion', 'exp(x) = 10ᵉ · exp(x − e·ln(10))', null, { power });
      decompose(residual, constants.ln);
      state = { x: one, y: zero, z: residual, approximation: A.scale(one, power) };
      record('Produktstart', 'x ← 1; zerlegte Faktoren anwenden');
      for (let n = 0; n < levels; n++) for (let j = 0; j < counts[n]; j++) {
        state.x = A.add(state.x, A.shift(state.x, n)); state.z = A.sub(state.z, constants.ln[n]);
        state.approximation = A.scale(state.x, power);
        record('Pseudo-Multiplikation', 'x ← x + x·10⁻ⁿ; z ← z − ln(1 + 10⁻ⁿ)', n,
          { constant: ROM.ln[n] });
      }
      result = state.approximation; reference = fn === 'exp' ? Math.exp(input) : 10 ** input;
    }
    state.approximation = result;
    record('Ergebnis', 'Vorzeichen, Normierung und Bereichsreduktion auswerten');
    const numberResult = A.toNumber(result);
    return Object.freeze({ fn, input, unit, levels, arithmetic,
      precision: arithmetic === 'bcd' ? A.precision : null,
      result: numberResult, reference, error: numberResult - reference,
      ...(arithmetic === 'bcd' ? { resultDecimal: A.snapshot(result) } : {}),
      counts: Object.freeze(counts), steps: Object.freeze(steps) });
  }

  /** Align accepted operations by phase, shift level and occurrence.
   * A null side means that operation was NOT accepted in that arithmetic.
   * Never compare unrelated steps simply because their array indices match.
   */
  function compare(fn, input, options = {}) {
    const floating = calculate(fn, input, { ...options, arithmetic: 'double' });
    const bcd = calculate(fn, input, { ...options, arithmetic: 'bcd' });
    const phases = ['Start', 'Reduktion', 'Zerlegung', 'Vektorstart', 'Produktstart',
      'Rotation', 'Vektorisierung', 'Pseudo-Division', 'Pseudo-Multiplikation', 'Auswertung', 'Ergebnis'];
    function keyed(trace) {
      const seen = new Map();
      return trace.steps.map(s => {
        const prefix = `${s.phase}:${s.n}`;
        const occurrence = seen.get(prefix) || 0; seen.set(prefix, occurrence + 1);
        return { key: `${prefix}:${occurrence}`, phase: s.phase, n: s.n, occurrence, step: s };
      });
    }
    const left = keyed(floating), right = keyed(bcd);
    const keys = new Map([...left, ...right].map(row => [row.key, row]));
    const lm = new Map(left.map(row => [row.key, row.step])), rm = new Map(right.map(row => [row.key, row.step]));
    const rows = [...keys.values()].sort((a, b) => phases.indexOf(a.phase) - phases.indexOf(b.phase)
      || (a.n ?? -1) - (b.n ?? -1) || a.occurrence - b.occurrence).map((row, index) => {
        const l = lm.get(row.key) || null, r = rm.get(row.key) || null;
        const delta = l && r ? Object.freeze(Object.fromEntries(['x', 'y', 'z', 'approximation'].map(k => [k, k === 'approximation' && (r.approximationValid === false || l.approximationValid === false) ? null : r[k] - l[k]]))) : null;
        return Object.freeze({ index, key: row.key, phase: row.phase, n: row.n, floating: l, bcd: r, delta });
      });
    return Object.freeze({ floating, bcd, difference: bcd.result - floating.result,
      firstDifference: rows.findIndex(row => row.delta && Object.values(row.delta).some(v => Number.isFinite(v) && v !== 0)),
      firstBranchDifference: rows.findIndex(row => !row.floating || !row.bcd), rows: Object.freeze(rows) });
  }
  return Object.freeze({ ROM, FUNCTIONS, calculate, compare });
});
