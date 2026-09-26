#Project by HF
#This project aimed to find a series of n*n squares filled up with black
#and white stones which make the whole plane the squares made has no
#any five or more same-colored stones in a line continuously.

#Future aims are to find n*m squares
#and more or less than four as the maxium continue stones.

#aim next: reduce RAM cost and fill CPU usage

from os import system, cpu_count
from time import sleep
from multiprocessing import Pool
from copy import deepcopy
while __name__=="__main__":
     try:
          from tqdm import tqdm
          from datetime import datetime
          break
     except Exception:
          system("pip install tqdm")
          system("cls")
while __name__=="__main__":
     try:
          from filelock import FileLock
          break
     except Exception:
          system("pip install filelock")
          system("cls")
from filelock import FileLock

def Extend(base):
     SL=len(base)
     #Record side lenth as SL
     '''
     j\i1   2   3   4   5   6   7   8   9
     1  x   x   x   x   x   x   x   x   x
     2  x   x   x   x   x   x   x   x   x
     3  x   x   x   x   x   x   x   x   x
     4  x   x   x   o   o   o   x   x   x
     5  x   x   x   o   o   o   x   x   x
     6  x   x   x   o   o   o   x   x   x
     7  x   x   x   x   x   x   x   x   x
     8  x   x   x   x   x   x   x   x   x
     9  x   x   x   x   x   x   x   x   x
     '''
     a=[[None for i in range(SL*3+1)] for j in range(SL*3+1)]
     for i in range(1,3*SL+1):
          for j in range(1,3*SL+1):
               i1=i
               j1=j
               if i in range (1,SL+1):
                    i1+=SL
               elif i in range(2*SL+1,3*SL+1):
                    i1-=SL
               if j in range (1,SL+1):
                    j1+=SL
               elif j in range(2*SL+1,3*SL+1):
                    j1-=SL
               a[i][j]=base[i1-SL-1][j1-SL-1]
     return a

def Certify(r):
     SL=len(r)
     #Record side lenth plus 1 as SL
 
     #Below checks horizontal -
     for j in range(1,SL):
          alart=0
          state=None
          for i in range(1,SL):
               if state!=r[i][j]:
                    alart=1
                    state=r[i][j]
               else:
                    alart+=1
               if alart>=5:
                    return False

     #Below checks vertical |
     for i in range(1,SL):
          alart=0
          state=None
          for j in range(1,SL):
               if state!=r[i][j]:
                    alart=1
                    state=r[i][j]
               else:
                    alart+=1
               if alart>=5:
                    return False
     
     #Below checks /
     for j in range (1,SL):
          alart=0
          state=None
          y=j
          x=1
          while y>=1:
               if state!=r[x][y]:
                    alart=1
                    state=r[x][y]
               else:
                    alart+=1
               if alart >=5:
                    return False
               x+=1
               y-=1
     for j in range (1,SL):
          alart=0
          state=None
          y=j
          x=SL-1
          while y<=SL-1:
               if state!=r[x][y]:
                    alart=1
                    state=r[x][y]
               else:
                    alart+=1
               if alart >=5:
                    return False
               x-=1
               y+=1
     #Below checks \
     for j in range (1,SL):
          alart=0
          state=None
          y=j
          x=1
          while y<=SL-1:
               if state!=r[x][y]:
                    alart=1
                    state=r[x][y]
               else:
                    alart+=1
               if alart >=5:
                    return False
               x+=1
               y+=1
     for j in range (1,SL):
          alart=0
          state=None
          y=j
          x=SL-1
          while y>=1:
               if state!=r[x][y]:
                    alart=1
                    state=r[x][y]
               else:
                    alart+=1
               if alart >=5:
                    return False
               x-=1
               y-=1
     return True

def TakeDown(S):
     while True:
          try:
               Lock=FileLock("Results.txt")
               with open("Results.txt","a") as File:
                    #S1=deepcopy(S)
                    File.write("-"*len(S)+"Ver0.5"+"-"*len(S)+"\n")
                    for i in S:
                         for j in i:
                              if j:
                                   File.write("■")
                              else:
                                   File.write("□")
                         File.write("\n")
               break
          except:
               sleep(0.5)
          finally:
               Lock.release()

def OnePossible(Square):
     if Certify(Extend(Square))==True:
          #print(Square)
          TakeDown(Square)
     #else: print(Esuqare,False)

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

if __name__=="__main__":
     N=int(input("Gobang Board v0.5\nMade by HF\nSide length = "))
     if N<2: raise ValueError
     '''
     This test is not useful.
     a=[[True,False,True,True,True],
        [True,True,True,False,True],
        [False,True,True,True,True],
        [True,True,False,True,True],
        [True,True,True,True,False]]
     OnePossible(a)
     '''
     print("Preprocessing...",end="")
     Pro=Pool(processes=cpu_count())
     Inputs=Encode2(N,1,Encode1(N,1))
     print("\r"+" "*16,end="")
     #print(Inputs[0],Inputs[-1],len(Inputs))
     #system('pause')
     #Inputs=[("{:0"+str(N**2)+"b}").format(i) for i in range(int(2**(N**2)))]
     #Outputs=list(tqdm(Pro.imap(Generate,Inputs),total=int(2**(N**2))))
     #Inputs=Encode(N,1)
     list(tqdm(Pro.imap(OnePossible,Inputs),total=int(2**(N**2))))
     R='Results '+str(datetime.now().date()) +f" {datetime.now().hour}-{datetime.now().minute}-{datetime.now().second}"
     system(f"rename results.txt \"{R}.txt\"")
     del Pro, Inputs
     system("pause")
