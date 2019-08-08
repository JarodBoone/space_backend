# Makefile for the soundmap c++ file 
CC := gcc 
CFLAGS := -g 

# Should not be doing this manually 
INCLUDE :=  -I/home/jarod/space/lib/soundmap/include
LINK := -L/usr/local/lib -L/usr/lib/python3.6/config-3.6m-x86_64-linux-gnu -lpthread -lm

# Compilation pipeline 
SRC := soundmap.c profiler.c transform.c 
OBJ := $(addsuffix .o,$(basename $(SRC)))
EXEC := soundmap

.PHONY : all clean

$(EXEC) : $(OBJ)
	@ $(CC) $(CFLAGS)  $(INCLUDE)  $^ -o $@ $(LINK)

$(OBJ) : $(SRC)
	@ $(CC) $(CFLAGS) $(INCLUDE) -c $^ 

clean : 
	@ rm -rf $(EXEC) $(OBJ) 