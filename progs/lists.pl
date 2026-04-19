member(X,[X|_]).
member(X,[_|T]):-member(X,T).
append([],L,L).
append([H|T],L,[H|R]):-append(T,L,R).
length([],0).
length([_|T],N):-length(T,N1),N is N1+1.
last([X],X).
last([_|T],X):-last(T,X).
nth(1,[H|_],H).
nth(N,[_|T],X):-N>1,N1 is N-1,nth(N1,T,X).
reverse([],[]).
reverse([H|T],R):-reverse(T,RT),append(RT,[H],R).
