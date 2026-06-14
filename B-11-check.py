import math


"""
Sind X und Y zwei reelle, integrierbare Zufallsvariablen,
deren Produkt ebenfalls integrierbar ist, d. h.,
die Erwartungswerte E(X), E(Y) und E(X * Y) existieren, dann heißt

    Cov(X, Y) := E( (X − E(X)) * (Y − E(Y)) ) 

die Kovarianz von X und Y.
Die Kovarianz ist also das Produkt der Differenzen je zwischen X und Y und ihren Erwartungswerten.
In der Statistik werden E(X) und E(Y) als arithmetische Mittelwerte berechnet.
"""


"""
Verschiebungssatz

Zur oft einfacheren Berechnung der Kovarianz kann man auch den Verschiebungssatz als alternative Darstellung der Kovarianz anwenden.

Satz (Verschiebungssatz für die Kovarianz):

    Cov(X, Y) = E(X*Y) − E(X)*E(Y)

Beweis:

       Cov(X, Y) = E[ ( X − E ⁡ ( X ) ) ⋅ ( Y − E ⁡ ( Y ) ) ]
    =  E [ ( X Y − X E ⁡( Y ) − Y E ⁡( X ) + E ⁡( X ) E( Y ) ) ]
    =  E ⁡( X Y ) − E ⁡( X ) E ⁡( Y ) − E ⁡( Y ) E ( X ) + E ⁡( X ) E ⁡( Y )
    =  E ⁡( X Y ) − E ⁡( X ) E ⁡( Y ) ◻ 

"""
x = [0]*5
y = [0]*5

n = 5

x[0] = 4; y[0] = 6
x[1] = 5; y[1] = 9
x[2] = 1; y[2] = 4
x[3] = 7; y[3] = 3
x[4] = 2; y[4] = 2

print("product_sum", 4*6 + 5*9 + 1*4 + 7*3 + 2*2, sum([ a*b for a,b in zip(x,y)]))
      

def naive_covariant(X,Y,n):
    print("n given:", n)
    sxy = sum( [ X[i]*Y[i] for i in range(n)] )
    sx =  sum(x)
    sy =  sum(y)
    
    Sxy = 1.0/(n-1) * ( sxy - 1.0/n * sx * sy )
    
    sx2 = sum( t*t for t in X )
    sy2 = sum( t*t for t in Y )

    Sx = math.sqrt( ( sx2 - sx*sx/n ) / (n-1))

    Sy = math.sqrt( ( sy2 - sy*sy/n ) / (n-1))

    r = Sxy / (Sx * Sy)

    print("Sxy",Sxy)
    print("r",r)

    return Sxy

def E(a):
    s = 0
    for x in a:
        s += x
    return s/len(a)

def Cov(X,Y):
    print("n given:", n)
    # Cov(X, Y) := E( (X − E(X)) * (Y − E(Y)) )
    Ex = E(X)
    Ey = E(Y)
    product = [ (a - Ex) * (b - Ey) for a,b in zip(X,Y) ]
    return E(product)

def Cov2(X,Y,n):
    print("n given:", n)
    n = len(X)
    print("n calculated:", n)
    # Cov(X, Y) = E ⁡( X Y ) − E ⁡( X ) E ⁡( Y )    
    Ex = E(X)
    Ey = E(Y)
    product = [ a*b for a,b in zip(X,Y) ]
    return E(product) - Ex * Ey

def S(X):
    return sum(X)

def Cov3(X,Y,n):
    print("n given", n)
    n = len(X)
    print("n calculated:", n)
    
    product = [ a*b for a,b in zip(X,Y) ]
    Sxy = 1.0/n * (S(product) - 1.0/n *( S(X) * S(Y)))
    return Sxy

def Cov_Bessel(X,Y,n):
    Ex = E(X)
    Ey = E(Y)
    product = [ (x - Ex)*(y-Ey) for x,y in zip(X,Y) ]
    
    return 1.0/(n-1) * S(product)
    
def Cov_Bessel_sim(X,Y,n):
    product = [ x*y for x,y in zip(X,Y) ]


    print("Cov_Bessel_sim")
    print("S(p)", S(product))
    print("S(X), S(Y)", S(X), S(Y))    

    print("1/(n-1) * ( S(p) - 1.0/n * S(X) * S(Y)")

    print("Berechnung:")
    print(S(product), "STO 0")
    print(S(X), "STO 1")
    print(S(Y), "STO 2")
    print(n, "STO 5")

    print("rcl 0 - ( rcl 1 * rcl 2 / rcl 5 = / (rcl 5 - 1 =")
    
    return 1.0/(n-1) * ( S(product) - 1.0/n * S(X) *S(Y))


# print(E([1,2,3,4,5,6]))

print("Cov:", Cov(x,y))

print("Cov-naive:", naive_covariant(x,y,n))

print("Cov2:", Cov2(x,y,n))

print("Cov3:", Cov3(x,y,n))

print("Cov_Bessel:", Cov_Bessel(x,y,n))

print("Cov_Bessel_sim:", Cov_Bessel_sim(x,y,n))
    
