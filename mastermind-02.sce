# Mastermind - Ansatz 02
#
# Idee:
# Vielleicht muss die erste Ziffer fuer den Vergleich gar nicht einzeln
# isoliert werden. Stattdessen koennte man versuchen, Code und Guess direkt
# voneinander abzuziehen und aus der Differenz etwas ueber die Bewertung
# abzuleiten.
#
# Beispiel 1:
#   Code  = 4589
#   Guess = 4225
#   4589 - 4225 =  364
#   4225 - 4589 = -364
#
# Beispiel 2:
#   Code  = 6123
#   Guess = 5789
#   6123 - 5789 =  334
#   5789 - 6123 = -334
#
# Problem:
# Die reine Differenz scheint nicht auszureichen, um die verglichenen
# Ziffernfolgen eindeutig zu unterscheiden. Selbst wenn das Vorzeichen
# erhalten bleibt, geht zu viel Struktur verloren.
#
# Offene Frage:
# Gibt es eine abgeleitete Groesse, die mehr Information bewahrt als die
# einfache Differenz, aber trotzdem billiger ist als das Isolieren jeder
# einzelnen Ziffer?

