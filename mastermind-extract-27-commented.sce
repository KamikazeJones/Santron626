# Kommentierte Extraktionsroutine: Ziffer an Stelle 10^M4 aus M8
#
# Vorbedingungen:
# - M4 = n, die gesuchte Dezimalstelle
# - M8 = codierter CODE
#
# Wirkung:
# - M7 = Ziffer an Stelle 10^M4 aus M8
# - M4 und M8 bleiben erhalten
# - M6 wird als Scratch-Speicher benutzt und zerstoert
#
# Idee:
# Sei q = M8 / 10^M4.
# Die Routine bildet zuerst T = 1E9 + floor(q).
# Danach erzeugt sie durch Addition eines grossen Vielfachen von T einen
# Rundungsverlust, der die letzte Dezimalziffer von floor(q) abschneidet.
# Die Differenz aus T und diesem abgeschnittenen Wert ist die gesuchte Ziffer.

:LOAD

# M6 = 10. Dieser Faktor wird gleich benutzt, um einen gezielten
# Signifikanzverlust zu erzwingen. M6 wird bei jedem Aufruf neu gesetzt,
# damit die Routine wiederholt aufgerufen werden kann.
1 0 STO 6

# q = M8 / 10^M4.
# Das anschliessende + schliesst die Division ab und laesst einen
# offenen Additionsoperator stehen.
RCL 8 / RCL 4 10^X +

# X = 1E9.
# Wegen des offenen + wird beim folgenden + gerechnet:
# T = q + 1E9.
# Durch die 10-stellige interne Genauigkeit wird daraus:
# T = 1E9 + floor(q)
EXP 0 9 +

# T in M7 sichern.
STO 7

# M6 = 10 * T.
# X bleibt dabei T, und der offene Additionsoperator bleibt erhalten.
M* 6

# Jetzt wird mit dem offenen + zuerst T + M6 gebildet.
# Weil M6 = 10*T ist, entsteht 11*T. Bei der internen 10-stelligen
# Genauigkeit geht dabei die letzte Ziffer von T verloren.
#
# Beispiel T = 1000000304:
# 11*T = 11000003344 wird intern zu 11000003340,
# minus 10*T ergibt 1000000300.
RCL 6 - RCL 6 =

# M7 war T. Jetzt wird der abgeschnittene Wert abgezogen.
# Ergebnis: letzte Ziffer von floor(q).
M- 7

R/S

list;
