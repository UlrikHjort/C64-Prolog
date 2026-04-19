fact(0,1):-!.
fact(N,F):-N>0,N1 is N-1,fact(N1,F1),F is N*F1.
fib(0,0):-!.
fib(1,1):-!.
fib(N,F):-N>1,N1 is N-1,N2 is N-2,fib(N1,F1),fib(N2,F2),F is F1+F2.
gcd(X,0,X):-!.
gcd(X,Y,G):-Y>0,R is X mod Y,gcd(Y,R,G).
between(L,H,L):-L=<H.
between(L,H,X):-L<H,L1 is L+1,between(L1,H,X).
abs(X,X):-X>=0,!.
abs(X,A):-A is -X.
