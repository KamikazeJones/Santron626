import argparse
import random
import sys


"""
Mastermind-Bewertung fuer Ziffern von 1 bis 6.

code:  der zu ratende Code
guess: der vom Benutzer eingegebene, geratene Code

s: Anzahl der schwarzen Stifte, also richtige Ziffer an richtiger Position
w: Anzahl der weissen Stifte, also richtige Ziffer an falscher Position

Die weisse Bewertung nutzt codierte Vorkommens-Arrays. Die Stelle 10^n zaehlt,
wie oft die Ziffer n in den nicht-schwarzen Resten vorkommt.
"""


MIN_LEN = 2
MAX_LEN = 7
DEFAULT_LEN = 4
DIGITS = "123456"


def evaluate(code, guess, length=None):
    code = str(code)
    guess = str(guess)
    if length is None:
        length = len(code)
    if len(code) != length or len(guess) != length:
        raise ValueError("code and guess must have the requested length")

    black = 0
    white = 0
    code_counts = 0
    guess_counts = 0

    for code_digit, guess_digit in zip(code, guess):
        if code_digit == guess_digit:
            black += 1
        else:
            code_counts += pow(10, int(code_digit))
            guess_counts += pow(10, int(guess_digit))

    for digit in range(6, 0, -1):
        code_count = int(code_counts / pow(10, digit))
        guess_count = int(guess_counts / pow(10, digit))
        white += min(code_count, guess_count)
        code_counts -= code_count * pow(10, digit)
        guess_counts -= guess_count * pow(10, digit)

    return black, white


def random_value(length):
    return "".join(random.choice(DIGITS) for _ in range(length))


def validate_value(name, value, length):
    if value is None:
        return None
    value = str(value)
    if len(value) != length:
        raise ValueError(f"{name} must have exactly {length} digits")
    invalid = sorted(set(value) - set(DIGITS))
    if invalid:
        raise ValueError(f"{name} may only contain digits 1..6")
    return value


def build_parser():
    parser = argparse.ArgumentParser(
        description=(
            "Bewertet einen Mastermind-Code gegen einen Guess. "
            "Doppelte Ziffern sind erlaubt; weisse Treffer werden ueber "
            "Vorkommens-Arrays korrekt begrenzt."
        )
    )
    parser.add_argument("--code", help="geheimer Code, z.B. 1334")
    parser.add_argument("--guess", help="geratener Code, z.B. 3131")
    parser.add_argument(
        "--random",
        action="store_true",
        help="setzt fehlenden Code und/oder Guess zufaellig",
    )
    parser.add_argument(
        "--len",
        type=int,
        default=DEFAULT_LEN,
        choices=range(MIN_LEN, MAX_LEN + 1),
        metavar=f"{MIN_LEN}..{MAX_LEN}",
        help=f"Anzahl der Ziffern, Standard ist {DEFAULT_LEN}",
    )
    parser.add_argument(
        "--repeat",
        type=int,
        default=1,
        help="wiederholt ein Zufallsexperiment n mal",
    )
    return parser


def run_experiment(args):
    code = validate_value("code", args.code, args.len)
    guess = validate_value("guess", args.guess, args.len)

    random_sets_code = args.random and code is None
    random_sets_guess = args.random and guess is None
    random_sets_anything = random_sets_code or random_sets_guess

    if args.repeat < 1:
        raise ValueError("--repeat must be at least 1")
    if args.repeat != 1 and not random_sets_anything:
        raise ValueError("--repeat requires --random to set code or guess")
    if code is None and not args.random:
        raise ValueError("--code is required unless --random is used")
    if guess is None and not args.random:
        raise ValueError("--guess is required unless --random is used")

    for _ in range(args.repeat):
        current_code = random_value(args.len) if random_sets_code else code
        current_guess = random_value(args.len) if random_sets_guess else guess
        black, white = evaluate(current_code, current_guess, args.len)
        print(f"code={current_code} guess={current_guess} black={black} white={white}")


def main(argv=None):
    parser = build_parser()
    argv = sys.argv[1:] if argv is None else argv
    if not argv:
        parser.print_help()
        return 0

    args = parser.parse_args(argv)
    try:
        run_experiment(args)
    except ValueError as exc:
        parser.error(str(exc))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

