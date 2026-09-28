/* Decimal arithmetic for the CORDIC precision experiment.
 * Exact integer arithmetic; normalized mantissa, truncate toward zero.
 * This models decimal precision, NOT the MCS7529's register masks/microcode.
 */
(function (root, factory) {
  if (typeof module === 'object' && module.exports) module.exports = factory();
  else root.CordicBCD = factory();
})(typeof globalThis !== 'undefined' ? globalThis : this, function () {
  'use strict';
  const abs = n => n < 0n ? -n : n;
  const pow = n => 10n ** BigInt(n);
  const length = n => abs(n).toString().length;
  function integerSqrt(n) {
    if (n < 0n) throw new RangeError('Negative Quadratwurzel.');
    if (n < 2n) return n;
    let x = 1n << BigInt(Math.ceil(n.toString(2).length / 2));
    while (true) { const next = (x + n / x) / 2n; if (next >= x) return x; x = next; }
  }
  function createArithmetic(precision = 10) {
    if (!Number.isInteger(precision) || precision < 10 || precision > 16)
      throw new RangeError('BCD-Mantisse: 10 bis 16 Stellen.');
    let events = [];
    function loss(operation, discarded, exponent) {
      if (discarded !== 0n) events.push(Object.freeze({ operation, discarded: discarded.toString(), exponent,
        amount: `${discarded}e${exponent}` }));
    }
    function normalize(c, e, operation) {
      if (c === 0n) return Object.freeze({ c: 0n, e: 0 });
      const excess = length(c) - precision;
      if (excess > 0) { const divisor = pow(excess); loss(operation, c % divisor, e); c /= divisor; e += excess; }
      else if (excess < 0) { c *= pow(-excess); e += excess; }
      return Object.freeze({ c, e });
    }
    function from(value) {
      if (value && typeof value.c === 'bigint') return value;
      const match = /^([+-]?)(\d+)(?:\.(\d*))?(?:e([+-]?\d+))?$/i.exec(String(value));
      if (!match) throw new RangeError('Ungültiger Dezimalwert: ' + value);
      const fraction = match[3] || '';
      const coefficient = BigInt(match[2] + fraction) * (match[1] === '-' ? -1n : 1n);
      return normalize(coefficient, Number(match[4] || 0) - fraction.length, 'Eingabe/Konstante');
    }
    function aligned(a, b) { const e = Math.min(a.e, b.e); return [a.c * pow(a.e - e), b.c * pow(b.e - e), e]; }
    function add(a, b) { const [x, y, e] = aligned(a, b); return normalize(x + y, e, 'Addition'); }
    function sub(a, b) { const [x, y, e] = aligned(a, b); return normalize(x - y, e, 'Subtraktion'); }
    function mul(a, b) { return normalize(a.c * b.c, a.e + b.e, 'Multiplikation'); }
    function div(a, b) {
      if (!b.c) throw new RangeError('Division durch null im BCD-Modell.');
      const digits = precision + 2;
      const scaled = a.c * pow(digits), quotient = scaled / b.c;
      const e = a.e - b.e - digits;
      if (scaled % b.c !== 0n) events.push(Object.freeze({ operation: 'Divisionsrest',
        numerator: (scaled % b.c).toString(), denominator: b.c.toString(), exponent: e }));
      return normalize(quotient, e, 'Division');
    }
    // Shift within the current mantissa BEFORE adding to another register.
    // Discarded digits are not recovered by later normalization.
    function shift(a, n) {
      const divisor = pow(n);
      loss('Dezimalverschiebung', a.c % divisor, a.e - n);
      return normalize(a.c / divisor, a.e, 'Dezimalverschiebung');
    }
    function sqrt(a) {
      if (a.c < 0n) throw new RangeError('Negative Quadratwurzel.');
      let c = a.c, e = a.e;
      if (e % 2 !== 0) { c *= 10n; e--; }
      const guard = precision + 2, radicand = c * pow(2 * guard);
      const root = integerSqrt(radicand);
      if (root * root !== radicand) events.push(Object.freeze({ operation: 'Wurzelrest', radicand: radicand.toString(), root: root.toString(), exponent: e / 2 - guard }));
      return normalize(root, e / 2 - guard, 'Quadratwurzel');
    }
    function compare(a, b) { const [x, y] = aligned(a, b); return x < y ? -1 : x > y ? 1 : 0; }
    function floor(a) {
      if (a.e >= 0) return Number(a.c * pow(a.e));
      const d = pow(-a.e), q = a.c / d;
      return Number(q - (a.c < 0n && a.c % d !== 0n ? 1n : 0n));
    }
    function mod(a, b) { const [x, y, e] = aligned(a, b); return normalize(x % y, e, 'Modulo'); }
    function negate(a) { return Object.freeze({ c: -a.c, e: a.e }); }
    function snapshot(a) {
      const digits = abs(a.c).toString().padStart(precision, '0');
      return Object.freeze({ coefficient: a.c.toString(), exponent: a.e, digits, sign: a.c < 0n ? -1 : 1,
        text: `${a.c < 0n ? '-' : ''}${digits[0]}.${digits.slice(1)}e${a.c === 0n ? 0 : a.e + precision - 1}`,
        xc3: Object.freeze([...digits].map(d => Number(d) + 3)) });
    }
    return Object.freeze({ from, add, sub, mul, div, shift, sqrt, compare, floor, mod, negate,
      abs: a => a.c < 0n ? negate(a) : a,
      hypot: (x, y) => sqrt(add(mul(x, x), mul(y, y))),
      scale: (a, exponent) => Object.freeze({ c: a.c, e: a.c === 0n ? 0 : a.e + exponent }),
      toNumber: a => Number(`${a.c}e${a.e}`), snapshot,
      drain: () => { const current = Object.freeze(events); events = []; return current; }, precision });
  }
  return Object.freeze({ createArithmetic });
});
