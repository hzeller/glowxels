# -*- Makefile -*-

FABS=glowxels-fab.zip

all : $(FABS)

%.zip : %.kicad_pcb
	python ../kicad-scripts/kicad-fab.py $<
	zip -r $@ fab

%-fab.kicad_pcb : %.kicad_pcb
	sed "s/%%gitversion%%/`git log --date=short --pretty=format:'%h@%cd' -n 1 $<`/" < $^ > $@

clean:
	rm -rf $(FABS) fab
