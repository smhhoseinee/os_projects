import tkinter as tk
from tkinter import filedialog

from ctypes import *
import ctypes

def select_dir():
    root = tk.Tk()
    root.withdraw()
    path = filedialog.askdirectory()
    return path


python_string = select_dir()



OS = cdll.LoadLibrary('/home/vboxuser/cpp workspace/exampe/.dist/OS.so')


c_string = ctypes.c_char_p(python_string.encode('utf-8'))

OS.exmain(c_string)