import Tkinter
import tkMessageBox
import os

top = Tkinter.Tk()

def helloCallBack():
   os.startfile(r"C:\Users\fdutina\Desktop\vs2017_PC_side\PCside\Debug\PCside.exe")

B = Tkinter.Button(top, text ="S  T  A  R  T", activebackground = "red", command = helloCallBack, height = 7, width = 30)


B.pack()

top.mainloop()