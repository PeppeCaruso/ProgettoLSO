CC = gcc
CFLAGS = -Wall -Wextra -O2
TARGET = server
OBJS = main.o connessione.o gestione_partita.o gestione_client.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) -lpthread

main.o: main.c connessione.h gestione_partita.h gestione_client.h strutture.h
	$(CC) $(CFLAGS) -c main.c

connessione.o: connessione.c connessione.h strutture.h
	$(CC) $(CFLAGS) -c connessione.c

gestione_partita.o: gestione_partita.c gestione_partita.h strutture.h
	$(CC) $(CFLAGS) -c gestione_partita.c

gestione_client.o: gestione_client.c gestione_client.h strutture.h
	$(CC) $(CFLAGS) -c gestione_client.c

clean:
	rm -f $(OBJS) $(TARGET)
