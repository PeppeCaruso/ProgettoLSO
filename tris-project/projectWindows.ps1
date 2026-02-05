# numero di client da avviare
Write-Host "Inserisci il numero di client da avviare"
$n_client = Read-Host

# Verifica del numero inserito
if ($n_client -lt 1)
{
    Write-Host "Riprova, inserisci un numero di client da avviare corretto"
    exit
}

# Avvia docker compose
Start-Process powershell -ArgumentList  "-NoExit", "-Command", "docker-compose up --build --scale client=$n_client"

# Aspetta che tutti i container siano avviati
$allContainersRunning = $false
while (-not $allContainersRunning)

{
    # Ottieni i nomi dei container in esecuzione
    $containers = docker ps --format "{{.Names}}"

    # Controlla se i container desiderati sono in esecuzione
    $allContainersRunning = $true
    for ($i = 1; $i -le $n_client; $i++) 
    {
        $containerName = "tris-project-client-$i"
        if (-not ($containers -contains $containerName)) 
        {
            $allContainersRunning = $false
            break
        }
    }
    # Aspetta 2 secondi prima di un altro controllo
    if (-not $allContainersRunning)
    {
        Start-Sleep -Seconds 2
    }
}

#avvia una finestra powershell per ogni client
for ($i = 1; $i -le $n_client; $i++)
{
    Start-Process powershell -ArgumentList "-NoExit", "-Command", "docker attach tris-project-client-$i" 
}