main.bin: sha2.o bitcoin_utils.o debug_utils.o data_utils.o custom_errors.o
	gcc -Wall src/main.c obj/sha2.o include/sha2.h obj/bitcoin_utils.o include/bitcoin_utils.h obj/debug_utils.o include/debug_utils.h obj/data_utils.o include/data_utils.h obj/custom_errors.o include/custom_errors.h -o bin/main.bin -lpthread -lrt -Iinclude

sha2.o:
	gcc -Wall -c src/sha2.c -o obj/sha2.o -Iinclude
	
bitcoin_utils.o:
	gcc -Wall -c src/bitcoin_utils.c -o obj/bitcoin_utils.o -Iinclude

debug_utils.o:
	gcc -Wall -c src/debug_utils.c -o obj/debug_utils.o -Iinclude

data_utils.o:
	gcc -Wall -c src/data_utils.c -o obj/data_utils.o -Iinclude

custom_errors.o:
	gcc -Wall -c src/custom_errors.c -o obj/custom_errors.o -Iinclude

freshbuild:
	rm -f obj/* bin/*
	make

build:
	rm -f bin/*
	make

clean:
	rm -f obj/* bin/*