from os import system, getpid
from time import sleep
from filelock import FileLock


i=0
while True:
     try:
          Lock=FileLock("abc.txt")
          with open(r"abc.txt",'a') as file:
               print("Start",getpid())
               file.write(__name__+str(getpid())+"\n")
               file.write(__name__+str(getpid()+1)+"\n")
               file.write(__name__+str(getpid()+2)+"\n")
               file.write(__name__+str(getpid()+3)+"\n")
               file.write(__name__+str(getpid()+4)+"\n")
               print("Over",getpid())
               #system("pause")
               #sleep(0.05)
     finally:
          Lock.release()
     i+=1