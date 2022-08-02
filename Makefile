FILE = expand.c
OUT = expand

all : $(FILE)
	gcc-11 $(FILE) -w -o $(OUT)
