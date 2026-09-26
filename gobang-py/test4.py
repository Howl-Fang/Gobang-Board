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

R='Results '+str(datetime.now().date()) +f" {datetime.now().hour}-{datetime.now().minute}-{datetime.now().second}"
B=f"rename results.txt \"{R}.txt\""
print(B)
system(B)
system("pause")