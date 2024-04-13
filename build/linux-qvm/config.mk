<<<<<<< HEAD
PK3 = zz-patch04.pk3
=======
PK3 = pak000.pk3
>>>>>>> edc5c58466dc0d3c07f7e5e5e367f892c64c75bf

basedir = ../../code

QADIR = $(basedir)/game
CGDIR = $(basedir)/cgame
UIDIR = $(basedir)/q3_ui

Q3ASM = q3asm -vq3 -r -m -v
Q3LCC = q3lcc -DQ3_VM -S -Wf-g -I$(QADIR)
7Z = 7z u -tzip -mx=9 -mpass=8 -mfb=255 --

QA_CFLAGS = -DQAGAME
CG_CFLAGS = -DCGAME -I$(CGDIR)
UI_CFLAGS = -DQ3UI -I$(UIDIR)

include srcs.mk
