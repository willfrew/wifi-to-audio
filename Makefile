build:
	gcc ./main.c ./depends/radiotap/radiotap.c ./alsa.c -o main -lpcap -lasound

clean:
	$(RM) main
