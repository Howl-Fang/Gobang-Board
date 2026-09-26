def Encode(SL,turn):
     if turn>=SL**2:
          return [[True],[False]]
     else:
          V=[]
          for u in Encode(SL,turn+1):
               V.append(u+[True])
               V.append(u+[False])
          return V
     
n=int(input())
m=Encode(n,1)
print(m)
input()
del m,n
input('del')