SDK_PATH = $(HOME)/ciq
KEY = developer_key.der

NAME = zspotlight
APP_ID = 9ed3d39b-8b52-4f41-94e5-8b1428449871

JUNGLE = monkey.jungle
ifeq ($(DEVICE),)
DEVICE = fr955
endif

MONKEYC = $(SDK_PATH)/bin/monkeyc
MONKEYDO = $(SDK_PATH)/bin/monkeydo

.PHONY: all clean sim graph

all: $(NAME).prg

clean:
	rm -f $(NAME)-fit_contributions.json
	rm -f $(NAME).iq
	rm -f $(NAME).prg
	rm -f $(NAME).prg.debug.xml
	rm -rf mir internal-mir gen

$(NAME).prg: manifest.xml resources/*.xml source/*.mc
	$(MONKEYC) -d $(DEVICE) -f $(JUNGLE) -o $(NAME).prg -y $(KEY)

$(NAME).iq: manifest.xml resources/*.xml source/*.mc
	$(MONKEYC) -e -f $(JUNGLE) -o $(PWD)/$(NAME).iq -y $(KEY)

sim: $(NAME).prg
	$(MONKEYDO) $(NAME).prg $(DEVICE)

graph:
	java -jar $(SDK_PATH)/bin/fit-graph.jar

era:
	$(SDK_PATH)/bin/era -k $(KEY) -a $(APP_ID)
