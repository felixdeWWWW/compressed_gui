# Datenkompression - HEIC vs JPEG 
## Autoren
1. Roman Kobets
2. Felix Wagner
## Einleitung
Dieses Projekt beschäftigt sich mit der Visualisierung der Qualitätsunterschiede zwischen HEIC und JPEG. 
Benötigt wird für jeden Kompressionsalgorithmus der PSNR-Wert pro Qualitätswert und Dateigröße. Dementsprechend bietet das hier entstandene Programm
Klassen zur JPEG-Komprimierung und HEIC-Komprimierung, eine UI zur Bedienung diesee Klassen, eine csv-Ausgabe der benötigten Daten. Ein Jupiter-Notebook
ermöglicht es diese Daten einzulesen und zu plotten.
## Codedokumentation
### 1. gui.h
Bietet die Grundstruktur für die UI.
### 2. heic.h
Diese Header-Datei bietet eine C++-Implementierung zur Komprimierung und Dekomprimierung von Bildern im HEIC-Format.
Die HEIC-Kodierung basiert auf libheif, einer modernen C-Bibliothek zur Handhabung von HEIC/HEIF-Dateien, die wiederum auf HEVC (H.265) aufbaut. Für die Bildvorverarbeitung und Ausgabe werden stb_image und stb_image_write verwendet.
#### HeicEncoder: 
Die Klasse HeicEncoder übernimmt die Umwandlung eines beliebigen Eingabebildes (z. B. PNG oder JPEG) in eine HEIC-Datei. Dazu wird das Bild zunächst mit stbi_load in ein RGB-Bild mit drei Kanälen geladen. Anschließend wird eine neue HEIC-Image-Struktur erzeugt, in die die RGB-Daten direkt kopiert werden.
Die Kompression erfolgt mit dem in libheif integrierten HEVC-Encoder, dem ein Qualitätswert zwischen 0 (sehr stark komprimiert, geringe Qualität) und 100 (nahezu verlustfrei) übergeben wird. Die resultierende HEIC-Datei wird dann entweder unter dem angegebenen Pfad oder automatisch unter einem generierten Standardnamen gespeichert. Der Encoder funktioniert unabhängig von Transparenzinformationen, da HEIC in diesem Fall nur RGB speichert (kein Alpha).
#### HeicDecoder:
Die Klasse HeicDecoder ermöglicht die Rückkonvertierung von HEIC-Dateien in PNG-Dateien. Hierzu wird eine HEIC-Datei eingelesen, dekodiert und wieder in ein RGB-Format gebracht. Diese RGB-Daten werden anschließend mit Hilfe von stb_image_write als PNG gespeichert. Dies ist vor allem für die anschließende PSNR-Berechnung und visuelle Überprüfung der Bildqualität wichtig.
#### Qualitätssweep und Benchmark
Die Methode evaluateHeicQualitySweep nimmt ein Originalbild (z. B. PNG) entgegen und kodiert es mehrfach in HEIC-Dateien bei verschiedenen Qualitätsstufen. Jede dieser Dateien wird anschließend wieder dekodiert, und das resultierende Bild wird mit dem Originalbild verglichen. Zur Bewertung der Bildqualität wird der PSNR-Wert verwendet, der angibt, wie stark das dekomprimierte Bild vom Original abweicht – höhere Werte entsprechen besserer Qualität. Zusätzlich wird die Dateigröße jeder HEIC-Datei mit std::filesystem::file_size ermittelt.
Alle Ergebnisse werden in eine CSV-Datei geschrieben, typischerweise unter dem Namen heic_quality.csv. Dort sind pro Zeile jeweils die Qualitätsstufe, der PSNR-Wert in Dezibel und die resultierende Dateigröße in Byte eingetragen.
Zur Steuerung des Ablaufs erlaubt die Funktion optionale Parameter:
- Der Pfad zur CSV-Datei
- Eine Liste gewünschter Qualitätsstufen
- Die Option, temporäre HEIC- und PNG-Dateien nach dem Testlauf zu behalten oder zu löschen (praktisch zur Fehleranalyse oder Weiterverarbeitung)
### 3. jpg.h
Die Datei bietet zwei zentralen Komponenten:
Die Klasse JpgEncoder ist für die JPEG-Kompression zuständig. 
Beim Laden eines Bildes über stb_image wird es mit vier Kanälen (RGBA) eingelesen. Um die Transparenz sinnvoll in ein RGB-JPEG zu überführen, 
wird eine Alpha-Maske angewendet: Pixel mit einem Alpha-Wert kleiner als 150 werden als vollständig transparent interpretiert und auf Weiß 
gesetzt (255, 255, 255), während alle anderen Pixel ihre Farbe behalten. Das vorbereitete RGB-Bild wird dann mit Hilfe von libjpeg-turbo in das 
JPEG-Format überführt, wobei der Kompressionsfaktor (Qualität) frei wählbar ist. Das Ergebnis wird in eine Zieldatei geschrieben.<br>
Die Klasse JpgDecoder übernimmt die Rückwandlung der JPEG-Datei in ein RGB-Bild. Dazu wird zunächst der Bild-Header analysiert, um Breite, 
Höhe und Farbraum zu bestimmen, anschließend werden die komprimierten Bilddaten dekodiert. Die Bilddaten stehen danach im RGB-Format zur Verfügung.<br>
Die Funktion JpegPSNRtoCSV führt die Qualitätsanalyse durch. Zunächst wird das Originalbild geladen und als Referenz verwendet. 
Für jede angegebene Qualitätsstufe wird das Bild neu komprimiert und wieder dekodiert. Anschließend wird mit der Hilfsfunktion computePSNR der 
PSNR-Wert zwischen Original und dekomprimiertem Bild berechnet – ein Maß für die Verzerrung durch Kompression. Zusätzlich wird die Größe der 
komprimierten Datei in Byte ermittelt. Die Ergebnisse (Qualität, PSNR, Dateigröße) werden gesammelt und am Ende in eine CSV-Datei geschrieben. 
Optional kann gesteuert werden, ob temporäre JPEG-Dateien während des Vorgangs behalten oder gelöscht werden sollen.

### 4. helpers.h
Beherbergt eine Methode: computePSNR(), die zwei Bilder (Original und dekodiert) vergleicht, welche als flache Arrays
von RGB-Werten (je 8 Bit pro Kanal) vorliegen. Sie berechnet den mittleren quadratischen Fehler (MSE)
über alle Farbkanäle hinweg und leitet daraus den PSNR-Wert in Dezibel (dB) ab.

## Nutzerhandbuch

### Installation
- Roman
```

```
### Nutzeroberfläche
-Roman
## Ergebnisse für ein Beispiel
-Roman
## Literatur Recherche
1.  Quelle: „Comprehensive Image Quality Assessment (IQA) of JPEG, WebP, HEIF and AVIF Formats “ 
Die Studie zeigt, dass JPEG zwar weit verbreitet ist, aber bei höheren Komprimierungsstufen 
deutliche Artefakte erzeugen kann. HEIF hingegen bietet eine bemerkenswerte Bildqualität, hat jedoch längere Kodierungs- und Dekodierungszeiten. 
Im Wertevergleich hat HEIF in beiden Bildern einen deutlichen Vorsprung zu JPEG im PSNR-Wert (siehe Figur 1). <br>
![](./pictures/diagram_literature.png) <br>
[Link](https://osf.io/preprints/osf/ud7w4_v1)
