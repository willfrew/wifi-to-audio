build:
	gcc ./main.c ./depends/radiotap/radiotap.c ./pulse.c ./drop-root.c -o main -lpcap -lpulse -pthread -lm

run:
	sudo -E ./main

clean:
	$(RM) main
