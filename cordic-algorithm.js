// Konstanten des MCS7529-014-ROMs, nach Dunkelwind / bITmASTER.
// Quelle: https://www.richis-lab.de/images/MCS7529/13.pdf
// digits: die zehn im ROM gespeicherten Dezimalziffern, einschließlich Nullen.
// value: zugehöriger Dezimalwert als String; Winkel im Bogenmaß.
// null: Bedeutung bzw. Dezimalpunktlage nicht gesichert.
// Die Exponenten sind nicht Bestandteil dieser zehn ROM-Ziffern.

const MCS7529_ROM_CONSTANTS = [
  { address:  0, digits: "0000006000", value: null,                 meaning: null },
  { address:  1, digits: "4342944819", value: "0.4342944819",       meaning: "log10(e)" },
  { address:  2, digits: "2302585093", value: "2.302585093",        meaning: "ln(10)" },
  { address:  3, digits: "6931471806", value: "0.6931471806",       meaning: "ln(2) = ln(1 + 10^0)" },
  { address:  4, digits: "9531017980", value: "0.09531017980",      meaning: "ln(1 + 10^-1)" },
  { address:  5, digits: "9950330853", value: "0.009950330853",     meaning: "ln(1 + 10^-2)" },
  { address:  6, digits: "9995003331", value: "0.0009995003331",    meaning: "ln(1 + 10^-3)" },
  { address:  7, digits: "9999500033", value: "0.00009999500033",   meaning: "ln(1 + 10^-4)" },
  { address:  8, digits: "9999950000", value: "0.000009999950000",  meaning: "ln(1 + 10^-5)" },
  { address:  9, digits: "9999995000", value: "0.0000009999995000", meaning: "ln(1 + 10^-6)" },
  { address: 10, digits: "9999999500", value: "0.00000009999999500", meaning: "ln(1 + 10^-7)" },
  { address: 11, digits: "9999999950", value: "0.000000009999999950", meaning: "ln(1 + 10^-8)" },
  { address: 12, digits: "9999999995", value: "0.0000000009999999995", meaning: "ln(1 + 10^-9)" },
  { address: 13, digits: "6283185308", value: "6.283185308",        meaning: "2*pi" },
  { address: 14, digits: "7853981634", value: "0.7853981634",       meaning: "pi/4 = atan(10^0)" },
  { address: 15, digits: "9966865249", value: "0.09966865249",      meaning: "atan(10^-1)" },
  { address: 16, digits: "9999666687", value: "0.009999666687",     meaning: "atan(10^-2)" },
  { address: 17, digits: "9999996667", value: "0.0009999996667",    meaning: "atan(10^-3)" },
  { address: 18, digits: "9999999967", value: "0.00009999999967",   meaning: "atan(10^-4)" },
  { address: 19, digits: "9999999999", value: "0.000009999999999",  meaning: "atan(10^-5)" },
  { address: 20, digits: "9999999999", value: "0.0000009999999999", meaning: "atan(10^-6)" },
  { address: 21, digits: "9999999999", value: "0.00000009999999999", meaning: "atan(10^-7)" },
  { address: 22, digits: "9999999999", value: "0.000000009999999999", meaning: "atan(10^-8)" },
  { address: 23, digits: "9999999999", value: "0.0000000009999999999", meaning: "atan(10^-9)" },
  { address: 24, digits: "5729577951", value: "5.729577951e9",      meaning: "1 / tan(10^-8 Grad)" },
  { address: 25, digits: "1745329252", value: "0.1745329252",       meaning: "10 Grad im Bogenmaß" },
  { address: 26, digits: "3141592654", value: "3.141592654",        meaning: "pi" },
  { address: 27, digits: "1570796327", value: "1.570796327",        meaning: "pi/2" },
  { address: 28, digits: "3600000000", value: "360",                meaning: "360 Grad" },
  { address: 29, digits: "0110000000", value: null,                 meaning: null },
  { address: 30, digits: "1100000000", value: null,                 meaning: null },
  { address: 31, digits: "1010000000", value: null,                 meaning: null },
];

// Einfaches dezimales Gleitkommaformat für unser Verständnis-Modell.
// Jedes Arrayelement ist ein Zeichen von "0" bis "9".
// Die Ziffern stehen von links nach rechts, höchstwertige Ziffer zuerst.
// Der Dezimalpunkt liegt gedanklich hinter der ersten Mantissenziffer.
// Beispiel: ["1", "2", "3", "4"], "+", ["0", "2"], "+" bedeutet 1,234 * 10^2.
// Keine automatische Normierung.
class DecimalNumber {
  constructor(n) {
    if (!Number.isInteger(n) || n < 1) {
      throw new RangeError("Die Mantisse benötigt eine positive ganze Ziffernanzahl.");
    }

    this.mantissa = Array(n).fill("0");
    this.mantissaSign = "+";
    this.exponent = ["0", "0"];
    this.exponentSign = "+";
  }

  copy() {
    const result = new DecimalNumber(this.mantissa.length);
    result.mantissa = this.mantissa.slice();
    result.mantissaSign = this.mantissaSign;
    result.exponent = this.exponent.slice();
    result.exponentSign = this.exponentSign;
    return result;
  }

  // Verschiebt die Mantisse um n Stellen nach rechts und erhöht den Exponenten.
  // Die letzte Mantissenziffer fällt bei jedem Einzelschritt weg.
  shiftRight(n) {
    if (!Number.isInteger(n) || n < 0) {
      throw new RangeError("Die Verschiebungsweite muss eine nichtnegative ganze Zahl sein.");
    }

    const mantissa = this.mantissa.slice();
    let exponent = this.exponent.slice();
    let exponentSign = this.exponentSign;

    for (let i = 0; i < n; i++) {
      mantissa.unshift("0");
      mantissa.pop();
      ({ exponent, exponentSign } = increaseExponent(exponent, exponentSign));
    }

    this.mantissa = mantissa;
    this.exponent = exponent;
    this.exponentSign = exponentSign;
    return this;
  }

  // Ersetzt jede Mantissenziffer durch ihr Neunerkomplement.
  nineComplement() {
    this.mantissa = this.mantissa.map(digit => NINES_COMPLEMENT_TABLE[digit]);
    return this;
  }

  // Addiert other zu dieser Zahl und speichert das Ergebnis in dieser Instanz.
  add(other) {
    if (!(other instanceof DecimalNumber)) {
      throw new TypeError("add erwartet eine DecimalNumber.");
    }
    if (this.mantissa.length !== other.mantissa.length) {
      throw new RangeError("Beide Zahlen müssen gleich viele Mantissenziffern haben.");
    }

    const left = this.copy();
    const right = other.copy();
    let exponentOrder = compareExponents(left, right);

    while (exponentOrder < 0) {
      left.shiftRight(1);
      exponentOrder = compareExponents(left, right);
    }
    while (exponentOrder > 0) {
      right.shiftRight(1);
      exponentOrder = compareExponents(left, right);
    }

    let resultMantissa;
    let resultSign;
    let resultExponent = left.exponent.slice();
    let resultExponentSign = left.exponentSign;

    if (left.mantissaSign === right.mantissaSign) {
      resultMantissa = Array(left.mantissa.length);
      let carry = 0;

      for (let i = resultMantissa.length - 1; i >= 0; i--) {
        const digit = digitadd(left.mantissa[i], right.mantissa[i], carry);
        resultMantissa[i] = digit.digit;
        carry = digit.overflow;
      }

      resultSign = left.mantissaSign;
      if (carry === 1) {
        resultMantissa = ["1", ...resultMantissa.slice(0, -1)];
        const increased = increaseExponent(resultExponent, resultExponentSign);
        resultExponent = increased.exponent;
        resultExponentSign = increased.exponentSign;
      }
    } else {
      const leftDigits = left.mantissa.join("");
      const rightDigits = right.mantissa.join("");

      if (leftDigits === rightDigits) {
        resultMantissa = Array(left.mantissa.length).fill("0");
        resultSign = "+";
      } else {
        const leftIsLarger = leftDigits > rightDigits;
        const larger = leftIsLarger ? left : right;
        const smaller = leftIsLarger ? right : left;
        const difference = subtractDigits(larger.mantissa, smaller.mantissa);
        if (difference.borrow === 1) throw new Error("Interner Fehler: unerwarteter Unterlauf.");
        resultMantissa = difference.digits;
        resultSign = larger.mantissaSign;
      }
    }

    this.mantissa = resultMantissa;
    this.mantissaSign = resultMantissa.every(digit => digit === "0") ? "+" : resultSign;
    this.exponent = resultExponent;
    this.exponentSign = resultExponentSign;
    return this;
  }

  // Subtrahiert other von dieser Zahl und speichert das Ergebnis hier.
  sub(other) {
    if (!(other instanceof DecimalNumber)) {
      throw new TypeError("sub erwartet eine DecimalNumber.");
    }
    const negatedOther = other.copy();
    negatedOther.mantissaSign = negatedOther.mantissaSign === "+" ? "-" : "+";
    return this.add(negatedOther);
  }
}

// Zeile = erste Ziffer, Spalte = zweite Ziffer; Eintrag = Ergebnisziffer.
const DIGIT_ADD_TABLE = [
  "0123456789",
  "1234567890",
  "2345678901",
  "3456789012",
  "4567890123",
  "5678901234",
  "6789012345",
  "7890123456",
  "8901234567",
  "9012345678",
];

// Addiert zwei Dezimalziffern und einen eingehenden Übertrag (0 oder 1).
// Die Ziffern sind Zeichen; der ausgehende Übertrag ist eine Zahl.
function digitadd(a, b, carry) {
  if (carry !== 0 && carry !== 1) {
    throw new RangeError("digitadd erwartet einen Übertrag von 0 oder 1.");
  }

  const first = DIGIT_ADD_TABLE[a][b];
  const digit = DIGIT_ADD_TABLE[first][carry];
  const overflow = first < a || digit < first ? 1 : 0;
  return { digit, overflow };
}

// Neunerkomplement: Index ist Ziffer, Eintrag ist 9 minus Ziffer.
const NINES_COMPLEMENT_TABLE = "9876543210";

// Subtrahiert gleich lange Ziffernarrays per Zehnerkomplement.
// Bei Unterlauf enthält digits das Ergebnis modulo 10^n und borrow ist 1.
function subtractDigits(a, b) {
  if (a.length !== b.length) {
    throw new RangeError("Für subtractDigits müssen die Arrays gleich lang sein.");
  }

  const digits = Array(a.length);
  let carry = 1;

  for (let i = a.length - 1; i >= 0; i--) {
    const complement = NINES_COMPLEMENT_TABLE[b[i]];
    const result = digitadd(a[i], complement, carry);
    digits[i] = result.digit;
    carry = result.overflow;
  }

  return { digits, borrow: carry === 0 ? 1 : 0 };
}

function increaseExponent(exponent, exponentSign) {
  const digits = exponent.slice();
  let sign = exponentSign;

  if (sign === "-") {
    if (digits.every(digit => digit === "0")) {
      sign = "+";
    } else {
      const result = subtractDigits(digits, Array(digits.length - 1).fill("0").concat("1"));
      if (result.borrow === 1) throw new RangeError("Der Exponent kann nicht weiter verringert werden.");
      digits.splice(0, digits.length, ...result.digits);

      if (digits.every(digit => digit === "0")) sign = "+";
      return { exponent: digits, exponentSign: sign };
    }
  }

  for (let i = digits.length - 1; i >= 0; i--) {
    const result = digitadd(digits[i], "1", 0);
    digits[i] = result.digit;
    if (result.overflow === 0) return { exponent: digits, exponentSign: sign };
  }

  throw new RangeError("Der Exponent passt nicht mehr in seine Ziffernstellen.");
}

function compareExponents(a, b) {
  const aDigits = a.exponent.join("");
  const bDigits = b.exponent.join("");
  const aSign = aDigits === "0".repeat(a.exponent.length) ? "+" : a.exponentSign;
  const bSign = bDigits === "0".repeat(b.exponent.length) ? "+" : b.exponentSign;

  if (aSign !== bSign) return aSign === "+" ? 1 : -1;
  if (aDigits === bDigits) return 0;

  const magnitudeOrder = aDigits > bDigits ? 1 : -1;
  if (aSign === "+") return magnitudeOrder;
  return magnitudeOrder === 1 ? -1 : 1;
}
