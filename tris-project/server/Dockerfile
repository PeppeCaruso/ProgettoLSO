# Usa Ubuntu come immagine base
FROM ubuntu:latest

# Installa GCC e Make
RUN apt-get update && apt-get install -y gcc make && rm -rf /var/lib/apt/lists/*

# Imposta la cartella di lavoro
WORKDIR /app

# Copia i file del progetto nel container
COPY . .

# Compila il codice
RUN make

# Espone la porta 8080 per la comunicazione TCP
EXPOSE 8080

# Avvia il server
CMD ["./server"]
