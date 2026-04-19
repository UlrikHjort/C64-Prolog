move(F,T):-write(F),write(->),write(T),nl.
hanoi(N):-hanoi(N,left,right,mid).
hanoi(1,F,T,_):-!,move(F,T).
hanoi(N,F,T,V):-N>1,N1 is N-1,hanoi(N1,F,V,T),move(F,T),hanoi(N1,V,T,F).
