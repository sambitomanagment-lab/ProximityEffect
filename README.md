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

## Compilare su Windows

Servono [Visual Studio 2022](https://visualstudio.microsoft.com/) (carico di lavoro "Sviluppo di applicazioni desktop con C++"), [CMake](https://cmake.org/download/) 3.22 o superiore e Git.

```bat
cmake -B build
cmake --build build --config Release
```

JUCE viene scaricato automaticamente al primo configure. Se hai già JUCE in locale: `cmake -B build -DJUCE_PATH=C:\percorso\JUCE`.

Il plugin si trova in `build\ProximityEffect_artefacts\Release\VST3\Proximity Effect.vst3`: copialo in `C:\Program Files\Common Files\VST3\`.

## Compilare su macOS

Servono Xcode e CMake.

```sh
cmake -B build -G Xcode
cmake --build build --config Release
```

Trovi `Proximity Effect.vst3` e `Proximity Effect.component` (AU) in `build/ProximityEffect_artefacts/Release/`. Copiali in `~/Library/Audio/Plug-Ins/VST3/` e `~/Library/Audio/Plug-Ins/Components/`.

## Build automatiche

Su GitHub, il workflow in `.github/workflows/build.yml` compila Windows e macOS a ogni push e allega i plugin come artifact.

## Strumento di verifica

`cmake -B build -DPE_BUILD_TOOLS=ON` aggiunge `DevCheck`, che misura la risposta in frequenza per distanza e pattern e salva screenshot dell'interfaccia.
