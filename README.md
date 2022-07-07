# Macchinina

Un progetto che realizza con Arduino una 'macchinina', capace di trovare e seguire il muro lungo la destra o la sinistra.

## Componenti utilizzate

- Arduino UNO R3
- 4 sensori di ultrasuoni HC-SR04
- 2 DC Motor 3-6V con gearbox
- Ponte H: L293D
- Pila 9V
- Sensore IR
- Buzzer

## Obiettivo

Lo scopo del progetto è di creare una macchinina che tramite i 4 sensori di ultrasuoni (due davanti, uno a sinistra e uno a destra) è capace di rilevare la distanza dal muro e adattare la velocità dei motori in modo tale da raggiungerlo (segnalandolo con il buzzer) e procedere lungo la sua sinistra o destra secondo ciò che preferisce l'utente.

## Funzionamento

TODO

## Caratteristiche

- Dimensioni: 20x15x11 cm
- Peso: 400 g

## Librerie usate

- [IRremote](https://github.com/Arduino-IRremote/Arduino-IRremote)
- [NewPing](https://bitbucket.org/teckel12/arduino-new-ping/wiki/Home)

## Schema circuitale

![image](images/macchina_bb.jpg)

## Foto

|||
| ---------| ---------- |
| ![image](images/img1.jpg) | ![image](images/img3.jpg) |
| ![image](images/img3.jpg) | ![image](images/img4.jpg) |
