# Proximity Effect: stato del lavoro

## 1. Progetto e percorsi

- **Nome:** Proximity Effect (plugin audio VST3 / AU / Standalone, JUCE 8.0.4, C++17, CMake)
- **Copia locale persistente (cartella condivisa del progetto Claude):** `/mnt/project-files/proximity-effect/ProximityEffect/`
- **Copia di lavoro della sessione cloud (temporanea, si perde a fine sessione):** `/home/claude/ProximityEffect/`
- **Copia su GitHub (temporanea):** repository privato `sambitomanagment-lab/PRODUCER-OS`, branch `proximity-effect`, cartella `proximity-effect/`. Vedi §9.

## 2. Ultimo aggiornamento

2026-10-22 08:30 UTC

## 3. Stato generale

Codice, GUI, DSP, test automatici, script di build/packaging e workflow CI sono completi.
Compilato e testato **solo su Linux x64** (ambiente cloud). Le build Windows e macOS sono configurate ma la loro riuscita dipende dal primo run di GitHub Actions (vedi §7 e §9).

## 4. Funzionalità presenti e verificate (su Linux)

| Funzione | Verifica |
|---|---|
| Personaggio sulla sedia trascinabile (0 a 100 cm), doppio clic = default | Screenshot GUI |
| Volto che si gonfia e sedia che si schiaccia con la vicinanza | Screenshot GUI |
| Angry: espressione arrabbiata, pugni alzati, tremore, vapore; distorsione in serie | Screenshot + test (correlazione < 0,995) |
| Pattern polari Omni / Cardioid / Super / Hyper / Figure 8 | Test: a 0 cm Fig 8 +10,06 dB, Cardioid +5,12 dB, Omni +0,01 dB di bassi (60 Hz vs 1 kHz) |
| Risposta piatta a 100 cm | Test: ±0,1 dB da 40 Hz a 15 kHz |
| Ego Killer: pad -20 dB in ingresso, prima della saturazione | Test: -20,00 dB esatti; con Angry riduce il drive |
| Input, Output, Low-End Push, Gain Boost, Mix, HPF 80 Hz, Air, Polarity | Test |
| Bypass del plugin (parametro host) allineato alla latenza | Test: errore < 1e-6 |
| Latenza oversampling dichiarata = reale (61 campioni a 48 kHz) | Test a 44,1 / 48 / 96 kHz |
| Sample rate 22,05 a 192 kHz, buffer 1 a 2048, buffer più grandi di quelli annunciati | Test: uscita finita e limitata |
| Mono e stereo (mono→stereo e 5.1 rifiutati) | Test |
| Nessun click con automazione di distanza, pattern, pad, polarità, bypass, HPF/Air | Test (salto massimo < 1,25× regime) |
| Salvataggio/ripristino stato, 13 parametri automatizzabili, stato corrotto ignorato | Test |
| GUI ridimensionabile (50% a 200%), creazione/distruzione ripetuta | Test |
| VST3 Linux valido | pluginval strictness 10: SUCCESS |

## 5. Funzionalità incomplete o non verificate

- **Build Windows x64 (VST3 + Standalone):** configurata, non ancora compilata. Da verificare con GitHub Actions o `scripts\build.ps1`.
- **Build macOS (VST3 + AU + Standalone, universale arm64 + x86_64):** configurata, non ancora compilata. Da verificare con GitHub Actions o `scripts/build.sh`.
- **auval (validazione AU):** previsto nel workflow macOS, non ancora eseguito.
- **Rilevamento in FL Studio / altre DAW:** non verificabile da qui; va provato a mano.
- **Firma:** macOS solo firma ad-hoc (serve `xattr -dr com.apple.quarantine`, vedi docs/INSTALL.md); Windows non firmato (avviso SmartScreen per lo .exe standalone). Nessuna notarizzazione Apple.
- **AAX (Pro Tools):** non previsto.
- **Personaggio:** cartone generico disegnato in codice, non la persona del mockup.

## 6. Modifiche effettuate

Sessione 1 (2026-10-22 mattina):
- Creato il progetto JUCE/CMake, DSP (shelf di prossimità che sale da 90 a 250 Hz, bump 200 Hz, gain boost, saturazione morbida, Angry, oversampling 4x), GUI comica completa.

Sessione 2 (2026-10-22, questa):
- Calibrazione DSP: Figure 8 +10 dB, Cardioid +5 dB, Omni 0 dB a 0 cm con Low-End Push a 0 (fattori pattern 0 / 0,48 / 0,60 / 0,72 / 1,0; profondità shelf 11,5 dB).
- Aggiunto parametro Bypass esposto all'host, con crossfade e ritardo uguale alla latenza; `processBlockBypassed` coerente.
- Runtime MSVC statico (niente VC++ Redistributable), versione nel plugin da `CMakeLists.txt`.
- Test automatici `Tests/ProximityEffectTests.cpp` (162 controlli) con CTest.
- Script `scripts/build.sh`, `scripts/build.ps1`, `scripts/package.sh`, `scripts/package.ps1`.
- `docs/INSTALL.md` con istruzioni per Windows, macOS, FL Studio, Ableton, Logic, Reaper, Cubase, Studio One.
- Workflow `.github/workflows/build.yml`: build Windows + macOS, test, pluginval, auval, zip, release da tag.
- Ego Killer: verificato che è un pad di ingresso (come da specifica originale), non una riduzione del volume finale; il volume d'uscita cala di 20 dB su segnali puliti, come su un microfono vero.

## 7. Build e test eseguiti (risultati reali)

| Data | Ambiente | Comando | Risultato |
|---|---|---|---|
| 2026-10-22 | Linux x64, g++ 13 | `cmake -B build -DJUCE_PATH=… -DPE_BUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Release && cmake --build build` | OK (VST3, Standalone, test) |
| 2026-10-22 | Linux x64 | `build/ProximityEffectTests_artefacts/Release/ProximityEffectTests --report` | 162 controlli, 0 falliti |
| 2026-10-22 | Linux x64 | `pluginval --strictness-level 10 --skip-gui-tests --validate "…/Proximity Effect.vst3"` | SUCCESS |
| 2026-10-22 | Linux x64 | `scripts/package.sh` | `dist/ProximityEffect-Linux-x64.zip` creato |

Risultati dei run GitHub Actions: vedi §9.

## 8. Problemi aperti e tentativi

- **Nessun repository dedicato su GitHub.** Questa sessione non può creare repository; l'utente non può crearlo ora. Soluzione temporanea concordata: branch `proximity-effect` di PRODUCER-OS (privato), cartella `proximity-effect/`, main non toccato.
- **Minuti GitHub Actions:** su repo privati i runner macOS consumano minuti 10× rispetto a Linux. Una build completa macOS dura indicativamente 10-15 minuti.

## 9. GitHub: connessione e autorizzazioni

- Accesso push a `sambitomanagment-lab/PRODUCER-OS`: verificato (repo privato).
- Creazione nuovi repository: non disponibile da questa sessione.
- Workflow attivo nel branch: `.github/workflows/proximity-effect.yml` (copia di `proximity-effect/.github/workflows/build.yml` con percorsi adattati). Si attiva solo sul branch `proximity-effect` e sui tag `proximity-effect-v*`.
- Stato dei run: **da aggiornare dopo il primo push**.

## 10. Repository locale e remoto

- Locale: repo git in `ProximityEffect/` (branch `main`), uno snapshot anche come `git bundle` in `/mnt/project-files/proximity-effect/ProximityEffect.bundle`.
- Remoto: PRODUCER-OS, branch `proximity-effect`. Il main di PRODUCER-OS non è stato modificato.

## 11. File generati e artefatti

- Sorgenti: `/mnt/project-files/proximity-effect/ProximityEffect/`
- Pacchetto Linux (test): `/mnt/project-files/proximity-effect/dist/ProximityEffect-Linux-x64.zip`
- Screenshot GUI: `/mnt/project-files/proximity-effect/ui-close.png`, `ui-angry.png`, `ui-far.png`
- Log: `/mnt/project-files/proximity-effect/logs/` (test, pluginval, build)
- Pacchetti Windows/macOS: Actions Artifacts del run su GitHub (`ProximityEffect-Windows-x64`, `ProximityEffect-macOS-Universal`), quando il run passa.

## 12. Ultima attività completata

Test automatici e pluginval su Linux superati; workflow CI scritto.

## 13. Prossima attività

1. Verificare il primo run GitHub Actions su Windows e macOS e correggere eventuali errori.
2. Scaricare gli zip dagli artifact e provarli in FL Studio.
3. Quando esiste il repo dedicato: spostare la cartella `proximity-effect/` alla radice del nuovo repo (il workflow giusto è già in `proximity-effect/.github/workflows/build.yml`), poi creare il tag `v1.0.0` per la prima release.

## 14. Come riprendere il lavoro

- Da zero su un PC: `git clone` del repo (o estrai la cartella), poi `scripts\build.ps1` (Windows) o `scripts/build.sh` (macOS/Linux). Requisiti nel README.
- Per spostare il progetto in un repo nuovo, dalla cartella `proximity-effect/`:
  ```sh
  git init && git add -A && git commit -m "Proximity Effect"
  git remote add origin https://github.com/<utente>/proximity-effect.git
  git push -u origin main
  ```
  Il workflow `.github/workflows/build.yml` partirà da solo. Poi elimina il branch `proximity-effect` da PRODUCER-OS.
- In una nuova chat con Claude: indica questo file (`status.md`) e il percorso del progetto.
