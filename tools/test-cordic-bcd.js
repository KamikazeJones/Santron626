'use strict';
const assert = require('node:assert/strict');
const { createArithmetic } = require('../cordic-bcd');
const { calculate, compare, FUNCTIONS } = require('../cordic');
const A = createArithmetic(10), f = A.from;
const text = n => A.snapshot(n).text;
assert.equal(text(A.add(f('0.1'), f('0.2'))), '3.000000000e-1');
assert.equal(text(A.sub(f('1'), f('0.9999999999'))), '1.000000000e-10');
assert.equal(text(A.add(f('9.999999999'), f('0.000000001'))), '1.000000000e1');
assert.equal(text(A.sub(f('0'), f('0.1'))), '-1.000000000e-1');
assert.equal(text(A.div(f('1'), f('3'))), '3.333333333e-1');
assert.equal(text(A.div(f('-1'), f('3'))), '-3.333333333e-1');
assert.equal(text(A.mul(f('1.234567891'), f('2'))), '2.469135782e0');
assert.equal(text(A.sqrt(f('2'))), '1.414213562e0');
assert.equal(text(A.sqrt(f('0.04'))), '2.000000000e-1');
assert.equal(text(A.sqrt(f('4e-300'))), '2.000000000e-150');
assert.equal(text(A.scale(f('2'), -300)), '2.000000000e-300');
assert.equal(A.floor(f('-1.000000001')), -2);
assert.equal(A.floor(f('1.9')), 1);
assert.equal(text(A.mod(f('-721'), f('360'))), '-1.000000000e0');
assert.equal(A.compare(f('1e-300'), f('1e-301')), 1);
// Shifts truncate mantissa digits before addition, unlike exponent scaling.
A.drain();
const shifted = A.shift(f('1.234567891'), 3);
assert.equal(text(shifted), '1.234567000e-3');
assert.deepEqual(A.drain()[0], { operation: 'Dezimalverschiebung', discarded: '891', exponent: -12, amount: '891e-12' });
assert.equal(text(A.scale(f('1.234567891'), -3)), '1.234567891e-3');
assert.deepEqual(A.snapshot(f('1.234567890')).xc3, [4, 5, 6, 7, 8, 9, 10, 11, 12, 3]);
assert.equal(A.snapshot(f('0')).text, '0.000000000e0');
assert.throws(() => A.div(f(1), f(0)), RangeError);
assert.throws(() => createArithmetic(9), RangeError);
assert.throws(() => calculate('sin', 1, { arithmetic: 'unknown' }), RangeError);
// Verify truncation with independent rational inequalities, no floating arithmetic.
for (const precision of [10, 13, 16]) {
  const D = createArithmetic(precision);
  for (const [a, b] of [[2, 7], [12345, 17], [99999, 3], [1, 19]]) {
    const q = D.div(D.from(a), D.from(b));
    const scale = 10n ** BigInt(-q.e);
    assert.ok(q.c * BigInt(b) <= BigInt(a) * scale);
    assert.ok((q.c + 1n) * BigInt(b) > BigInt(a) * scale);
  }
}
let cases = 0;
for (const precision of [10, 13, 16]) {
  for (const fn of FUNCTIONS) for (const input of [0.1, 0.6, 1, 7.3, 30, 60]) {
    const comparison = compare(fn, input, { unit: 'deg', precision });
    const bcd = comparison.bcd;
    assert.ok(Number.isFinite(bcd.result), `${fn} ${input}`);
    const tolerance = fn === 'atan' ? 3e-5 : 3e-6;
    assert.ok(Math.abs(bcd.error) <= tolerance * Math.max(1, Math.abs(bcd.reference)), `${fn}(${input}) ${bcd.error}`);
    assert.ok(Object.isFrozen(bcd.steps.at(-1).decimal));
    assert.equal(bcd.steps.at(-1).decimal.approximation.text, bcd.resultDecimal.text);
    assert.ok(bcd.steps.every(s => s.decimal.x.digits.length === precision));
    assert.equal(comparison.rows.filter(r => r.floating).length, comparison.floating.steps.length);
    assert.equal(comparison.rows.filter(r => r.bcd).length, bcd.steps.length);
    for (const row of comparison.rows) if (row.floating && row.bcd) {
      assert.equal(row.floating.phase, row.bcd.phase);
      assert.equal(row.floating.n, row.bcd.n);
    }
    assert.doesNotThrow(() => JSON.stringify(comparison));
    cases++;
  }
}
for (const [fn, input] of [['ln', Number.MIN_VALUE], ['ln', Number.MAX_VALUE], ['exp', -708], ['exp', 709], ['atan', -1e100], ['sin', -200], ['cos', -200], ['tan', -60]]) {
  const bcd = calculate(fn, input, { arithmetic: 'bcd', unit: 'deg' });
  assert.ok(Number.isFinite(bcd.result));
}
for (const fn of ['sin', 'cos']) {
  const t = calculate(fn, 45, { unit: 'deg', arithmetic: 'bcd', precision: 13 });
  assert.equal(t.steps.filter(s => s.phase === 'Normierung').length, 0);
  assert.equal(t.steps.filter(s => s.phase === 'Auswertung').length, 1);
  const evaluation = t.steps.find(s => s.phase === 'Auswertung');
  const raw = t.steps[evaluation.index - 1];
  assert.equal(raw.x, 1); assert.equal(raw.y, 1);
  assert.deepEqual(evaluation.decimal.x, raw.decimal.x);
  assert.deepEqual(evaluation.decimal.y, raw.decimal.y);
  assert.deepEqual(t.steps.at(-1).decimal.x, raw.decimal.x);
  assert.deepEqual(t.steps.at(-1).decimal.y, raw.decimal.y);
  assert.ok(t.steps.slice(0, evaluation.index).every(s => s.losses.every(l => !['Wurzelrest', 'Quadratwurzel'].includes(l.operation))));
  assert.ok(evaluation.losses.some(l => l.operation === 'Wurzelrest'));
}
assert.equal(calculate('tan', 30, { unit: 'deg', arithmetic: 'bcd' }).steps.filter(s => s.phase === 'Normierung').length, 0);
const nearOne = calculate('atan', 1.00000000001, { arithmetic: 'bcd' });
assert.equal(nearOne.steps.find(s => s.phase === 'Reduktion').reciprocal, false);
assert.equal(nearOne.resultDecimal.text, '7.853981634e-1');
const branch = compare('ln', 7.3);
assert.ok(branch.firstBranchDifference >= 0);
assert.ok(branch.rows.some(row => !row.floating || !row.bcd));
const low = compare('tan', 30, { unit: 'deg', precision: 10 });
const high = compare('tan', 30, { unit: 'deg', precision: 16 });
assert.ok(Math.abs(high.difference) < Math.abs(low.difference));
assert.ok(low.bcd.steps.some(s => s.losses.some(l => l.operation === 'Dezimalverschiebung')));
console.log(`BCD: exact decimal operations, truncation and ${cases} comparison cases passed.`);
