'use strict';
const assert = require('node:assert/strict');
const { calculate, ROM } = require('../cordic');
let cases = 0;
function close(fn, input, options, tolerance = 3e-8) {
  const trace = calculate(fn, input, options);
  assert.ok(Number.isFinite(trace.result), `${fn}(${input}) finite`);
  assert.ok(Math.abs(trace.error) <= tolerance * Math.max(1, Math.abs(trace.reference)), `${fn}(${input}): ${trace.error}`);
  assert.equal(trace.steps.at(-1).approximation, trace.result);
  assert.ok(Object.isFrozen(trace.steps[0]));
  assert.ok(trace.steps.length < 500);
  cases++;
  return trace;
}
for (let degrees = -720; degrees <= 720; degrees += 7) {
  for (const fn of ['sin', 'cos']) close(fn, degrees, { unit: 'deg' });
  if (Math.abs(Math.cos(degrees * Math.PI / 180)) > 0.2) close('tan', degrees, { unit: 'deg' });
}
for (const rad of [-20, -Math.PI, -1.5, -0.001, 0, 0.3, 1.5, Math.PI, 20]) {
  for (const fn of ['sin', 'cos', 'tan']) close(fn, rad);
}
for (const x of [-1e100, -10, -1, -0.1, 0, 1e-12, 0.5, 1, 10, 1e100]) {
  close('atan', x); close('atan', x, { unit: 'deg' }, 2e-7);
}
for (const x of [Number.MIN_VALUE, 1e-300, 0.001, 0.1, 1, 2, Math.E, 9.999, 10, 12345, Number.MAX_VALUE]) {
  close('ln', x); close('log10', x);
}
for (const x of [-708, -100, -10, -1, 0, 0.1, 1, 10, 100, 709]) close('exp', x);
for (const x of [-300, -1, 0, 0.5, 1, 2, 300]) close('pow10', x);
for (const degrees of [-270, -90, 90, 270]) assert.throws(() => calculate('tan', degrees, { unit: 'deg' }), RangeError);
for (const x of [0, -1, NaN, Infinity]) assert.throws(() => calculate('ln', x), RangeError);
assert.throws(() => calculate('exp', 710), RangeError);
assert.throws(() => calculate('sin', 1, { levels: 0 }), RangeError);
assert.throws(() => calculate('sin', 1, { levels: 11 }), RangeError);
assert.throws(() => calculate('sin', 1, { unit: 'gon' }), RangeError);
assert.throws(() => calculate('unknown', 1), RangeError);
assert.throws(() => calculate('sin', '1'), RangeError);
// Numerical invariants validate the mechanism independently of Math results.
const rotation = calculate('tan', 30, { unit: 'deg' });
for (let i = 1; i < rotation.steps.length; i++) {
  const s = rotation.steps[i], p = rotation.steps[i - 1];
  if (s.phase === 'Rotation') {
    assert.ok(Math.abs((s.x * s.x + s.y * s.y) / (p.x * p.x + p.y * p.y) - (1 + s.shift ** 2)) < 1e-14);
    assert.ok(Math.abs(s.z - (p.z - ROM.atan[s.n])) < 1e-15);
  }
}
// Function extraction must never change the accumulated working vector.
for (const fn of ['sin', 'cos']) {
  const t = calculate(fn, 45, { unit: 'deg' });
  assert.equal(t.steps.filter(s => s.phase === 'Normierung').length, 0);
  const evaluations = t.steps.filter(s => s.phase === 'Auswertung');
  assert.equal(evaluations.length, 1);
  const evaluated = evaluations[0], raw = t.steps[evaluated.index - 1];
  assert.equal(raw.phase, 'Rotation');
  assert.equal(raw.x, 1); assert.equal(raw.y, 1);
  assert.equal(evaluated.evaluationFactor, Math.SQRT2);
  assert.equal(evaluated.x, raw.x); assert.equal(evaluated.y, raw.y);
  assert.equal(t.steps.at(-1).x, raw.x); assert.equal(t.steps.at(-1).y, raw.y);
  assert.equal(raw.approximationKind, 'component'); assert.equal(raw.approximation, 1);
  assert.equal(evaluated.approximation, 1 / Math.SQRT2);
}
for (const fn of ['tan', 'atan']) assert.equal(calculate(fn, 0.6).steps.filter(s => s.phase === 'Auswertung').length, 0);
const log = calculate('ln', 7.3);
for (const s of log.steps.filter(s => s.phase === 'Pseudo-Division')) {
  assert.ok(Math.abs(Math.log(s.x) + s.z - Math.log(7.3)) < 2e-9);
}
for (const fn of ['tan', 'atan', 'ln', 'exp']) {
  const input = fn === 'ln' ? 7.3 : 0.6;
  const coarse = calculate(fn, input, { levels: 1 }), fine = calculate(fn, input);
  assert.ok(Math.abs(fine.error) < Math.abs(coarse.error));
}
console.log(`CORDIC: ${cases} numeric cases, domain checks and convergence invariants passed.`);
