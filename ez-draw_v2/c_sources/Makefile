# **************************************************************************** #
#                                                                              #
#                                                                              #
#    Makefile    for windows and linux (or unix like)                          #
#                                                                              #
#    By: Marpon											                       #
#                                                                              #
#    Created: 2017/03/11                                                       #
#    Updated: 2019/16/07                                                       #
#                                                                              #
# **************************************************************************** #

# use under  ez-draw2 ; ez-image2 or ez-plus2 (according the lib to create)

NAME = ez-plus2



# use under  32   or  64      (according the right OS)   

BITS = 64

#-std=gnu99
CFLAGSX =  -Wall -pedantic  -m$(BITS)

#to detect if it is windows OS or not(multi_platform)
ifeq ($(shell echo "_"),"_")  	#windows OS
	DO_MKDIR = @if not exist $@ mkdir $@
	MY_OS =	w
	DQ =
	NL = @echo. 
else							#other OS linux (or unix like) 
	DO_MKDIR = @test -d $@ || mkdir $@ 
	MY_OS =	l
	DQ = "
	NL = @echo ""
endif

INC = .

SRC	= .

OBJ	= obj_d

ifeq ($(NAME), ez-plus2)
	SRCS = ez_capture2.c save_bmp.c save_jpg.c
else
	SRCS = $(NAME).c
endif


LIBZ = lib$(NAME)_$(MY_OS)$(BITS).a

COBJS = $(SRCS:.c=.o)

OBJ_F	= $(addprefix $(OBJ)/,$(COBJS))

DEP_F	= $(OBJ_F:.o=.d)
 
OBJ_DIR = $(OBJ)

all: $(OBJ_DIR) $(NAME) finish
		
$(OBJ_DIR):
	$(DO_MKDIR)
		

$(NAME): $(OBJ_F)
	$(NL)
	@echo $(DQ)  Creating lib : $@ $(DQ)
	@echo $(DQ)         using : $+ $(DQ)
	$(NL)	
	@ar rc $(LIBZ) $(OBJ_F)
	@ranlib $(LIBZ)
	@echo $(DQ)           Lib : $(LIBZ) successfully created!$(DQ)
	$(NL)
	
$(OBJ_F): $(OBJ)/%.o: $(SRC)/%.c Makefile
	$(NL)
	@echo $(DQ)  Compiling : $< to $@ $(DQ)
	@gcc -c -MMD -MP $(CFLAGSX) -o $@ $< -I $(INC)
	$(NL)
	
-include $(DEP_F)

finish:
	$(NL)
	@echo $(DQ) Make terminating with : $(LIBZ)!$(DQ)
	$(NL)
	
clean:
	$(NL)
	@echo $(DQ) Deleting object/dependances files and removing : $(OBJ_DIR)$(DQ)
	@rm -rf $(OBJ_F)
	@rm -rf $(DEP_F)
	@rm -rf $(OBJ_DIR)
	$(NL)	
	
fclean: clean
	$(NL)
	@echo $(DQ) Deleting  : $(LIBZ)$(DQ)
	@rm -rf $(LIBZ)
	$(NL)
	
re: fclean all



.PHONY: all clean fclean re

