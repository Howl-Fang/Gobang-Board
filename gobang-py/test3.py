from sys import getsizeof

def Encode1(SL,turn):
     #Here uses nest subroutine to generate a SL length True-or-False list
     if turn>=SL:
          return [[True],[False]]
     else:
          V=[]
          for u in Encode1(SL,turn+1):
               V.append(u+[True])
               V.append(u+[False])
          return V
     
def Encode2(SL,turn,L):
     #Here uses nest subroutine to generate a SL length a list Encode1 creates
     if turn>=SL:
          return [[i] for i in L]
     else:
          V=[]
          for u in Encode2(SL,turn+1,L):
               for w in range(int(2**SL)):
                    V.append(u+[L[w]])
          return V
     
N=int(input("N="))
Inputs1=Encode2(N,1,Encode1(N,1))
print(getsizeof(Inputs1))
del Inputs1
Inputs2=[("{:0"+str(N**2)+"b}").format(i) for i in range(int(2**(N**2)))]
print(getsizeof(Inputs2))
del Inputs2
input()