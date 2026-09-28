'use strict';

const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const vm = require('node:vm');

const source = fs.readFileSync(path.join(__dirname, '..', 'cordic-algorithm.js'), 'utf8');
const { DecimalNumber, digitadd } = vm.runInNewContext(`${source}\n({ DecimalNumber, digitadd })`);

for (const [a, b, carry, digit, overflow] of [
  ['0', '0', 0, '0', 0],
  ['4', '5', 0, '9', 0],
  ['4', '5', 1, '0', 1],
  ['9', '1', 0, '0', 1],
  ['0', '9', 1, '0', 1],
  ['9', '0', 1, '0', 1],
  ['9', '9', 1, '9', 1],
]) {
  const result = digitadd(a, b, carry);
  assert.equal(result.digit, digit, `${a} + ${b} + ${carry}: Ziffer`);
  assert.equal(result.overflow, overflow, `${a} + ${b} + ${carry}: Übertrag`);
}

assert.throws(() => digitadd('x', '1', 0), { name: 'TypeError' });
assert.throws(() => digitadd('1', '2', 2), { name: 'RangeError' });

function number(mantissa, mantissaSign = '+', exponent = '00', exponentSign = '+') {
  const value = new DecimalNumber(mantissa.length);
  value.mantissa = [...mantissa];
  value.mantissaSign = mantissaSign;
  value.exponent = [...exponent];
  value.exponentSign = exponentSign;
  return value;
}

function assertNumber(value, mantissa, mantissaSign, exponent, exponentSign) {
  assert.equal(value.mantissa.join(''), mantissa);
  assert.equal(value.mantissaSign, mantissaSign);
  assert.equal(value.exponent.join(''), exponent);
  assert.equal(value.exponentSign, exponentSign);
}

const original = number('1234', '-', '07', '-');
const copied = original.copy();
assert.notEqual(copied, original);
assert.notEqual(copied.mantissa, original.mantissa);
assert.notEqual(copied.exponent, original.exponent);
copied.mantissa[0] = '9';
copied.exponent[0] = '8';
assertNumber(original, '1234', '-', '07', '-');

const complemented = number('039', '-', '12', '-');
assert.equal(complemented.nineComplement(), complemented);
assertNumber(complemented, '960', '-', '12', '-');

const shifted = number('1234', '+', '01', '-');
assert.equal(shifted.shiftRight(2), shifted);
assertNumber(shifted, '0012', '+', '01', '+');
assert.throws(() => shifted.shiftRight(-1), { name: 'RangeError' });
assert.throws(() => shifted.shiftRight(1.5), { name: 'RangeError' });

const exponentLimit = number('12', '+', '99', '+');
assert.throws(() => exponentLimit.shiftRight(1), { name: 'RangeError' });
assertNumber(exponentLimit, '12', '+', '99', '+');

const simple = number('123');
const addend = number('234');
assert.equal(simple.add(addend), simple);
assertNumber(simple, '357', '+', '00', '+');
assertNumber(addend, '234', '+', '00', '+');

const aligned = number('120', '+', '02', '+');
aligned.add(number('340', '+', '01', '+'));
assertNumber(aligned, '154', '+', '02', '+');

const negativeExponent = number('120', '+', '01', '-');
negativeExponent.add(number('340', '+', '02', '-'));
assertNumber(negativeExponent, '154', '+', '01', '-');

const subtraction = number('500');
subtraction.add(number('200', '-', '00', '+'));
assertNumber(subtraction, '300', '+', '00', '+');

const negativeResult = number('200');
negativeResult.add(number('500', '-', '00', '+'));
assertNumber(negativeResult, '300', '-', '00', '+');

const mantissaOverflow = number('999');
mantissaOverflow.add(number('999'));
assertNumber(mantissaOverflow, '199', '+', '01', '+');

const cancellation = number('123', '-');
cancellation.add(number('123'));
assertNumber(cancellation, '000', '+', '00', '+');

assert.throws(() => number('12').add(number('123')), { name: 'RangeError' });

const subPositive = number('500');
const subtrahend = number('200');
assert.equal(subPositive.sub(subtrahend), subPositive);
assertNumber(subPositive, '300', '+', '00', '+');
assertNumber(subtrahend, '200', '+', '00', '+');

const subNegative = number('200');
subNegative.sub(number('500'));
assertNumber(subNegative, '300', '-', '00', '+');

const subNegativeOperands = number('500', '-');
subNegativeOperands.sub(number('200', '-'));
assertNumber(subNegativeOperands, '300', '-', '00', '+');

const subOppositeSigns = number('500');
subOppositeSigns.sub(number('200', '-'));
assertNumber(subOppositeSigns, '700', '+', '00', '+');

const subAligned = number('120', '+', '02', '+');
subAligned.sub(number('340', '+', '01', '+'));
assertNumber(subAligned, '086', '+', '02', '+');

const subNegativeExponent = number('120', '+', '01', '-');
subNegativeExponent.sub(number('340', '+', '02', '-'));
assertNumber(subNegativeExponent, '086', '+', '01', '-');

const subCancellation = number('456', '-');
subCancellation.sub(number('456', '-'));
assertNumber(subCancellation, '000', '+', '00', '+');

console.log('copy, nineComplement, shiftRight, digitadd, add und sub: Tests erfolgreich');
