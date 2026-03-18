# net scan

**IoT Network Scanner — Level 5**  
IoTS25 · Jensen Yrkeshögskola · 2026

| | |
|---|---|
| **Kurs** | Embedded Linux / IoT Architecture & Data Communication |
| **Uppgift** | Bygg ett CLI-verktyg som scannar ett subnät, hämtar MAC-adresser och avgör om enheten är en Raspberry Pi |
| **Verktyg** | Python · uv · typer · arp · macvendors API |
| **Platform** | Debian (Docker container / Raspberry Pi) |

---

## 1. Vad är uppgiften?

Uppgiften är att bygga ett CLI-verktyg som tar ett subnät (t.ex. `192.168.1.0/27`) som argument, pingar alla IP-adresser i subnätet, hämtar MAC-adressen för varje svarande IP via ARP, slår upp tillverkaren via ett publikt API och avgör om enheten är en Raspberry Pi.

**Förväntat output:**

```
No, b0:39:56:57:46:88 192.168.1.1 is not a known Raspberry Pi address.
No, 68:a4:0e:03:f1:5a 192.168.1.2 is not a known Raspberry Pi address.
Yes, 88:a2:9e:3f:71:d2 192.168.1.17 is a Raspberry Pi address.
Yes, 2c:cf:67:d1:a5:18 192.168.1.20 is a Raspberry Pi address.
```

> **TIP:** I verkligheten körs detta på ett nätverk med riktiga Pi:ar. I Docker-miljön finns bara din container och gateway — det är normalt.

---

## 2. Förutsättningar

### 2.1 Windows — Docker Desktop

Docker Desktop måste vara installerat och igång på din Windows-dator.

### 2.2 Starta IoT-nätverket och containern

Kör dessa kommandon i PowerShell från projektmappen:

```powershell
cd C:\Users\<ditt-namn>\ws\Level-5\edu-embedded-linux

bash start-iotnet.sh       # Skapar Docker-nätverket iotnet

docker compose down
docker compose up -d --build   # Bygger och startar containern
docker compose ps              # Verifiera att rpi-dev körs
```

> **OBS:** `start-iotnet.sh` MÅSTE köras INNAN `docker compose up` — annars saknas nätverket och containern startar inte.

### 2.3 SSH in i containern

```bash
ssh -p 2225 dev@localhost
# Lösenord: dev
```

*Du är nu inne i en Debian Linux-container som simulerar en Raspberry Pi.*

---

## 3. Installera beroenden i containern

Kör dessa kommandon en gång efter att du SSH:at in:

```bash
# PATH-installation
echo 'export PATH="$HOME/.local/bin:$PATH"' >> ~/.bashrc
echo 'export PATH="$PATH:/usr/sbin"' >> ~/.bashrc
source ~/.bashrc

# Systemverktyg
sudo apt update
sudo apt install -y curl iputils-ping net-tools

# uv (Python package manager)
curl -LsSf https://astral.sh/uv/install.sh | sh
source ~/.bashrc

# Verifiera
uv --version
which arp    # Ska visa: /usr/sbin/arp
which ping   # Ska visa sökväg till ping
```

> **TIP:** Om `uv` inte hittas: kör `source ~/.bashrc` igen. uv installeras till `~/.local/bin/` som måste vara i PATH.

---

## 4. Hitta rätt IP-adress att scanna

Kör detta inne i containern:

```bash
hostname -I
# Output: 192.168.2.10
```

Om din IP är `192.168.2.10` scannar du `192.168.2.0/27` — inte `192.168.1.0/27`. Matcha alltid subnätet mot din faktiska IP.

> **OBS:** Vanligt misstag: README visar `192.168.1.0/27` men ditt Docker-nätverk använder `192.168.2.0/24`. Kolla alltid med `hostname -I`.

---

## 5. Koden — main.py

Fil: `~/ws/iot/src/net/main.py` — ersätt hela innehållet med koden nedan.

```python
import typer
import subprocess
import ipaddress
import urllib.request
import time

app = typer.Typer(no_args_is_help=True)

def _get_mac(ip: str):
    result = subprocess.run(
        ["arp", "-n", ip],
        capture_output=True, text=True, timeout=5
    )
    for line in result.stdout.splitlines():
        parts = line.split()
        if len(parts) >= 3 and parts[0] == ip:
            return parts[2]
    return None

def _get_vendor(mac_addr: str):
    url = f"https://api.macvendors.com/{mac_addr}"
    try:
        req = urllib.request.Request(
            url, headers={"User-Agent": "Mozilla/5.0"})
        with urllib.request.urlopen(req) as response:
            return response.read().decode("utf-8")
    except urllib.error.HTTPError as e:
        if e.code == 404: return None
        elif e.code == 429: raise Exception("Too many requests.")
        else: raise Exception(f"HTTP {e.code} {e.reason}")
    except Exception as e:
        raise Exception(f"{e}")

def _is_raspberry_pi(vendor: str) -> bool:
    if vendor is None: return False
    return "raspberry" in vendor.lower()

@app.command()
def scan(netmask: str = typer.Argument(
        ..., help="Netmask to scan (e.g., 192.168.1.0/27)")):
    """Scan network, find MACs, detect Raspberry Pis."""
    print(f"Scanning network: {netmask}...")
    try:
        network = ipaddress.ip_network(netmask, strict=False)
    except ValueError as e:
        print(f"Error: {e}"); raise typer.Exit(code=1)

    try:
        for ip in network:
            ip_str = str(ip)
            try:
                result = subprocess.run(
                    ["ping", "-n", "-c", "1", "-W", "1", ip_str],
                    capture_output=True, text=True, timeout=3)
                if result.returncode != 0: continue

                mac = _get_mac(ip_str)
                if mac is None: continue

                try:
                    vendor = _get_vendor(mac)
                    time.sleep(1)
                except Exception as e:
                    typer.secho(f"Vendor lookup failed: {e}",
                        fg=typer.colors.YELLOW, err=True)
                    vendor = None

                if _is_raspberry_pi(vendor):
                    print(f"Yes, {mac} {ip_str} is a Raspberry Pi address.")
                else:
                    print(f"No, {mac} {ip_str} is not a known Raspberry Pi address.")

            except subprocess.TimeoutExpired: continue
            except KeyboardInterrupt: raise

    except KeyboardInterrupt:
        print("\nScan interrupted."); raise typer.Exit(code=0)

@app.command()
def other():
    """Dummy command to avoid command name being collapsed."""
    pass

if __name__ == "__main__":
    app()
```

---

## 6. Kodförklaring — block för block

### Imports

| Modul | Syfte |
|---|---|
| `typer` | CLI-ramverk som hanterar argument och kommandon automatiskt |
| `subprocess` | Kör externa program (`ping`, `arp`) inifrån Python |
| `ipaddress` | Parsear och itererar subnät som `192.168.2.0/27` |
| `urllib.request` | Gör HTTP-anrop till macvendors API |
| `time` | `sleep(1)` för att respektera API:ets rate limit (max 1 req/sek) |

### `app = typer.Typer(...)` — Skapar CLI-appen

- `no_args_is_help=True` visar hjälptext om du kör `net` utan argument.
- Varje funktion med `@app.command()` blir ett subkommando.
- Kommandona `net scan` och `net other` skapas automatiskt.

### `_get_mac(ip)` — Hämtar MAC via ARP

- Kör `arp -n <ip>` → output: `192.168.2.1 ether 42:87:d8:5a:9e:b6 C eth0`
- Delar raden på whitespace, kontrollerar `parts[0] == ip`, returnerar `parts[2]` (MAC).
- Om IP:n är din egna returnerar arp `--` → funktionen returnerar `None`.
- Kräver: `net-tools` installerat + `/usr/sbin` i PATH.

### `_get_vendor(mac)` — Slår upp tillverkare via API

- Skickar GET till `https://api.macvendors.com/<mac>`
- `200 OK` → returnerar vendorsträngen, t.ex. `Raspberry Pi Trading Ltd`
- `404` → okänd MAC, returnerar `None` (inte ett fel)
- `429` → rate limit nådd, kastar exception (skyddas av `time.sleep(1)`)
- User-Agent header krävs — utan den blockeras requests av API:et.

### `_is_raspberry_pi(vendor)` — Identifierar Pi:ar

- Kollar om strängen `raspberry` finns i vendor (case-insensitive).
- Raspberry Pi Foundation's OUI returnerar `Raspberry Pi Trading Ltd`.
- `None`-check skyddar mot misslyckad vendor-lookup.

### `@app.command() scan()` — Huvudflödet

1. Parsear subnätet med `ipaddress.ip_network()`
2. Itererar varje IP i subnätet
3. Pingar: `ping -n -c 1 -W 1 <ip>` (1 paket, 1 sek timeout)
4. `returncode == 0` → IP svarade, hämta MAC
5. MAC hittad → vendor lookup + `sleep(1)`
6. Skriv ut `Yes/No` baserat på `_is_raspberry_pi()`

`KeyboardInterrupt` (Ctrl+C) hanteras elegant med exit code 0.

---

## 7. Uppdatera filen och köra

**Steg 1 — Skriv ny kod till filen:**

```bash
cat > ~/ws/iot/src/net/main.py << 'EOF'
<klistra in hela koden från avsnitt 5>
EOF
```

> **TIP:** Hela heredoc (från `<< 'EOF'` till `EOF`) måste klistras in i ett svep i terminalen.

**Steg 2 — Verifiera:**

```bash
head -3 ~/ws/iot/src/net/main.py
# Ska visa: import typer / import subprocess / import ipaddress
```

**Steg 3 — Kör scanningen:**

```bash
cd ~/ws/iot

uv run net scan 192.168.2.0/27              # med felmeddelanden
uv run net scan 192.168.2.0/27 2>/dev/null  # utan felmeddelanden
```

> **OBS:** Kör alltid från `~/ws/iot` där `pyproject.toml` finns. uv hittar inte projektet från `src/net/`.

---

## 8. Felsökning

| Felmeddelande | Lösning |
|---|---|
| `uv: command not found` | `echo 'export PATH="$HOME/.local/bin:$PATH"' >> ~/.bashrc && source ~/.bashrc` |
| `arp: command not found` | `sudo apt install net-tools -y` och `echo 'export PATH="$PATH:/usr/sbin"' >> ~/.bashrc && source ~/.bashrc` |
| `curl: command not found` | `sudo apt install curl -y` |
| Inga IP:er / tom output | Kör `hostname -I` — kontrollera att du scannar rätt subnet |
| HTTP 429 (rate limit) | `time.sleep(1)` ska förhindra detta. Vänta 60 sekunder och försök igen. |
| `--` istället för MAC | Det är din egna IP. `arp` returnerar alltid `--` för localhost. Normalt. |
| `Neither setup.py nor pyproject` | Du är i fel mapp. Kör från `~/ws/iot/` — inte `~/ws/iot/src/net/` |

---

## 9. Varför relevant för IoT?

Device discovery är ett grundläggande IoT-problem. När du deployar 20 Raspberry Pi:ar som edge-noder på en fabrik delar DHCP ut IP:er slumpmässigt. Utan ett verktyg som `net scan` måste du logga in på routern och leta manuellt.

Ping sweep, ARP lookup och OUI/vendor matching är exakt samma tekniker som professionella verktyg som `nmap` och Fing använder. Du bygger en lightweight version anpassad för ditt behov.

| Scenario | Hur net scan hjälper |
|---|---|
| Ny Pi Zero deployad på nätverk | Scanna → hitta IP direkt → SSH in |
| 10 edge-noder på fabrik | Scanna `/24` → se vilka som är Pi:ar |
| En nod offline | Scanna → saknad IP = offline nod |
| Inventering av enheter | Scanna → lista alla Pi:ar automatiskt |

---

## 10. Git — pusha koden till GitHub

Skapa ett tomt repository på GitHub (inga filer, ingen README). Hämta URL:en och skapa ett Personal Access Token (PAT) som lösenord.

```bash
git config --global init.defaultBranch main
git config --global user.name "Ditt Namn"
git config --global user.email "din@email.com"

cd ~/ws/net
git init
git add .
git commit -m "initial commit"
git remote add origin https://github.com/<username>/<repo>.git
git push -u origin main
# Ange PAT som lösenord när du uppmanas
```

> **TIP:** Installera sedan på vilken Pi som helst:
> `uv tool install git+https://github.com/<username>/<repo>.git`

---

## 11. Snabbreferens — alla kommandon

| Kommando | Syfte |
|---|---|
| `bash start-iotnet.sh` | Skapa Docker-nätverket iotnet |
| `docker compose up -d --build` | Starta containern |
| `ssh -p 2225 dev@localhost` | SSH in (lösenord: `dev`) |
| `hostname -I` | Se din IP i containern |
| `sudo apt install net-tools curl iputils-ping -y` | Installera systemverktyg |
| `curl -LsSf https://astral.sh/uv/install.sh \| sh` | Installera uv |
| `source ~/.bashrc` | Ladda om PATH |
| `cd ~/ws/iot && uv run net scan 192.168.2.0/27` | Kör nätverksscan |
| `uv run net scan 192.168.2.0/27 2>/dev/null` | Kör utan felmeddelanden |
| `ssh-keygen -R "[localhost]:2225"` | Rensa SSH fingerprint |

---

*IoTS25 · Jensen Yrkeshögskola · Embedded Linux Level 5 · 2026*
