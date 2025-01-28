BIN_FILE=digger_tpc.bin
OUT_FILE=digger_tpc.out
OPT_FLAG=-O2
XGCC=/home/prcoder/xgcc
GCC_FLAGS=-std=gnu23 -fomit-frame-pointer -msoft-float -fcprop-registers -fPIC -nostartfiles -nodefaultlibs -nostdlib -m10 $(OPT_FLAG) -I$(XGCC)/include
GCC_ASM_FLAGS=-S -fverbose-asm
AS_FLAGS=-mno-fpu -mlimited-eis -pic
GMPI_API_URL=http://10.0.0.55/api
GMPI_UPLOAD_DIR=/BK_Uploads

all: asm-file bin-file

asm-file: digger.c sprites.c tools.c
	pdp11-aout-gcc ${GCC_FLAGS} ${GCC_ASM_FLAGS} digger.c
	pdp11-aout-gcc ${GCC_FLAGS} ${GCC_ASM_FLAGS} sprites.c
	pdp11-aout-gcc ${GCC_FLAGS} ${GCC_ASM_FLAGS} tools.c

main-file: digger.c
	pdp11-aout-gcc ${GCC_FLAGS} -c -o digger.o digger.c

levels-file: digger_levels.c
	pdp11-aout-gcc ${GCC_FLAGS} -c -o digger_levels.o digger_levels.c

font-file: digger_font.c
	pdp11-aout-gcc ${GCC_FLAGS} -c -o digger_font.o digger_font.c

sprites-file: digger_sprites.c
	pdp11-aout-gcc ${GCC_FLAGS} -c -o digger_sprites.o digger_sprites.c

# music-file: digger_music.c
# 	pdp11-aout-gcc ${GCC_FLAGS} -c -o digger_music.o digger_music.c

crt0:
	pdp11-aout-as ${AS_FLAGS} crt0.s -o crt0.o

libs: memory.s divmulmod.s sprites.c sound.c tools.c
	pdp11-aout-as ${AS_FLAGS} memory.s -o memory.o
# 	pdp11-aout-as ${AS_FLAGS} divmulmod.s -o divmulmod.o
	pdp11-aout-gcc ${GCC_FLAGS} -c -o sprites.o sprites.c
	pdp11-aout-gcc ${GCC_FLAGS} -c -o sound.o sound.c
	pdp11-aout-gcc ${GCC_FLAGS} -c -o tools.o tools.c
	pdp11-aout-ar rcs libs.a memory.o sprites.o sound.o tools.o  # divmulmod.o

out-file: crt0 main-file sprites-file font-file levels-file libs
	pdp11-aout-ld -T a.out.ld -Map digger.map -o ${OUT_FILE} crt0.o digger.o libs.a digger_levels.o digger_font.o digger_sprites.o # digger_music.o

aout2bin:
	gcc aout2bin.c -o aout2bin

bin-file: aout2bin out-file
	./aout2bin ${OUT_FILE} ${BIN_FILE}

g-mpi: bin-file
	curl -i -o /dev/null -X POST -H "Content-Type: multipart/form-data" -F "storeas=${GMPI_UPLOAD_DIR}/${BIN_FILE}" -F "size=$(shell stat -c%s ${BIN_FILE})" -F "file=@${BIN_FILE}" "${GMPI_API_URL}/upload"
	curl -i -s -o /dev/null "${GMPI_API_URL}/run?dev=file&emu10=no&fname=${GMPI_UPLOAD_DIR}/${BIN_FILE}"

docs:
	doxygen Doxyfile

clean:
	rm -f *.o *.a *.out *.map aout2bin
	rm -rf docs/*
