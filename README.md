# Proximity Effect

Plugin audio (VST3, AU su macOS, più una versione Standalone) che emula l'**effetto prossimità** dei microfoni direzionali, con un'interfaccia in stile fumetto: trascini il tizio sulla sedia verso il microfono e la voce diventa più vicina, più grossa e più forte.

## Controlli

| Controllo | Cosa fa |
|---|---|
| **Personaggio sulla sedia** (macro) | Distanza dal microfono, 0 cm (attaccato al pop filter) a 100 cm (troppo lontano). A 100 cm il segnale è dry e piatto; avvicinandosi crescono boost dei bassi, volume e una leggera saturazione. Il comportamento segue circa 1/r, quindi gli ultimi centimetri contano di più. Doppio clic per tornare al default. |
| **Polar Pattern** | Omni, Cardioid, Supercardioid, Hypercardioid, Figure 8. Scala l'intensità del boost dei bassi (Omni = nessun proximity effect, Figure 8 = massimo). |
| **Input Gain** | Guadagno in ingresso, da -inf a +20 dB. |
| **Low-End Push** | Profondità del boost dei bassi (shelf tra ~90 e ~250 Hz, la frequenza sale avvicinandosi). A 0 il massimo è circa +12 dB in Figure 8; da -12 (niente boost) a +12 (fino a +24 dB). |
| **Gain Boost** | Quanto volume in più arriva avvicinandosi (0 a +12 dB alla distanza minima). Indipendente dal pattern. |
| **Mix** | Blend dry/wet globale. |
| **Output** | Guadagno in uscita, da -inf a +12 dB. |
| **HPF Filter** | Passa-alto 80 Hz, 12 dB/ottava, in ingresso. |
| **Air Mode** | High shelf +5 dB a 10 kHz in uscita. |
| **Angry Mode** | Distorsione asimmetrica con enfasi sui medi (~1.6 kHz): qualcuno che urla nel microfono. Il personaggio si arrabbia e trema. |
| **Ego Killer -20 dB** | Pad di -20 dB in ingresso. |
| **Polarity (Ø)** | Inverte la polarità. |

Le manopole hanno un LED che si accende quando sono spinte oltre il 70% circa.

## Catena del segnale

Pad → Input Gain → Polarità → HPF → *oversampling 4x* → shelf di prossimità + bump a 200 Hz → Gain Boost → saturazione di prossimità → Angry → Mix → *downsampling* → Air → Output.

La latenza dovuta all'oversampling (circa 61 campioni a 48 kHz) viene comunicata alla DAW, che la compensa.

## Installazione

Vedi [docs/INSTALL.md](docs/INSTALL.md): Windows, macOS, FL Studio e le altre DAW.

## Compilare

Requisiti: CMake 3.22+, Git e un compilatore C++17.
- Windows: Visual Studio 2022 con "Sviluppo di applicazioni desktop con C++".
- macOS: Xcode (o i Command Line Tools).
- Linux: g++ e le librerie di sviluppo di ALSA, FreeType, fontconfig e X11.

JUCE 8.0.4 viene scaricato automaticamente al primo configure. Per usare una copia locale imposta `JUCE_PATH`.

Un solo comando configura, compila, esegue i test e crea lo zip in `dist/`:

```bat
:: Windows (PowerShell o cmd)
powershell -ExecutionPolicy Bypass -File scripts\build.ps1
```

```sh
# macOS / Linux
scripts/build.sh
```

Risultato:

| Piattaforma | Pacchetto | Contenuto |
|---|---|---|
| Windows | `dist/ProximityEffect-Windows-x64.zip` | VST3 x64, Standalone .exe |
| macOS | `dist/ProximityEffect-macOS-Universal.zip` | VST3, AU, Standalone .app (arm64 + x86_64, firma ad-hoc) |
| Linux | `dist/ProximityEffect-Linux-x64.zip` | VST3, Standalone |

I file compilati singoli restano in `build/ProximityEffect_artefacts/Release/`.

## Test

`cmake -B build -DPE_BUILD_TESTS=ON` aggiunge `ProximityEffectTests`, eseguito da `ctest`. Controlla:
- calibrazione del proximity effect (Figure 8 circa +10 dB, Cardioid circa +5 dB, Omni 0 dB), risposta piatta a 100 cm;
- pad -20 dB, HPF, Air, Angry, gain, mix;
- latenza dichiarata uguale a quella reale, polarità, bypass allineato in tempo;
- sample rate da 22,05 a 192 kHz, buffer da 1 a 2048 campioni e buffer più grandi di quelli annunciati;
- layout mono e stereo;
- assenza di click durante l'automazione;
- salvataggio e ripristino dello stato, parametri automatizzabili;
- creazione e ridimensionamento della GUI.

Opzioni: `--report` stampa la tabella completa di risposta in frequenza, `--snapshots DIR` salva screenshot della GUI.

## GitHub Actions

`.github/workflows/build.yml` compila su Windows e macOS a ogni push, esegue i test, pluginval (strictness 10) e auval, e carica gli zip come artifact. Un tag `v*` (per esempio `v1.0.0`) pubblica una Release con gli zip, solo se tutti i controlli sono passati.

## Struttura

```
Source/          codice del plugin (DSP, GUI)
Tests/           test automatici
scripts/         build e packaging
docs/INSTALL.md  istruzioni di installazione
status.md        stato del lavoro
```
