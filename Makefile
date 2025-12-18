BIN_FILE_1=digger_tpc.bin
BIN_FILE_2=digger_tit.bin
OUT_FILE_1=digger_tpc.out
OUT_FILE_2=digger_tit.out
OPT_FLAG=-O2
XGCC=/home/prcoder/xgcc
GCC_FLAGS=-std=gnu23 -fomit-frame-pointer -msoft-float -fcprop-registers -fPIC -nostartfiles -nodefaultlibs -nostdlib -m10 $(OPT_FLAG) -I$(XGCC)/include
GCC_ASM_FLAGS=-S -fverbose-asm
AS_FLAGS=-mno-fpu -mlimited-eis -pic
GMPI_API_URL=http://10.0.0.55/api
GMPI_UPLOAD_DIR=/BK_Uploads

all: asm-files bin-files

asm-files: digger.c sprites.c tools.c
	pdp11-aout-gcc ${GCC_FLAGS} ${GCC_ASM_FLAGS} digger.c
	pdp11-aout-gcc ${GCC_FLAGS} ${GCC_ASM_FLAGS} sprites.c
	pdp11-aout-gcc ${GCC_FLAGS} ${GCC_ASM_FLAGS} tools.c

digger-main-file: digger.c
	pdp11-aout-gcc ${GCC_FLAGS} -c -o digger.o digger.c

title-main-file: title.c
	pdp11-aout-gcc ${GCC_FLAGS} -c -o title.o title.c

levels-file: digger_levels.c
	pdp11-aout-gcc ${GCC_FLAGS} -c -o digger_levels.o digger_levels.c

short-font-file: digger_short_font.c
	pdp11-aout-gcc ${GCC_FLAGS} -c -o digger_short_font.o digger_short_font.c

full-font-file: digger_full_font.c
	pdp11-aout-gcc ${GCC_FLAGS} -c -o digger_full_font.o digger_full_font.c

sprites-file: digger_sprites.c
	pdp11-aout-gcc ${GCC_FLAGS} -c -o digger_sprites.o digger_sprites.c

sprites-file-title: digger_sprites_title.c
	pdp11-aout-gcc ${GCC_FLAGS} -c -o digger_sprites_title.o digger_sprites_title.c

music-file-title: digger_music_title.c
	pdp11-aout-gcc ${GCC_FLAGS} -c -o digger_music_title.o digger_music_title.c

crt0:
	pdp11-aout-as ${AS_FLAGS} crt0.s -o crt0.o

libs: memory.s divmulmod.s sprites.c sprites_title.c sound.c tools.c
	pdp11-aout-as ${AS_FLAGS} memory.s -o memory.o
# 	pdp11-aout-as ${AS_FLAGS} divmulmod.s -o divmulmod.o
	pdp11-aout-gcc ${GCC_FLAGS} -c -o sprites.o sprites.c
	pdp11-aout-gcc ${GCC_FLAGS} -c -o sprites_title.o sprites_title.c
	pdp11-aout-gcc ${GCC_FLAGS} -c -o sound.o sound.c
	pdp11-aout-gcc ${GCC_FLAGS} -c -o tools.o tools.c
	pdp11-aout-ar rcs libs.a memory.o sprites.o sprites_title.o sound.o tools.o  # divmulmod.o

digger-out-file: crt0 digger-main-file sprites-file short-font-file levels-file libs
	pdp11-aout-ld -T a.out.ld -Map digger.map -o ${OUT_FILE_1} crt0.o digger_sprites.o digger_short_font.o digger_levels.o digger.o libs.a

title-out-file: crt0 title-main-file sprites-file-title full-font-file music-file-title libs
	pdp11-aout-ld -T a.out.ld -Map title.map -o ${OUT_FILE_2} crt0.o digger_sprites_title.o digger_full_font.o digger_music.o title.o libs.a

aout2bin:
	gcc aout2bin.c -o aout2bin

bin-files: aout2bin digger-out-file title-out-file
	./aout2bin ${OUT_FILE_1} ${BIN_FILE_1}
	./aout2bin ${OUT_FILE_2} ${BIN_FILE_2}

g-mpi: bin-files
	curl -i -o /dev/null -X POST -H "Content-Type: multipart/form-data" -F "storeas=${GMPI_UPLOAD_DIR}/${BIN_FILE_1}" -F "size=$(shell stat -c%s ${BIN_FILE_1})" -F "file=@${BIN_FILE_1}" "${GMPI_API_URL}/upload"
	curl -i -s -o /dev/null "${GMPI_API_URL}/run?dev=file&emu10=no&fname=${GMPI_UPLOAD_DIR}/${BIN_FILE_1}"

docs:
	doxygen Doxyfile

clean:
	rm -f *.o *.a *.out *.map aout2bin
	rm -rf docs/*
